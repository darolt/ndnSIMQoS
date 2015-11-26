/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Authors: Yaogong Wang <ywang15@ncsu.edu>
 */

#include "ndn-shaper-net-device-face.h"
#include "ndn-l3-protocol.h"

#include "ns3/net-device.h"
#include "ns3/log.h"
#include "ns3/packet.h"
#include "ns3/node.h"
#include "ns3/pointer.h"
#include "ns3/random-variable.h"

#include "ns3/point-to-point-net-device.h"
#include "ns3/channel.h"
#include "ns3/ndn-name.h"
#include "ns3/ndn-header-helper.h"
#include "ns3/ndn-interest.h"
#include "ns3/simulator.h"

NS_LOG_COMPONENT_DEFINE ("ndn.ShaperNetDeviceFace");

namespace ns3 {
namespace ndn {

class TimestampTag : public Tag
{
public:
  TimestampTag (): m_timestamp (Simulator::Now ()) {}

  static TypeId GetTypeId (void)
  {
    static TypeId tid = TypeId ("ns3::ndn::TimestampTag")
      .SetParent<Tag> ()
      .AddConstructor<TimestampTag> ()
    ;
    return tid;
  }

  virtual TypeId GetInstanceTypeId (void) const
  {
    return GetTypeId ();
  }

  virtual uint32_t GetSerializedSize (void) const
  {
    return 8;
  }

  virtual void Serialize (TagBuffer i) const
  {
    i.WriteU64 (m_timestamp.GetTimeStep());
  }

  virtual void Deserialize (TagBuffer i)
  {
    m_timestamp = Time(i.ReadU64 ());
  }

  virtual void Print (std::ostream &os) const
  {
    os << "Timestamp=" << m_timestamp;
  }

  Time GetTimestamp (void) const
  {
    return m_timestamp;
  }

private:
  Time m_timestamp;
};


NS_OBJECT_ENSURE_REGISTERED (ShaperNetDeviceFace);

TypeId
ShaperNetDeviceFace::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::ndn::ShaperNetDeviceFace")
    .SetParent<NetDeviceFace> ()
    .SetGroupName ("Ndn")
    .AddAttribute ("MaxInterest",
                   "Size of the shaper interest queue.",
                   UintegerValue (100),
                   MakeUintegerAccessor (&ShaperNetDeviceFace::m_maxInterest),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("Headroom",
                   "Headroom in interest shaping to absorb burstiness.",
                   DoubleValue (0.98),
                   MakeDoubleAccessor (&ShaperNetDeviceFace::m_headroom),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("UpdateInterval",
                   "Interval to update observed incoming interest rate.",
                   TimeValue (Seconds(0.01)),
                   MakeTimeAccessor (&ShaperNetDeviceFace::m_updateInterval),
                   MakeTimeChecker ())
    .AddAttribute ("QueueMode", 
                   "Determine when to reject/drop an interest (DropTail/PIE/CoDel)",
                   EnumValue (QUEUE_MODE_DROPTAIL),
                   MakeEnumAccessor (&ShaperNetDeviceFace::SetMode),
                   MakeEnumChecker (QUEUE_MODE_DROPTAIL, "QUEUE_MODE_DROPTAIL",
                                    QUEUE_MODE_PIE, "QUEUE_MODE_PIE",
                                    QUEUE_MODE_CODEL, "QUEUE_MODE_CODEL"))
    .AddAttribute ("DelayTarget",
                   "Target queueing delay (for PIE or CoDel).",
                   TimeValue (Seconds(0.02)),
                   MakeTimeAccessor (&ShaperNetDeviceFace::m_delayTarget),
                   MakeTimeChecker ())
    .AddAttribute ("MaxBurst",
                   "Maximum burst allowed before random early drop kicks in (for PIE).",
                   TimeValue (Seconds(0.1)),
                   MakeTimeAccessor (&ShaperNetDeviceFace::m_maxBurst),
                   MakeTimeChecker ())
    .AddAttribute ("DelayObserveInterval",
                   "Interval to observe minimum packet sojourn time (for CoDel).",
                   TimeValue (Seconds(0.1)),
                   MakeTimeAccessor (&ShaperNetDeviceFace::m_delayObserveInterval),
                   MakeTimeChecker ())
    ;
  return tid;
}

ShaperNetDeviceFace::ShaperNetDeviceFace (Ptr<Node> node, const Ptr<NetDevice> &netDevice)
  : NetDeviceFace (node, netDevice)
  , m_lastUpdateTime (0.0)
  , m_byteSinceLastUpdate (0)
  , m_observedInInterestBitRate (0.0)
  , m_outInterestSize (26)
  , m_inInterestSize (27)
  , m_outContentSize (1033)
  , m_inContentSize (1032)
  , m_outInterestFirst (true)
  , m_inInterestFirst (true)
  , m_outContentFirst (true)
  , m_inContentFirst (false)
  , m_shaperState (OPEN)
  , m_old_delay (0.0)
  , m_drop_prob (0.0)
  , m_dq_count (-1)
  , m_avg_dq_rate (0.0)
  , m_dq_start (0.0)
  , m_burst_allowance (Seconds(0.1))
  , m_first_above_time (0.0)
  , m_drop_next (0.0)
  , m_drop_count (0)
  , m_dropping (false)
{
  uint8_t i;
  for (i=8; i<4; i++) {
    m_subShaperState[i] = OPEN;
    m_queuePrevState[i] = 0;
  }

  m_headroom = 0.98;

  DataRateValue dataRate;
  netDevice->GetAttribute ("DataRate", dataRate);
  m_outBitRate = (dataRate.Get().GetBitRate());
  m_inBitRate = m_outBitRate; // assume symmetric bandwidth, can be overridden by SetInRate()
}

ShaperNetDeviceFace::~ShaperNetDeviceFace ()
{
  NS_LOG_FUNCTION_NOARGS ();
}

ShaperNetDeviceFace& ShaperNetDeviceFace::operator= (const ShaperNetDeviceFace &)
{
  return *this;
}

void
ShaperNetDeviceFace::SetInRate (DataRateValue inRate)
{
  NS_LOG_FUNCTION_NOARGS ();
  m_inBitRate = inRate.Get().GetBitRate();
}

void
ShaperNetDeviceFace::SetMode (ShaperNetDeviceFace::QueueMode mode)
{
  NS_LOG_FUNCTION (this << mode);
  m_mode = mode;
  if (m_mode == QUEUE_MODE_PIE)
    Simulator::Schedule (Seconds(0.03), &ShaperNetDeviceFace::PIEUpdate, this);
}

ShaperNetDeviceFace::QueueMode
ShaperNetDeviceFace::GetMode (void)
{
  NS_LOG_FUNCTION (this);
  return m_mode;
}

// JRO - take into account all queues
void
ShaperNetDeviceFace::PIEUpdate ()
{
  NS_LOG_FUNCTION (this);

  double qdelay;
  if (m_avg_dq_rate > 0)
    // JRO
    /*
    qdelay = (m_interestPriorityQueue[0].size() +
              m_interestPriorityQueue[1].size() +
              m_interestPriorityQueue[2].size() +
              m_interestPriorityQueue[3].size())/m_avg_dq_rate;
    */
    qdelay = m_interestQueue.size() / m_avg_dq_rate;
  else
    qdelay = 0.0;

  NS_LOG_LOGIC(this << " PIE qdelay: " << qdelay << " old delay: " << m_old_delay);

  double tmp_p = 0.125 * (qdelay - m_delayTarget.GetSeconds()) + 1.25 * (qdelay - m_old_delay);
  if (m_drop_prob < 0.01)
      tmp_p /= 8.0;
  else if (m_drop_prob < 0.1)
      tmp_p /= 2.0;

  tmp_p += m_drop_prob;
  if (tmp_p < 0)
    m_drop_prob = 0.0;
  else if (tmp_p > 1)
    m_drop_prob = 1.0;
  else
    m_drop_prob = tmp_p;

  NS_LOG_LOGIC(this << " PIE: udpate drop probability to " << m_drop_prob);

  if (qdelay < m_delayTarget.GetSeconds() / 2 && m_old_delay < m_delayTarget.GetSeconds() / 2 and m_drop_prob == 0.0)
    {
      m_dq_count = -1;
      m_avg_dq_rate = 0.0;
      m_burst_allowance = m_maxBurst;
    }

  m_old_delay = qdelay;
  Simulator::Schedule (Seconds(0.03), &ShaperNetDeviceFace::PIEUpdate, this);
}

// JRO
void ShaperNetDeviceFace::CalculateSubShapers(double shapingBitRate, double subShaper[4]) {
  double sumOfWeights = 0.0;
  // weights for each priority are hardcoded
  // ss3 = 2ss2 = 4ss1 = 8ss0
  double weights[4] = {1.0, 2.0, 4.0, 8.0};
  uint8_t i;	
  for (i=0; i<4; i++) {
    if ((m_queuePrevState[i] == 0)&&(m_interestPriorityQueue[i].size() != 0))
    {
      m_queuePrevState[i] = 1;
      m_lastUpdateQueues = Simulator::Now();
    } else if ((m_queuePrevState[i] == 1) &&
        (m_interestPriorityQueue[i].size() == 0) &&
        (Simulator::Now() - m_lastUpdateQueues >= Seconds(0.01)))
    {
      m_queuePrevState[i] = 0;
      m_lastUpdateQueues = Simulator::Now();
    }
    // update subShapers rate
    sumOfWeights += (m_queuePrevState[i] > 0) ? weights[i] : 0.0;
  }

  subShaper[0] = shapingBitRate/sumOfWeights;
  subShaper[1] = (m_queuePrevState[1] > 0) ? subShaper[0]*weights[1] : 0.0;
  subShaper[2] = (m_queuePrevState[2] > 0) ? subShaper[0]*weights[2] : 0.0;
  subShaper[3] = (m_queuePrevState[3] > 0) ? subShaper[0]*weights[3] : 0.0;
  subShaper[0] = (m_queuePrevState[0] > 0) ? subShaper[0]            : 0.0;

  NS_LOG_LOGIC("### queue 0 size: " << m_interestPriorityQueue[0].size());
  NS_LOG_LOGIC("### queue 1 size: " << m_interestPriorityQueue[1].size());
  NS_LOG_LOGIC("### queue 2 size: " << m_interestPriorityQueue[2].size());
  NS_LOG_LOGIC("### queue 3 size: " << m_interestPriorityQueue[3].size());
  NS_LOG_LOGIC("sumOfWeights: " << +sumOfWeights);
  NS_LOG_LOGIC("### shaping rate: " << +shapingBitRate);
  NS_LOG_LOGIC("### ss0: " << +subShaper[0]);
  NS_LOG_LOGIC("### ss1: " << +subShaper[1]);
  NS_LOG_LOGIC("### ss2: " << +subShaper[2]);
  NS_LOG_LOGIC("### ss3: " << +subShaper[3]);
}

bool
ShaperNetDeviceFace::SendImpl (Ptr<Packet> p)
{
  NS_LOG_FUNCTION (this << p);

  HeaderHelper::Type type = HeaderHelper::GetNdnHeaderType (p);
  switch (type)
    {
    case HeaderHelper::INTEREST_NDNSIM:
      {
        Ptr<Packet> packet = p->Copy ();
        Ptr<Interest> header = Create<Interest> ();
        packet->RemoveHeader (*header);
        uint8_t interestPriority = header->GetPriority();
        if (header->GetNack () > 0)
          return NetDeviceFace::SendImpl (p); // no shaping for NACK packets

	// JRO
	// UNDO: replace queue
        if(m_interestPriorityQueue[interestPriority].size() < m_maxInterest)
          {
            NS_LOG_LOGIC(this << " Max queue size " << m_maxInterest);
            if (m_mode == QUEUE_MODE_PIE)
              {
                if (m_burst_allowance <= 0 && !(m_old_delay < m_delayTarget.GetSeconds() / 2 && m_drop_prob < 0.2))
                  {
                    NS_LOG_LOGIC(this << " PIE: flip a coin to decide to drop or not " << m_drop_prob);
                    UniformVariable r (0.0, 1.0);
                    if (r.GetValue () < m_drop_prob)
                      {
                        NS_LOG_LOGIC(this << " PIE drop");
                        return false;
                      }
                  }
              }
            else if (m_mode == QUEUE_MODE_CODEL)
              {
                if (m_dropping && Simulator::Now() >= m_drop_next)
                  {
                    NS_LOG_LOGIC(this << " CoDel drop");
                    m_drop_count++;
                    m_drop_next += Seconds(m_delayObserveInterval.GetSeconds() / sqrt(m_drop_count));
                    return false;
                  }

                TimestampTag tag;
                p->AddPacketTag(tag);
              }

            // JRO
            // push into different queues depending on priority
            m_interestPriorityQueue[interestPriority].push(p);
            NS_LOG_LOGIC("Enqueuing " << +interestPriority);
            NS_LOG_LOGIC("... for queue size is " << m_interestPriorityQueue[interestPriority].size());

	    // UNDO rename it
            if (m_subShaperState[interestPriority] == OPEN) {
              NS_LOG_LOGIC("... and it is OPEN");
              ShaperDequeue(interestPriority);
   	    }

            return true;
          }
        else { // drop Interest but do not return Nack?
            NS_LOG_LOGIC(this << " Tail drop, current size" << m_interestPriorityQueue[interestPriority].size());
            return false;
        }
      }
    case HeaderHelper::CONTENT_OBJECT_NDNSIM:
      {
        if (m_outContentFirst)
          {
            m_outContentSize = p->GetSize(); // first sample
            m_outContentFirst = false;
          }
        else
          {
            m_outContentSize += (p->GetSize() - m_outContentSize) / 8.0; // smoothing
          }

        return NetDeviceFace::SendImpl (p); // no shaping for content packets
      }
    default:
      return false;
    }
}
// JRO
// must have Open/Blocked for the 4 queues
void
ShaperNetDeviceFace::ShaperOpen (uint8_t priority) {
  NS_LOG_FUNCTION (this);

  NS_LOG_LOGIC("ShaperOpen for queue: " << +priority);

  // UNDO: replace m_interestQueue per m_interestPriorityQueue[priority]
  if (m_interestPriorityQueue[priority].size() > 0) {
      ShaperDequeue(priority);
  } else { // queue has no element
      // JRO
      m_subShaperState[priority] = OPEN;

      if (m_mode == QUEUE_MODE_CODEL) {
          if (m_dropping) {
              // leave dropping state if queue is empty
              NS_LOG_LOGIC(this << " CoDel: leave dropping state due to empty queue");
              m_first_above_time = Seconds(0.0);
              m_dropping = false;
          }
      }
  }
}

// take interests from shaper queue and dispatch them to L2 queue 
void
ShaperNetDeviceFace::ShaperDequeue (uint8_t priority)
{
  NS_LOG_FUNCTION (this);
  // UNDO: replace queue
  NS_LOG_LOGIC(this << " shaper qlen: " << m_interestPriorityQueue[priority].size());

  // JRO - take from priority queues
  Ptr<Packet> p = m_interestPriorityQueue[priority].front();

  // all queues will be in the same mode
  if (m_mode == QUEUE_MODE_PIE) {
    DequeuePIE(priority);
  } else if (m_mode == QUEUE_MODE_CODEL) {
    DequeueCODEL(p);
  }

  double shapingBitRate = CalculateShapingRate(p->GetSize());
  // JRO
  // calculate sub-shapers rates and schedule event
  double subShapingBitRate[4] = {0.0, 0.0, 0.0, 0.0};
  CalculateSubShapers(shapingBitRate, subShapingBitRate);
  // size in bits / rate = period
  Time gap = Seconds (p->GetSize() * 8.0 / subShapingBitRate[priority]);
  // pop it later to calculate subShapingRate correctly

  NS_LOG_LOGIC("Actual shaping rate: " << shapingBitRate << "bps, Gap: " << gap);

  // JRO
  m_subShaperState[priority] = BLOCKED;
  m_interestPriorityQueue[priority].pop();

  Simulator::Schedule (gap, &ShaperNetDeviceFace::ShaperOpen, this, priority);

  // send out the interest
  NetDeviceFace::SendImpl (p);
}

void
ShaperNetDeviceFace::ReceiveFromNetDevice (Ptr<NetDevice> device,
                                     Ptr<const Packet> p,
                                     uint16_t protocol,
                                     const Address &from,
                                     const Address &to,
                                     NetDevice::PacketType packetType)
{
  NS_LOG_FUNCTION (device << p << protocol << from << to << packetType);

  HeaderHelper::Type type = HeaderHelper::GetNdnHeaderType (p);
  switch (type)
    {
    case HeaderHelper::INTEREST_NDNSIM:
      {
        if (m_inInterestFirst) {
            m_inInterestSize = p->GetSize(); // first sample
            m_inInterestFirst = false;
            m_lastUpdateTime = Simulator::Now();
            m_byteSinceLastUpdate = p->GetSize();
        }
        else {
            m_inInterestSize += (p->GetSize() - m_inInterestSize) / 8.0; // smoothing

            if (Simulator::Now() - m_lastUpdateTime >= m_updateInterval) {
                m_observedInInterestBitRate = m_byteSinceLastUpdate * 8.0 / (Simulator::Now() - m_lastUpdateTime).GetSeconds();
                m_lastUpdateTime = Simulator::Now();
                m_byteSinceLastUpdate = 0;
            }
            else {
                m_byteSinceLastUpdate += p->GetSize();
            }
        }

        break;
      }
    case HeaderHelper::CONTENT_OBJECT_NDNSIM:
      {
        if (m_inContentFirst)
          {
            m_inContentSize = p->GetSize(); // first sample
            m_inContentFirst = false;
          }
        else
          {
            m_inContentSize += (p->GetSize() - m_inContentSize) / 8.0; // smoothing
          }

        break;
      }
    default:
        break;
    }

  Receive (p);
}

// take interests from shaper queue and dispatch them to L2 queue 
double ShaperNetDeviceFace::CalculateShapingRate (uint32_t packetSize) {
  if (m_outInterestFirst) {
      m_outInterestSize = packetSize; // first sample
      m_outInterestFirst = false;
  } else {
      m_outInterestSize += (packetSize - m_outInterestSize) / 8.0; // smoothing
  }

  double r1 = m_inContentSize / m_outInterestSize; // 1032/26 = 39
  double r2 = m_outContentSize / m_inInterestSize; // 1032/26 = 39
  double c1_over_c2 = 1.0 * m_outBitRate / m_inBitRate; // 1

  NS_LOG_LOGIC("r1: " << r1 << ", r2: " << r2);
  NS_LOG_LOGIC("inrate: " << m_inBitRate);
  NS_LOG_LOGIC("outrate: " << m_outBitRate);
  NS_LOG_LOGIC("c1_over_c2: " << c1_over_c2);
  NS_LOG_LOGIC("In Content size: " << m_inContentSize);
  NS_LOG_LOGIC("Out Content size: " << m_outContentSize);
  NS_LOG_LOGIC("In Interest size: " << m_inInterestSize);
  NS_LOG_LOGIC("Out Interest size: " << m_outInterestSize);

  // calculate max shaping rate when there's no demand in the reverse direction
  // max_s1 = c2/r1
  double maxBitRate = m_inBitRate / r1;

  // calculate min shaping rate when there's infinite demand in the reverse direction
  // minbitRate is actually s1
  double minBitRate, expectedInInterestBitRate;
  // m_outBitRate is c1
  // m_inBitRate is c2
  if (c1_over_c2 < (2 * r2) / (r1 * r2 + 1)) // Table 1 1st row
    {
      minBitRate = m_outBitRate / 2.0;
      expectedInInterestBitRate = m_outBitRate / (2 * r2);
    }
  else if (c1_over_c2 > (r1 * r2 + 1) / (2 * r1)) // Table 1 3rd row
    {
      minBitRate = m_inBitRate / (2 * r1);
      expectedInInterestBitRate = m_inBitRate / 2.0;
    }
  else // Table 1 2nd row
    {
      minBitRate = (r2 * m_inBitRate - m_outBitRate) / (r1 * r2 - 1);
      expectedInInterestBitRate = (r1 * m_outBitRate - m_inBitRate) / (r1 * r2 - 1);
    }

  // other router also uses headroom
  expectedInInterestBitRate *= m_headroom;

  // determine actual shaping rate based on observedInInterestBitRate and expectedInInterestBitRate
  double shapingBitRate;
  // obs_s2 >= expmin_s2, limits shapingBitRate, otherwise it can be less than min
  if (m_observedInInterestBitRate >= expectedInInterestBitRate)
    shapingBitRate = minBitRate;
  else
    // here is the actual shaping rate
    shapingBitRate = minBitRate + (maxBitRate - minBitRate) * (1.0 - m_observedInInterestBitRate / expectedInInterestBitRate) * (1.0 - m_observedInInterestBitRate / expectedInInterestBitRate);

  NS_LOG_LOGIC("Observed incoming interest rate: " << m_observedInInterestBitRate << "bps, Expected incoming interest rate: " << expectedInInterestBitRate << "bps");
  NS_LOG_LOGIC("### MAX: " << maxBitRate);
  NS_LOG_LOGIC("### MIN: " << minBitRate);

  shapingBitRate *= m_headroom;

  return shapingBitRate;
}
// JRO - refactoring
void ShaperNetDeviceFace::DequeuePIE (uint8_t priority) {
      // UNDO: replace queue
      if (m_dq_count == -1 && m_interestPriorityQueue[priority].size() >= 10) {
          // start a measurement cycle
          NS_LOG_LOGIC(this << " PIE: start a measurement cycle");
          m_dq_start = Simulator::Now();
          m_dq_count = 0;
        }

      if (m_dq_count != -1)
        {
          m_dq_count += 1;

          if (m_dq_count >= 10) {
              // done with a measurement cycle
              NS_LOG_LOGIC(this << " PIE: done with a measurement cycle");

              Time tmp = Simulator::Now() - m_dq_start;
              if (m_avg_dq_rate == 0.0) {
                m_avg_dq_rate = m_dq_count / tmp.GetSeconds();
              } else {
                m_avg_dq_rate = 0.9 * m_avg_dq_rate + 0.1 * m_dq_count / tmp.GetSeconds();
	      }
              NS_LOG_LOGIC(this << " PIE: average dequeue rate " << m_avg_dq_rate);

              if (m_interestQueue.size() >= 10) {
                  // start a measurement cycle
                  NS_LOG_LOGIC(this << " PIE: start a measurement cycle");
                  m_dq_start = Simulator::Now();
                  m_dq_count = 0;
              } else {
                  m_dq_count = -1;
              }

              if (m_burst_allowance > 0)
                m_burst_allowance -= tmp;

              NS_LOG_LOGIC(this << " PIE: burst allowance " << m_burst_allowance);
            }
        }
  return;
}
// JRO - refactoring
void ShaperNetDeviceFace::DequeueCODEL (Ptr<Packet> p) {
      TimestampTag tag;
      p->PeekPacketTag (tag);
      Time sojourn_time = Simulator::Now() - tag.GetTimestamp ();
      NS_LOG_LOGIC(this << " CoDel sojourn time: " << sojourn_time);
      p->RemovePacketTag (tag);

      if (m_dropping && sojourn_time < m_delayTarget) {
          // leave dropping state
          NS_LOG_LOGIC(this << " CoDel: leave dropping state due to low delay");
          m_first_above_time = Seconds(0.0);
          m_dropping = false;
        }
      else if (!m_dropping && sojourn_time >= m_delayTarget) {
          if (m_first_above_time == Seconds(0.0)) {
              NS_LOG_LOGIC(this << " CoDel: first above time " << Simulator::Now());
              m_first_above_time = Simulator::Now() + m_delayObserveInterval;
          } else if (Simulator::Now() >= m_first_above_time
                   && (Simulator::Now() - m_drop_next < m_delayObserveInterval || Simulator::Now() - m_first_above_time >= m_delayObserveInterval)) {
              // enter dropping state
              NS_LOG_LOGIC(this << " CoDel: enter dropping state");
              m_dropping = true;

              if (Simulator::Now() - m_drop_next < m_delayObserveInterval)
                m_drop_count = m_drop_count>2 ? m_drop_count-2 : 0;
              else
                m_drop_count = 0;

              m_drop_next = Simulator::Now();
            }
        }
  return;
}

} // namespace ndnsim
} // namespace ns3


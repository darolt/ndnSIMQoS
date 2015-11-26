/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
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
 * Author: Yaogong Wang <ywang15@ncsu.edu>
 */

#include "ndn-consumer-window-relentless.h"
#include "ns3/ndn-interest.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/packet.h"
#include "ns3/ndnSIM/utils/ndn-fw-pmin-tag.h"

#include <boost/lexical_cast.hpp>

NS_LOG_COMPONENT_DEFINE ("ndn.ConsumerWindowRelentless");

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED (ConsumerWindowRelentless);

TypeId
ConsumerWindowRelentless::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ndn::ConsumerWindowRelentless")
    .SetGroupName ("Ndn")
    .SetParent<ConsumerWindow> ()
    .AddConstructor<ConsumerWindowRelentless> ()

    .AddTraceSource ("SsthreshTrace",
                     "Ssthresh that determines whether we are in slow start or congestion avoidance phase",
                     MakeTraceSourceAccessor (&ConsumerWindowRelentless::m_ssthresh))
    ;

  return tid;
}

ConsumerWindowRelentless::ConsumerWindowRelentless ()
  : ConsumerWindow ()
  , m_ssthresh (std::numeric_limits<uint32_t>::max ())
  , m_window_cnt (0)
  , m_alpha (1.0)
  , m_nackRatio (0.0)
  , m_counterStarted (false)
  , m_nack (0)
  , m_data (0)
{
}

ConsumerWindowRelentless::~ConsumerWindowRelentless ()
{
}

void
ConsumerWindowRelentless::AdjustWindowOnContentObject (const Ptr<const ContentObject> &contentObject,
                                                       Ptr<Packet> payload)
{
  if (m_window < m_ssthresh)
    {
      // in slow start phase
      m_window++;
    }
  else
    {
      // in congestion avoidance phase
      if (m_window_cnt >= m_window / m_alpha)
        {
          m_window++;
          m_window_cnt = 0;
        }
      else
        {
          m_window_cnt++;
        }
    }

  m_data++;

  NS_LOG_DEBUG ("Window: " << m_window << ", InFlight: " << m_inFlight << ", Ssthresh: " << m_ssthresh);
}

void
ConsumerWindowRelentless::AdjustWindowOnNack (const Ptr<const Interest> &interest, Ptr<Packet> payload)
{
  m_window = std::max<uint32_t> (1, m_window - 1);
  m_ssthresh = m_window;

  FwPminTag pminTag;
  if (payload->RemovePacketTag (pminTag))
    {
      if (m_nackRatio > pminTag.GetPmin ())
        m_alpha = m_nackRatio / pminTag.GetPmin ();
      else
        m_alpha = 1.0;

      NS_LOG_DEBUG ("pmin: " << pminTag.GetPmin () << " pavg: " << m_nackRatio << " alpha: " << m_alpha);
    }

  m_nack++;
  if (!m_counterStarted)
    {
      m_counterStarted = true;
      Simulator::Schedule (Seconds (0.1), &ConsumerWindowRelentless::RecalculateNackRatio, this);
    }

  NS_LOG_DEBUG ("Window: " << m_window << ", InFlight: " << m_inFlight << ", Ssthresh: " << m_ssthresh);
}

void
ConsumerWindowRelentless::RecalculateNackRatio ()
{
  double sample = m_nack>0 ? 1.0 * m_nack / (m_nack + m_data) : 0.0;
  m_nackRatio = m_nackRatio * 0.875 + sample * 0.125;

  m_nack = 0;
  m_data = 0;
  Simulator::Schedule (Seconds (0.1), &ConsumerWindowRelentless::RecalculateNackRatio, this);
}

} // namespace ndn
} // namespace ns3

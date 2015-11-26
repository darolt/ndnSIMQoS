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
 */

#include "ndn-consumer-rate-relentless.h"
#include "ns3/ptr.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/packet.h"
#include "ns3/callback.h"
#include "ns3/string.h"
#include "ns3/boolean.h"
#include "ns3/uinteger.h"
#include "ns3/double.h"

#include "ns3/ndn-app-face.h"
#include "ns3/ndn-interest.h"
#include "ns3/ndn-content-object.h"

#include <boost/lexical_cast.hpp>

NS_LOG_COMPONENT_DEFINE ("ndn.ConsumerRateRelentless");

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED (ConsumerRateRelentless);

TypeId
ConsumerRateRelentless::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ndn::ConsumerRateRelentless")
    .SetGroupName ("Ndn")
    .SetParent<ConsumerRate> ()
    .AddConstructor<ConsumerRateRelentless> ()

    .AddAttribute ("Alpha",
                   "Rate increase factor",
                   DoubleValue (20.0),
                   MakeDoubleAccessor (&ConsumerRateRelentless::m_alpha),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("Beta",
                   "Rate decrease factor",
                   DoubleValue (1.1),
                   MakeDoubleAccessor (&ConsumerRateRelentless::m_beta),
                   MakeDoubleChecker<double> ())
    ;

  return tid;
}

ConsumerRateRelentless::ConsumerRateRelentless ()
  : ConsumerRate ()
  , m_inSlowStart (true)
{
}

ConsumerRateRelentless::~ConsumerRateRelentless ()
{
}

void
ConsumerRateRelentless::AdjustFrequencyOnContentObject (const Ptr<const ContentObject> &contentObject,
                                                      Ptr<Packet> payload)
{
  if (m_inSlowStart)
    m_frequency = m_frequency + m_alpha;
  else
    m_frequency = m_frequency + m_alpha / m_frequency;

  NS_LOG_DEBUG ("Current frequency: " << m_frequency);
}

void
ConsumerRateRelentless::AdjustFrequencyOnNack (const Ptr<const Interest> &interest,
                                               Ptr<Packet> payload)
{
  m_frequency = std::max<double> (10.0, m_frequency - m_beta);
  m_inSlowStart = false;

  NS_LOG_DEBUG ("Current frequency: " << m_frequency);
}

} // namespace ndn
} // namespace ns3

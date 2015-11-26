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

#ifndef NDN_CONSUMER_RATE_RELENTLESS_H
#define NDN_CONSUMER_RATE_RELENTLESS_H

#include "ndn-consumer-rate.h"

namespace ns3 {
namespace ndn {

class ConsumerRateRelentless: public ConsumerRate
{
public:
  static TypeId GetTypeId ();

  ConsumerRateRelentless ();
  virtual ~ConsumerRateRelentless ();

protected:
  virtual void
  AdjustFrequencyOnContentObject (const Ptr<const ContentObject> &contentObject,
                                  Ptr<Packet> payload);

  virtual void
  AdjustFrequencyOnNack (const Ptr<const Interest> &interest,
                         Ptr<Packet> payload);

  double m_alpha;
  double m_beta;
  bool m_inSlowStart;
};

} // namespace ndn
} // namespace ns3

#endif

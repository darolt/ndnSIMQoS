/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2011 University of California, Los Angeles
 *
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
 * Author:  Yaogong Wang <ywang15@ncsu.edu>
 *
 */
#ifndef NDNSIM_CONGESTION_AWARE_H
#define NDNSIM_CONGESTION_AWARE_H

#include "nacks.h"

namespace ns3 {
namespace ndn {
namespace fw {

/**
 * \ingroup ndn
 */
class CongestionAware :
    public Nacks
{
public:
  static TypeId
  GetTypeId (void);

  CongestionAware();

protected:
  virtual bool
  DoPropagateInterest (Ptr<Face> inFace,
                       Ptr<const Interest> header,
                       Ptr<const Packet> origPacket,
                       Ptr<pit::Entry> pitEntry);

  virtual void
  WillSatisfyPendingInterest (Ptr<Face> inFace,
                              Ptr<pit::Entry> pitEntry);

  virtual void
  DidExhaustForwardingOptions (Ptr<Face> inFace,
                               Ptr<const Interest> header,
                               Ptr<const Packet> packet,
                               Ptr<pit::Entry> pitEntry);

  virtual void
  DidReceiveValidNack (Ptr<Face> incomingFace,
                       uint32_t nackCode,
                       Ptr<const Interest> header,
                       Ptr<const Packet> origPacket,
                       Ptr<pit::Entry> pitEntry);
private:
  typedef Nacks super;
  uint32_t m_k;
};

} // namespace fw
} // namespace ndn
} // namespace ns3

#endif // NDNSIM_CONGESTION_AWARE_H

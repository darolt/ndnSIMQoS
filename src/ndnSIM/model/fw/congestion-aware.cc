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

#include "congestion-aware.h"

#include "ns3/ndn-pit.h"
#include "ns3/ndn-pit-entry.h"
#include "ns3/ndn-interest.h"
#include "ns3/ndn-content-object.h"
#include "ns3/ndn-pit.h"
#include "ns3/ndn-fib.h"
#include "ns3/ndn-fib-entry.h"
#include "ns3/ndn-content-store.h"
#include "ns3/random-variable.h"
#include "ns3/ndnSIM/utils/ndn-fw-hop-count-tag.h"
#include "ns3/ndnSIM/utils/ndn-fw-pmin-tag.h"

#include "ns3/assert.h"
#include "ns3/ptr.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/boolean.h"
#include "ns3/string.h"

#include <math.h>
#include <boost/ref.hpp>
#include <boost/foreach.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
#include <boost/tuple/tuple.hpp>
namespace ll = boost::lambda;

NS_LOG_COMPONENT_DEFINE ("ndn.fw.CongestionAware");

namespace ns3 {
namespace ndn {
namespace fw {

NS_OBJECT_ENSURE_REGISTERED (CongestionAware);

TypeId
CongestionAware::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ndn::fw::CongestionAware")
    .SetGroupName ("Ndn")
    .SetParent<Nacks> ()
    .AddConstructor <CongestionAware> ()

    .AddAttribute ("K", "Make traffic fraction proportional to (1/p)^k",
                   UintegerValue (1),
                   MakeUintegerAccessor (&CongestionAware::m_k),
                   MakeUintegerChecker<uint32_t> ())
    ;

  return tid;
}

CongestionAware::CongestionAware ()
{
}

bool
CongestionAware::DoPropagateInterest (Ptr<Face> inFace,
                                     Ptr<const Interest> header,
                                     Ptr<const Packet> origPacket,
                                     Ptr<pit::Entry> pitEntry)
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT_MSG (m_pit != 0, "PIT should be aggregated with forwarding strategy");

  bool success = false;

  double total = 0.0;
  BOOST_FOREACH (const fib::FaceMetric &metricFace, pitEntry->GetFibEntry ()->m_faces.get<fib::i_nth> ())
    {
      if (pitEntry->GetFibEntry ()->m_faces.size () > 1)
        NS_LOG_DEBUG (pitEntry->GetFibEntry ()->GetPrefix () << " " << metricFace.GetFace () << " NackRatio: " << metricFace.GetNackRatio ());
      total += pow (1.0 / metricFace.GetNackRatio(), m_k);
    }

  UniformVariable r (0, 1.0);
  double p_random = r.GetValue ();
  double p_sum = 0;
  BOOST_FOREACH (const fib::FaceMetric &metricFace, pitEntry->GetFibEntry ()->m_faces.get<fib::i_nth> ())
    {
      p_sum += pow (1.0 / metricFace.GetNackRatio(), m_k) / total;
      if (p_random <= p_sum)
        {
          success = TrySendOutInterest (inFace, metricFace.GetFace (), header, origPacket, pitEntry);

          if (!success && pitEntry->GetFibEntry ()->m_faces.size () > 1)
            pitEntry->GetFibEntry ()->UpdateFaceCounter (metricFace.GetFace (), true);

          break;
        }
    }

  return success;
}

void
CongestionAware::WillSatisfyPendingInterest (Ptr<Face> inFace,
                                            Ptr<pit::Entry> pitEntry)
{
  if (inFace != 0 && pitEntry->GetFibEntry ()->m_faces.size () > 1)
    {
      pitEntry->GetFibEntry ()->UpdateFaceCounter (inFace, false);
    }

  super::WillSatisfyPendingInterest (inFace, pitEntry);
}

void
CongestionAware::DidExhaustForwardingOptions (Ptr<Face> inFace,
                                             Ptr<const Interest> header,
                                             Ptr<const Packet> origPacket,
                                             Ptr<pit::Entry> pitEntry)
{
  if (m_nacksEnabled)
    {
      Ptr<Packet> packet = Create<Packet> ();
      Ptr<Interest> nackHeader = Create<Interest> (*header);
      nackHeader->SetNack (Interest::NACK_GIVEUP_PIT);
      packet->AddHeader (*nackHeader);

      FwHopCountTag hopCountTag;
      if (origPacket->PeekPacketTag (hopCountTag))
        {
          packet->AddPacketTag (hopCountTag);
        }
      else
        {
          NS_LOG_DEBUG ("No FwHopCountTag tag associated with original Interest");
        }

      if (pitEntry->GetFibEntry ()->m_faces.size () > 1)
        {
          double pmin = 2.0;
          BOOST_FOREACH (const fib::FaceMetric &metricFace, pitEntry->GetFibEntry ()->m_faces.get<fib::i_nth> ())
            {
              if (metricFace.GetNackRatio() != 1e-6 && metricFace.GetNackRatio() < pmin)
                pmin = metricFace.GetNackRatio();
            }

          if (pmin == 2.0)
            {
              FwPminTag pminTag;
              if (origPacket->PeekPacketTag (pminTag))
                {
                  packet->AddPacketTag (pminTag);
                }
            }
          else
            {
              FwPminTag pminTag;
              if (origPacket->PeekPacketTag (pminTag) && pmin > pminTag.GetPmin())
                {
                  packet->AddPacketTag (pminTag);
                }
              else
                {
                  pminTag.SetPmin (pmin);
                  packet->AddPacketTag (pminTag);
                }
            }
        }
      else
        {
          FwPminTag pminTag;
          if (origPacket->PeekPacketTag (pminTag))
            {
              packet->AddPacketTag (pminTag);
            }
        }

      BOOST_FOREACH (const pit::IncomingFace &incoming, pitEntry->GetIncoming ())
        {
          NS_LOG_DEBUG ("Send NACK for " << boost::cref (nackHeader->GetName ()) << " to " << boost::cref (*incoming.m_face));
          incoming.m_face->Send (packet->Copy ());

          m_outNacks (nackHeader, incoming.m_face);
        }

      pitEntry->ClearOutgoing (); // to force erasure of the record
    }

  ForwardingStrategy::DidExhaustForwardingOptions (inFace, header, origPacket, pitEntry);
}

void
CongestionAware::DidReceiveValidNack (Ptr<Face> inFace,
                                     uint32_t nackCode,
                                     Ptr<const Interest> header,
                                     Ptr<const Packet> origPacket,
                                     Ptr<pit::Entry> pitEntry)
{
  if (inFace != 0 && pitEntry->GetFibEntry ()->m_faces.size () > 1 &&
      (nackCode == Interest::NACK_CONGESTION ||
       nackCode == Interest::NACK_GIVEUP_PIT))
    {
      pitEntry->GetFibEntry ()->UpdateFaceCounter (inFace, true);
    }

  NS_LOG_DEBUG ("nackCode: " << nackCode << " for [" << header->GetName () << "]");

  // If NACK is NACK_GIVEUP_PIT, then neighbor gave up trying to and removed it's PIT entry.
  // So, if we had an incoming entry to this neighbor, then we can remove it now
  if (nackCode == Interest::NACK_GIVEUP_PIT)
    {
      pitEntry->RemoveIncoming (inFace);
    }

  if (nackCode == Interest::NACK_LOOP ||
      nackCode == Interest::NACK_CONGESTION ||
      nackCode == Interest::NACK_GIVEUP_PIT)
    {
      pitEntry->SetWaitingInVain (inFace);

      if (!pitEntry->AreAllOutgoingInVain ()) // not all ougtoing are in vain
        {
          NS_LOG_DEBUG ("Not all outgoing are in vain");
          // suppress
          // Don't do anything, we are still expecting data from some other face
          m_dropNacks (header, inFace);
          return;
        }

      Ptr<Packet> nonNackInterest = Create<Packet> ();
      Ptr<Interest> nonNackHeader = Create<Interest> (*header);
      nonNackHeader->SetNack (Interest::NORMAL_INTEREST);
      nonNackInterest->AddHeader (*nonNackHeader);

      FwHopCountTag hopCountTag;
      if (origPacket->PeekPacketTag (hopCountTag))
        {
          nonNackInterest->AddPacketTag (hopCountTag);
        }
      else
        {
          NS_LOG_DEBUG ("No FwHopCountTag tag associated with received NACK");
        }

      FwPminTag pminTag;
      if (origPacket->PeekPacketTag (pminTag))
        {
          nonNackInterest->AddPacketTag (pminTag);
        }

      DidExhaustForwardingOptions (inFace, nonNackHeader, nonNackInterest, pitEntry);
    }
}


} // namespace fw
} // namespace ndn
} // namespace ns3

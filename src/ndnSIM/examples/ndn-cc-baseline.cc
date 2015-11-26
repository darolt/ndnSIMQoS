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

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/ndnSIM-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ndn-shaper-net-device-face.h"
#include <ns3/ndnSIM/utils/tracers/ndn-l3-aggregate-tracer.h>
#include <ns3/ndnSIM/utils/tracers/ndn-l3-rate-tracer.h>
#include <ns3/ndnSIM/utils/tracers/ndn-app-delay-tracer.h>

using namespace ns3;

int main (int argc, char *argv[]) {

  std::string qsize ("300"), rate_trace ("rate-trace.txt"), bw_b("1Mbps");
  uint32_t i;
  bool randomPacketSize = false;

  ndn::ShaperNetDeviceFace::QueueMode mode_enum = ndn::ShaperNetDeviceFace::QUEUE_MODE_DROPTAIL;

  // queue size
  uint32_t qsize_int;
  std::istringstream (qsize) >> qsize_int;

  // Setup topology
  NodeContainer nodes;
  nodes.Create (10);
  // Getting containers for the consumer/producer
  Ptr<Node> e1 = nodes.Get (1);
  Ptr<Node> r0 = nodes.Get (2);
  Ptr<Node> r1 = nodes.Get (3);
  Ptr<Node> d0 = nodes.Get (4);
  Ptr<Node> d1 = nodes.Get (5);

  Config::SetDefault ("ns3::DropTailQueue::MaxPackets", StringValue (qsize));

  PointToPointHelper p2p;
  p2p.SetDeviceAttribute("DataRate", StringValue ("100Mbps"));
  p2p.SetChannelAttribute("Delay", StringValue ("10ms"));
  for (i=0; i<4; i++) { // left side
    p2p.Install (nodes.Get (i), nodes.Get (8));
  }
  for (i=4; i<8; i++) { // left side
    p2p.Install (nodes.Get (i), nodes.Get (9));
  }
  p2p.SetDeviceAttribute("DataRate", StringValue ("10Mbps"));
  p2p.Install (nodes.Get (8), nodes.Get (9));

  // Install CCNx stack on all nodes (NDN Stack?)
  ndn::StackHelper ndnHelper;
  ndnHelper.SetContentStore ("ns3::ndn::cs::Nocache");
  ndnHelper.SetForwardingStrategy ("ns3::ndn::fw::BestRoute", "EnableNACKs", "true");
  ndnHelper.EnableShaper (true, qsize_int, 0.98, Seconds(0.01), mode_enum);
  ndnHelper.Install(nodes.Get(8));
  //Ptr<ndn::FaceContainer> faces = ndnHelper.Install(nodes.Get(8));
  //DynamicCast<ndn::ShaperNetDeviceFace> (*(--(faces->End())))->SetInRate(DataRateValue(DataRate(bw_b)));
  ndnHelper.Install(nodes.Get(9));

  ndnHelper.SetForwardingStrategy ("ns3::ndn::fw::BestRoute"); // No hop-by-hop interest shaping, no NACKs.
  ndnHelper.EnableShaper (false, qsize_int, 0.98, Seconds(0.1), mode_enum);

  for (i=0; i<8; i++) { // left side
    ndnHelper.Install (nodes.Get (i));
  }

  // Installing global routing interface on all nodes
  ndn::GlobalRoutingHelper ndnGlobalRoutingHelper;
  ndnGlobalRoutingHelper.InstallAll ();

  // CONSUMERS
  ndn::AppHelper *consumerHelper[8];
  ApplicationContainer app[8];
  uint32_t priorities[8] = { 0, 1, 2, 3, 0, 1, 2, 3 };
  //double startTimes[8] = {0.0, 10.0, 20.0, 30.0, 10.0, 10.0, 20.0, 0.0};
  //double stopTimes[8] = {40.0, 50.0, 60.0, 70.0, 40.0, 30.0, 70.0, 50.0};
  std::string prefix[8] = { "l0", "l1", "l2", "l3", "e0", "e1", "e2", "e3" };
  for (i=0; i<8; i++) { 
    consumerHelper[i] = new ndn::AppHelper ("ns3::ndn::ConsumerWindowAIMD");
    consumerHelper[i]->SetPrefix (prefix[i]);
    consumerHelper[i]->SetAttribute ("StartTime", TimeValue (Seconds (0.0)));
    consumerHelper[i]->SetAttribute ("StopTime", TimeValue (Seconds (10.0)));
    app[i] = consumerHelper[i]->Install (nodes.Get(i));
    DynamicCast<ndn::Consumer> (app[i].Get(0))->SetPriority(priorities[i]);
    delete consumerHelper[i];
  }
  
  // PRODUCERS
  ndn::AppHelper producerHelper ("ns3::ndn::Producer");

  if (!randomPacketSize) {
    producerHelper.SetAttribute ("PayloadSize", StringValue("1000"));
  } else {
      producerHelper.SetAttribute ("RandomPayloadSizeMin", StringValue("600"));
      producerHelper.SetAttribute ("RandomPayloadSizeMax", StringValue("1400"));
  }

  std::string prefix2[8] = { "e0", "e1", "e2", "e3", "l0", "l1", "l2", "l3" };
  for (i=0; i<8; i++) {
    ndnGlobalRoutingHelper.AddOrigins (prefix2[i], nodes.Get(i));
    producerHelper.SetPrefix (prefix2[i]);
    producerHelper.Install (nodes.Get(i));
  }
  ndnGlobalRoutingHelper.CalculateRoutes ();

  Simulator::Stop (Seconds (10.1));

  boost::tuple< boost::shared_ptr<std::ostream>, std::list<Ptr<ndn::L3RateTracer> > >
    rateTracers = ndn::L3RateTracer::InstallAll (rate_trace, Seconds (1.0));

  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}

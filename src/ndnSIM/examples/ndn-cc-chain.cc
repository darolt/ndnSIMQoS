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
#include "ns3/ndn-face-container.h"
#include "ns3/ndn-shaper-net-device-face.h"
#include <ns3/ndnSIM/utils/tracers/ndn-l3-aggregate-tracer.h>
#include <ns3/ndnSIM/utils/tracers/ndn-app-delay-tracer.h>

using namespace ns3;

int
main (int argc, char *argv[])
{
  std::string twoway ("0"), consumer ("WindowRelentless"), shaper ("PIE");
  std::string bw_a ("10Mbps"), bw_b ("10Mbps"), lat ("13ms"), qsize ("38");
  std::string isize ("0"), payload_0 ("1000"), payload_3 ("1000"); 
  std::string agg_trace ("aggregate-trace.txt"), delay_trace ("app-delays-trace.txt"); 

  CommandLine cmd;
  cmd.AddValue("twoway", "0 - one-way traffic, 1 - two-way traffic", twoway);
  cmd.AddValue("consumer", "Consumer type (AIMD/CUBIC/RAAQM/WindowRelentless/RateRelentless/RateFeedback)", consumer);
  cmd.AddValue("shaper", "Shaper mode (None/DropTail/PIE/CoDel)", shaper);
  cmd.AddValue("bw_a", "Link bandwidth from 1 to 2", bw_a);
  cmd.AddValue("bw_b", "Link bandwidth from 2 to 1", bw_b);
  cmd.AddValue("lat", "Link latency between 1 and 2", lat);
  cmd.AddValue("qsize", "L2/Shaper queue size", qsize);
  cmd.AddValue("isize", "Randomized interest size, 0 means not to randomize", isize);
  cmd.AddValue("payload_0", "Payload size on 0, 0 means randomized between 600B and 1400B", payload_0);
  cmd.AddValue("payload_3", "Payload size on 3, 0 means randomized between 600B and 1400B", payload_3);
  cmd.AddValue("agg_trace", "Aggregate trace file name", agg_trace);
  cmd.AddValue("delay_trace", "App delay trace file name", delay_trace);
  cmd.Parse (argc, argv);

  ndn::ShaperNetDeviceFace::QueueMode mode_enum;
  if (shaper == "DropTail")
    mode_enum = ndn::ShaperNetDeviceFace::QUEUE_MODE_DROPTAIL;
  else if (shaper == "PIE")
    mode_enum = ndn::ShaperNetDeviceFace::QUEUE_MODE_PIE;
  else if (shaper == "CoDel")
    mode_enum = ndn::ShaperNetDeviceFace::QUEUE_MODE_CODEL;
  else if (shaper != "None")
    return -1;

  uint32_t qsize_int;
  std::istringstream (qsize) >> qsize_int;

  // Setup topology
  NodeContainer nodes;
  nodes.Create (4);

  Config::SetDefault ("ns3::DropTailQueue::MaxPackets", StringValue (qsize));

  PointToPointHelper p2p;
  p2p.SetDeviceAttribute("DataRate", StringValue ("1Gbps"));
  p2p.SetChannelAttribute("Delay", StringValue ("1ms"));
  p2p.Install (nodes.Get (0), nodes.Get (1));
  p2p.Install (nodes.Get (2), nodes.Get (3));

  ObjectFactory factory;
  factory.SetTypeId("ns3::PointToPointNetDevice");
  Config::SetDefault ("ns3::PointToPointNetDevice::DataRate", StringValue (bw_a));
  Ptr<PointToPointNetDevice> devA = factory.Create<PointToPointNetDevice> ();
  devA->SetAddress (Mac48Address::Allocate ());
  nodes.Get (1)->AddDevice (devA);

  factory.SetTypeId("ns3::DropTailQueue");
  Ptr<Queue> queueA = factory.Create<Queue> ();
  devA->SetQueue (queueA);

  factory.SetTypeId("ns3::PointToPointNetDevice");
  Config::SetDefault ("ns3::PointToPointNetDevice::DataRate", StringValue (bw_b));
  Ptr<PointToPointNetDevice> devB = factory.Create<PointToPointNetDevice> ();
  devB->SetAddress (Mac48Address::Allocate ());
  nodes.Get (2)->AddDevice (devB);

  factory.SetTypeId("ns3::DropTailQueue");
  Ptr<Queue> queueB = factory.Create<Queue> ();
  devB->SetQueue (queueB);

  factory.SetTypeId("ns3::PointToPointChannel");
  Config::SetDefault ("ns3::PointToPointChannel::Delay", StringValue (lat));
  Ptr<PointToPointChannel> channel = factory.Create<PointToPointChannel> ();
  devA->Attach (channel);
  devB->Attach (channel);

  // Install CCNx stack on all nodes
  ndn::StackHelper ndnHelper;
  if (shaper != "None")
    {
      ndnHelper.SetForwardingStrategy ("ns3::ndn::fw::BestRoute", "EnableNACKs", "true");
      ndnHelper.EnableShaper (true, qsize_int, 0.97, Seconds(0.1), mode_enum);
    }
  else
    {
      ndnHelper.SetForwardingStrategy ("ns3::ndn::fw::BestRoute"); // No hop-by-hop interest shaping, no NACKs.
    }
  ndnHelper.SetContentStore ("ns3::ndn::cs::Nocache");
  Ptr<ndn::FaceContainer> faces = ndnHelper.InstallAll ();
  if (shaper != "None")
    {
      for (ndn::FaceContainer::Iterator i = faces->Begin (); i != faces->End (); ++i)
        {
          if (DynamicCast<ndn::ShaperNetDeviceFace> (*i)->GetNetDevice() == devA)
            DynamicCast<ndn::ShaperNetDeviceFace> (*i)->SetInRate(DataRateValue(DataRate(bw_b)));
          else if (DynamicCast<ndn::ShaperNetDeviceFace> (*i)->GetNetDevice() == devB)
            DynamicCast<ndn::ShaperNetDeviceFace> (*i)->SetInRate(DataRateValue(DataRate(bw_a)));
        }
    }

  // Installing global routing interface on all nodes
  ndn::GlobalRoutingHelper ndnGlobalRoutingHelper;
  ndnGlobalRoutingHelper.InstallAll ();

  // Getting containers for the consumer/producer
  Ptr<Node> cp1 = nodes.Get (0);
  Ptr<Node> cp2 = nodes.Get (3);

  // Install consumer
  ndn::AppHelper *consumerHelper;
  if (consumer == "AIMD")
    consumerHelper = new ndn::AppHelper ("ns3::ndn::ConsumerWindowAIMD");
  else if (consumer == "CUBIC")
    consumerHelper = new ndn::AppHelper ("ns3::ndn::ConsumerWindowCUBIC");
  else if (consumer == "RAAQM")
    consumerHelper = new ndn::AppHelper ("ns3::ndn::ConsumerWindowRAAQM");
  else if (consumer == "WindowRelentless")
    consumerHelper = new ndn::AppHelper ("ns3::ndn::ConsumerWindowRelentless");
  else if (consumer == "RateRelentless")
    consumerHelper = new ndn::AppHelper ("ns3::ndn::ConsumerRateRelentless");
  else if (consumer == "RateFeedback")
    consumerHelper = new ndn::AppHelper ("ns3::ndn::ConsumerRateFeedback");
  else
    return -1;

  consumerHelper->SetAttribute ("LifeTime", TimeValue (Seconds (5.0)));

  if (isize != "0")
    consumerHelper->SetAttribute (std::string("RandComponentLenMax"), StringValue(isize));

  consumerHelper->SetPrefix ("/cp2");
  UniformVariable r (0.0, 5.0);
  consumerHelper->SetAttribute ("StartTime", TimeValue (Seconds (r.GetValue ())));
  consumerHelper->Install (cp1);

  if (twoway == "1")
    {
      consumerHelper->SetPrefix ("/cp1");
      consumerHelper->SetAttribute ("StartTime", TimeValue (Seconds (r.GetValue ())));
      consumerHelper->Install (cp2);
    }

  delete consumerHelper;

  // Register prefix with global routing controller and install producer
  ndn::AppHelper producerHelper ("ns3::ndn::Producer");

  ndnGlobalRoutingHelper.AddOrigins ("/cp1", cp1);
  if (payload_0 != "0")
    {
      producerHelper.SetAttribute ("PayloadSize", StringValue(payload_0));
    }
  else
    {
      producerHelper.SetAttribute ("RandomPayloadSizeMin", StringValue("600"));
      producerHelper.SetAttribute ("RandomPayloadSizeMax", StringValue("1400"));
    }
  producerHelper.SetPrefix ("/cp1");
  producerHelper.Install (cp1);

  ndnGlobalRoutingHelper.AddOrigins ("/cp2", cp2);
  if (payload_3 != "0")
    {
      producerHelper.SetAttribute ("PayloadSize", StringValue(payload_3));
    }
  else
    {
      producerHelper.SetAttribute ("RandomPayloadSizeMin", StringValue("600"));
      producerHelper.SetAttribute ("RandomPayloadSizeMax", StringValue("1400"));
    }
  producerHelper.SetPrefix ("/cp2");
  producerHelper.Install (cp2);

  // Calculate and install FIBs
  ndnGlobalRoutingHelper.CalculateRoutes ();

  Simulator::Stop (Seconds (70.1));

  boost::tuple< boost::shared_ptr<std::ostream>, std::list<Ptr<ndn::L3AggregateTracer> > >
    aggTracers = ndn::L3AggregateTracer::InstallAll (agg_trace, Seconds (10.0));

  boost::tuple< boost::shared_ptr<std::ostream>, std::list<Ptr<ndn::AppDelayTracer> > >
    delayTracers = ndn::AppDelayTracer::InstallAll (delay_trace);

  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}

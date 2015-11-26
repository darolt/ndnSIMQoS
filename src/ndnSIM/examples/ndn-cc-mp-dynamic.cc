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
#include <ns3/ndnSIM/utils/tracers/ndn-app-delay-tracer.h>

using namespace ns3;

int
main (int argc, char *argv[])
{
  std::string consumer ("WindowRelentless"), shaper ("PIE"), strategy ("CongestionAware"), k ("2");
  std::string agg_trace ("aggregate-trace.txt"), delay_trace ("app-delays-trace.txt");

  CommandLine cmd;
  cmd.AddValue("consumer", "Consumer type (AIMD/CUBIC/RAAQM/WindowRelentless/RateRelentless/RateFeedback)", consumer);
  cmd.AddValue("shaper", "Shaper mode (None/DropTail/PIE/CoDel)", shaper);
  cmd.AddValue("strategy", "Forwarding strategy (BestRoute/CongestionAware)", strategy);
  cmd.AddValue("k", "Parameter K of CongestionAware forwarding strategy", k);
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

  // Setup topology
  NodeContainer nodes;
  nodes.Create (7);

  Config::SetDefault ("ns3::DropTailQueue::MaxPackets", StringValue ("40"));

  PointToPointHelper p2p;
  p2p.SetChannelAttribute("Delay", StringValue ("1ms"));
  p2p.SetDeviceAttribute("DataRate", StringValue ("100Mbps"));
  p2p.Install (nodes.Get (0), nodes.Get (2));
  p2p.Install (nodes.Get (1), nodes.Get (2));
  p2p.Install (nodes.Get (3), nodes.Get (5));
  p2p.Install (nodes.Get (4), nodes.Get (6));

  p2p.SetChannelAttribute("Delay", StringValue ("10ms"));
  p2p.SetDeviceAttribute("DataRate", StringValue ("10Mbps"));
  p2p.Install (nodes.Get (2), nodes.Get (3));
  p2p.Install (nodes.Get (2), nodes.Get (4));

  // Install CCNx stack on all nodes
  ndn::StackHelper ndnHelper;
  if (shaper != "None")
    {
      if (strategy == "CongestionAware")
        ndnHelper.SetForwardingStrategy ("ns3::ndn::fw::CongestionAware", "EnableNACKs", "true", "K", k);
      else
        ndnHelper.SetForwardingStrategy ("ns3::ndn::fw::BestRoute", "EnableNACKs", "true");

      ndnHelper.EnableShaper (true, 40, 0.97, Seconds(0.1), mode_enum);
    }
  else
    {
      ndnHelper.SetForwardingStrategy ("ns3::ndn::fw::BestRoute"); // No hop-by-hop interest shaping, no NACKs.
    }
  ndnHelper.SetContentStore ("ns3::ndn::cs::Nocache");
  ndnHelper.InstallAll ();

  // Getting containers for the consumer/producer
  Ptr<Node> c1 = nodes.Get (0);
  Ptr<Node> c2 = nodes.Get (1);
  Ptr<Node> p1 = nodes.Get (5);
  Ptr<Node> p2 = nodes.Get (6);

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

  consumerHelper->SetPrefix ("/prefix1");
  UniformVariable r (0.0, 5.0);
  consumerHelper->SetAttribute ("StartTime", TimeValue (Seconds (r.GetValue ())));
  consumerHelper->Install (c1);

  delete consumerHelper;

  // add bursty competing traffic
  std::ostringstream oss;
  double time_cur = 0.0;
  while (time_cur < 70.1)
    {
      time_cur += r.GetValue (0.0, 0.2);
      oss << time_cur << "s " << r.GetInteger (0, 40) << " ";
    }

  ndn::AppHelper consumerBatchesHelper ("ns3::ndn::ConsumerBatches");
  consumerBatchesHelper.SetPrefix ("/prefix2");
  consumerBatchesHelper.SetAttribute ("Batches", StringValue(oss.str()));
  consumerBatchesHelper.Install (c2);

  // Install producer
  ndn::AppHelper producerHelper ("ns3::ndn::Producer");
  producerHelper.SetAttribute ("PayloadSize", StringValue("1000"));

  producerHelper.SetPrefix ("/prefix1");
  producerHelper.Install (p1);
  producerHelper.SetPrefix ("/prefix1");
  producerHelper.Install (p2);

  producerHelper.SetPrefix ("/prefix2");
  producerHelper.Install (p2);

  // Manually add multipath routes
  ndn::StackHelper::AddRoute (c1, "/prefix1", nodes.Get (2), 1);
  ndn::StackHelper::AddRoute (nodes.Get (2), "/prefix1", nodes.Get (3), 1);
  ndn::StackHelper::AddRoute (nodes.Get (2), "/prefix1", nodes.Get (4), 1);
  ndn::StackHelper::AddRoute (nodes.Get (3), "/prefix1", p1, 1);
  ndn::StackHelper::AddRoute (nodes.Get (4), "/prefix1", p2, 1);

  ndn::StackHelper::AddRoute (c2, "/prefix2", nodes.Get (2), 1);
  ndn::StackHelper::AddRoute (nodes.Get (2), "/prefix2", nodes.Get (4), 1);
  ndn::StackHelper::AddRoute (nodes.Get (4), "/prefix2", p2, 1);

  Simulator::Stop (Seconds (70.1));

  boost::tuple< boost::shared_ptr<std::ostream>, std::list<Ptr<ndn::L3AggregateTracer> > >
    aggTracers = ndn::L3AggregateTracer::InstallAll (agg_trace, Seconds (10.0));

  boost::tuple< boost::shared_ptr<std::ostream>, std::list<Ptr<ndn::AppDelayTracer> > >
    delayTracers = ndn::AppDelayTracer::InstallAll (delay_trace);

  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}

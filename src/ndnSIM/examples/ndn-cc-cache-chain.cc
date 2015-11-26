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
#include "ns3/ndn-consumer.h"
#include <ns3/ndnSIM/utils/tracers/ndn-l3-aggregate-tracer.h>
#include <ns3/ndnSIM/utils/tracers/ndn-app-delay-tracer.h>
#include <ns3/ndnSIM/utils/tracers/ndn-cs-tracer.h>

using namespace ns3;

int
main (int argc, char *argv[])
{
  std::string consumer ("WindowRelentless"), shaper ("PIE");
  std::string num_content ("10000"), q ("0.0"), s ("0.75"), cache_size ("100"), replace ("LRU");
  std::string agg_trace ("aggregate-trace.txt"), delay_trace ("app-delays-trace.txt"), cs_trace ("cs-trace.txt"); 

  CommandLine cmd;
  cmd.AddValue("consumer", "Consumer type (AIMD/CUBIC/RAAQM/WindowRelentless/RateRelentless/RateFeedback)", consumer);
  cmd.AddValue("shaper", "Shaper mode (None/DropTail/PIE/CoDel)", shaper);
  cmd.AddValue("num_content", "Total number of contents", num_content);
  cmd.AddValue("q", "Parameter of improve rank for Zipf-Mandelbrot", q);
  cmd.AddValue("s", "Parameter of power for Zipf-Mandelbrot", s);
  cmd.AddValue("cache_size", "Cache size at intermediate routers", cache_size);
  cmd.AddValue("replace", "Cache replacement policy", replace);
  cmd.AddValue("agg_trace", "Aggregate trace file name", agg_trace);
  cmd.AddValue("delay_trace", "App delay trace file name", delay_trace);
  cmd.AddValue("cs_trace", "Content store trace file name", cs_trace);
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
  nodes.Create (4);

  Config::SetDefault ("ns3::DropTailQueue::MaxPackets", StringValue ("75"));

  PointToPointHelper p2p;
  p2p.SetChannelAttribute("Delay", StringValue ("10ms"));
  p2p.SetDeviceAttribute("DataRate", StringValue ("100Mbps"));
  p2p.Install (nodes.Get (0), nodes.Get (1));
  p2p.Install (nodes.Get (2), nodes.Get (3));
  p2p.SetDeviceAttribute("DataRate", StringValue ("10Mbps"));
  p2p.Install (nodes.Get (1), nodes.Get (2));

  // Install CCNx stack on all nodes
  ndn::StackHelper ndnHelper;

  if (shaper != "None")
    {
      ndnHelper.SetForwardingStrategy ("ns3::ndn::fw::BestRoute", "EnableNACKs", "true");
      ndnHelper.EnableShaper (true, 75, 0.97, Seconds(0.1), mode_enum);
    }
  else
    {
      ndnHelper.SetForwardingStrategy ("ns3::ndn::fw::BestRoute"); // No hop-by-hop interest shaping, no NACKs.
    }

  ndnHelper.SetContentStore ("ns3::ndn::cs::Nocache");
  ndnHelper.Install (nodes.Get (0));
  ndnHelper.Install (nodes.Get (3));

  if (cache_size != "0")
    {
      if (replace == "LRU")
        ndnHelper.SetContentStore ("ns3::ndn::cs::Lru", "MaxSize", cache_size);
      else if (replace == "LFU")
        ndnHelper.SetContentStore ("ns3::ndn::cs::Lfu", "MaxSize", cache_size);
    }
  ndnHelper.Install (nodes.Get (1));
  ndnHelper.Install (nodes.Get (2));

  // Installing global routing interface on all nodes
  ndn::GlobalRoutingHelper ndnGlobalRoutingHelper;
  ndnGlobalRoutingHelper.InstallAll ();

  // Getting containers for the consumer/producer
  Ptr<Node> c = nodes.Get (0);
  Ptr<Node> p = nodes.Get (3);

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

  consumerHelper->SetAttribute ("RequestMode", EnumValue (ndn::Consumer::ZIPF_MANDELBROT));
  consumerHelper->SetAttribute ("NumberOfContents", StringValue (num_content));
  consumerHelper->SetAttribute ("Q", StringValue (q));
  consumerHelper->SetAttribute ("S", StringValue (s));
  consumerHelper->SetAttribute ("LifeTime", TimeValue (Seconds (5.0)));

  consumerHelper->SetPrefix ("/p");
  UniformVariable r (0.0, 5.0);
  consumerHelper->SetAttribute ("StartTime", TimeValue (Seconds (r.GetValue ())));
  consumerHelper->Install (c);

  delete consumerHelper;

  // Register prefix with global routing controller and install producer
  ndn::AppHelper producerHelper ("ns3::ndn::Producer");
  producerHelper.SetAttribute ("PayloadSize", StringValue("1000"));

  ndnGlobalRoutingHelper.AddOrigins ("/p", p);
  producerHelper.SetPrefix ("/p");
  producerHelper.Install (p);

  // Calculate and install FIBs
  ndnGlobalRoutingHelper.CalculateRoutes ();

  Simulator::Stop (Seconds (70.1));

  boost::tuple< boost::shared_ptr<std::ostream>, std::list<Ptr<ndn::L3AggregateTracer> > >
    aggTracers = ndn::L3AggregateTracer::InstallAll (agg_trace, Seconds (10.0));

  boost::tuple< boost::shared_ptr<std::ostream>, std::list<Ptr<ndn::AppDelayTracer> > >
    delayTracers = ndn::AppDelayTracer::InstallAll (delay_trace);

  boost::tuple< boost::shared_ptr<std::ostream>, std::list<Ptr<ndn::CsTracer> > >
    csTracers = ndn::CsTracer::InstallAll (cs_trace, Seconds (10.0));

  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}

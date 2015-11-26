# ndnSIMQoS

This code is branched from: https://github.com/wygivan/ndnSIM (a version of ndnSIM with congestion control mechanisms).
we improved so it considers QoS with four priority levels.

Main changes are at:
  src/ndnSIM/models/ndn-interest, ndn-shaping-net-devices
  src/ndnSIM/helper/ndn-stack-helper
  src/ndnSIM/utils/tracers/ndn-l3-rate-tracer
  src/ndnSIM/apps/ndn-consumer
  
To run:
sudo LD_LIBRARY_PATH=/usr/local/lib NS_LOG=ndn.ShaperNetDeviceFace ./waf --run ndn-cc-baseline
or
sudo LD_LIBRARY_PATH=/usr/local/lib NS_LOG=ndn.ShaperNetDeviceFace ./waf --run ndn-cc-dumbbell

different scenarios are hardcoded in each file.

#include <iomanip>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/internet-module.h"
#include "ns3/flow-monitor-module.h"
#include <string>
#include <cmath>
#include <numeric>

NS_LOG_COMPONENT_DEFINE ("WifiContentionDelayTest");

using namespace ns3;

class Experiment
{
public:
  Experiment(uint32_t, double, double, double, uint32_t, uint32_t,
    bool, uint32_t, double,std::string);
  void Run();

private:
  uint32_t m_runIndex;
  double m_percTraffic;
  double m_trafficRate;
  double m_burstLength;
  uint32_t m_payloadSize;
  uint32_t m_aggSize;
  bool m_rtsCts;
  uint32_t m_clientSize;
  double m_simTime;
  std::string m_filename;

  std::vector<uint32_t> TrafficClassifier();
};

Experiment::Experiment(uint32_t runIndex, double percTraffic, double trafficRate,
  double burstLength, uint32_t payloadSize, uint32_t aggSize, bool rtsCts,
  uint32_t clientSize, double simTime, std::string filename)
: m_runIndex(runIndex), m_percTraffic(percTraffic), m_trafficRate(trafficRate),
m_burstLength(burstLength), m_payloadSize(payloadSize), m_aggSize(aggSize),
m_rtsCts(rtsCts), m_clientSize(clientSize), m_simTime(simTime), m_filename(filename)
{
}

void Experiment::Run()
{
  //Packet::EnablePrinting ();
  //Packet::EnableChecking ();

  std::string controlMode ("HtMcs0");
  std::string phyMode ("HtMcs5");
  NodeContainer wifiApNode;
  wifiApNode.Create (1);
  NodeContainer wifiStaNodes;
  wifiStaNodes.Create (m_clientSize);

  YansWifiPhyHelper phy = YansWifiPhyHelper::Default ();
  YansWifiChannelHelper channel;
  channel.AddPropagationLoss ("ns3::FriisPropagationLossModel");
  channel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
  phy.SetChannel (channel.Create ());
  phy.Set ("ShortGuardEnabled", BooleanValue (false));
  phy.Set ("ChannelWidth", UintegerValue (20));

  // Setting RTS-CTS
  if(m_rtsCts)
  {
    // disable fragmentation for frames below 2200 bytes
    Config::SetDefault ("ns3::WifiRemoteStationManager::FragmentationThreshold", StringValue ("0"));
    // turn off RTS/CTS for frames below 2200 bytes
    Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue ("0"));
    // Data Mode
  }
  else{
    // disable fragmentation for frames below 2200 bytes
    Config::SetDefault ("ns3::WifiRemoteStationManager::FragmentationThreshold", StringValue ("1000000"));
    // turn off RTS/CTS for frames below 2200 bytes
    Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue ("100000"));
    // Data Mode
  }

  //std::cout << "Set RTS/CTS .." << std::endl;

  WifiHelper wifi;
  wifi.SetStandard (WIFI_PHY_STANDARD_80211n_5GHZ);
  WifiMacHelper mac;

  Ssid ssid = Ssid ("ns380211n");

  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager","DataMode",
                                StringValue(phyMode), "ControlMode",
                                StringValue(controlMode));

  uint32_t aMpduSize = (m_aggSize > 7395)? m_aggSize:0;
  uint32_t aMsduSize = (m_aggSize >= 7395)? 7395: m_aggSize;

  NetDeviceContainer staDevices;
  NetDeviceContainer apDevice;
  mac.SetType ("ns3::StaWifiMac",
               "Ssid", SsidValue (ssid),
               "ActiveProbing", BooleanValue (false),
               "BE_MaxAmpduSize", UintegerValue (aMpduSize),
               "BE_MaxAmsduSize", UintegerValue (aMsduSize));
  staDevices = wifi.Install (phy, mac, wifiStaNodes);
  mac.SetType ("ns3::ApWifiMac",
               "Ssid", SsidValue (ssid));
  apDevice = wifi.Install (phy, mac, wifiApNode);

  //std::cout << "Assigned MAC .." << std::endl;

  // mobility.
  MobilityHelper mobility;
  Ptr<RandomDiscPositionAllocator> staAlloc = CreateObject<RandomDiscPositionAllocator>();
  staAlloc -> SetAttribute("Rho", StringValue("ns3::UniformRandomVariable[Min=0.0|Max=25.0]"));

  Ptr<ListPositionAllocator> apAlloc = CreateObject<ListPositionAllocator> ();
  apAlloc->Add (Vector (0.0, 0.0, 0.0));

  mobility.SetPositionAllocator (apAlloc);
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (wifiApNode);

  mobility.SetPositionAllocator (staAlloc);
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (wifiStaNodes);

  //std::cout << "Assigned Mobility .." << std::endl;

  InternetStackHelper stack;
  stack.Install(wifiApNode);
  stack.Install (wifiStaNodes);

  Ipv4AddressHelper address;
  address.SetBase ("10.0.0.0", "255.255.240.0");

  Ipv4InterfaceContainer staNodeInterfaces;
  Ipv4InterfaceContainer apNodeInterface;
  apNodeInterface = address.Assign(apDevice);
  staNodeInterfaces = address.Assign (staDevices);

  Ipv4Address apIpv4 = apNodeInterface.GetAddress(0);
    //std::cout << "AP's IPv4 address: "
    //<< apIpv4 << std::endl;

  //std::cout << "Assigned IP Addresses .." << std::endl;

  //UDP flow
  uint16_t socketPort = 9;
  Ptr<Socket> recvSink = Socket::CreateSocket (wifiApNode.Get (0), UdpSocketFactory::GetTypeId ());
  InetSocketAddress local = InetSocketAddress (apNodeInterface.GetAddress (0), socketPort);
  recvSink->Bind (local);

  //UdpServerHelper server (port);
  //ApplicationContainer serverApp = server.Install (wifiApNode.Get (0));
  //serverApp.Start (Seconds (0.0));
  //serverApp.Stop (Seconds (m_simTime + 1));

  std::vector<uint32_t> trafficClassifier = TrafficClassifier();

  PPBPHelper ppbp = PPBPHelper ("ns3::UdpSocketFactory",
                       InetSocketAddress (apNodeInterface.GetAddress (0),socketPort));
  ppbp.SetAttribute("MeanBurstArrivals",DoubleValue(m_trafficRate));
  ppbp.SetAttribute("MeanBurstTimeLength", DoubleValue(m_burstLength));
  ppbp.SetAttribute("PacketSize", UintegerValue(m_payloadSize));

  //std::cout << "Assigning Traffic Generation .." << std::endl;

  uint32_t count = 0;
  std::vector<ApplicationContainer> ppbpApps;
  for (uint32_t staIter = 0; staIter < m_clientSize; staIter++)
  {
    if(trafficClassifier[staIter] == 1)
    {
      ppbpApps.push_back(ppbp.Install (wifiStaNodes.Get (staIter)));
      ppbpApps[count].Start (MilliSeconds (1.0));
      ppbpApps[count].Stop (MilliSeconds (m_simTime + 1));
      count += 1;
    }
  }

  //std::cout << "Assigned PPBP Traffic Generation .." << std::endl;

  // Install FlowMonitor on all nodes
  FlowMonitorHelper flowmon;
  Ptr<FlowMonitor> monitor = flowmon.InstallAll ();

  //std::cout << "Beginning run .." << std::endl;

  Simulator::Stop(MilliSeconds(m_simTime+ 1));
  Simulator::Run ();

  //std::cout << "Completed run .." << std::endl;

  // 10. Print per flow statistics
  monitor->CheckForLostPackets ();
  Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier ());

  //std::cout << classifier << '\n';
  double delayAvg = 0;
  double aggTput = 0;
  double delaySum = 0;
  uint32_t rxPackets = 0;
  uint32_t rxBytes = 0;
  double timeFirstPacket = m_simTime + 1;
  double timeLastPacket = 0.0;

  FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats ();
  for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin (); i != stats.end (); ++i)
    {
      Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);

      if(t.sourceAddress != apIpv4)
      {
        delaySum +=  i->second.delaySum.GetSeconds();
        rxPackets += i->second.rxPackets;
        rxBytes += i->second.rxBytes;

        if(i->second.timeLastRxPacket.GetSeconds() > timeLastPacket)
        {
          timeLastPacket = i->second.timeLastRxPacket.GetSeconds();
        }

        if(i->second.timeFirstTxPacket.GetSeconds() < timeFirstPacket)
        {
          timeFirstPacket = i->second.timeFirstTxPacket.GetSeconds();
        }
        //std::cout << "  Throughput: " << i->second.rxBytes * 8.0 / (i->second.timeLastRxPacket.GetSeconds()-i->second.timeFirstTxPacket.GetSeconds()) / 1000 / 1000  << " Mbps\n";
      }

    }

    delayAvg = 1000*delaySum / rxPackets;
    aggTput = rxBytes*8.0 / (timeLastPacket- timeFirstPacket) / 1000 / 1000 ;

    std::ofstream myfile;
    myfile.open (m_filename,std::ios_base::app);
    myfile << m_runIndex
    << ", " << m_percTraffic
    << ", " << m_trafficRate
    << ", " << m_burstLength
    << ", " << m_payloadSize
    << ", " << m_aggSize
    << ", " << m_rtsCts
    << ", " << m_clientSize
    << ", " << delayAvg
    << ", " << aggTput << std::endl;
    myfile.close();

    Simulator::Destroy ();
}

std::vector<uint32_t> Experiment::TrafficClassifier()
{
  // 0 means low traffic for the client
  // 1 means high traffic for the client

  Ptr<UniformRandomVariable> randomSta = CreateObject<UniformRandomVariable> ();
  randomSta ->SetAttribute ("Min", DoubleValue (0));
  randomSta ->SetAttribute ("Max", DoubleValue (1));

  uint32_t index = 0;
  uint32_t found = 0;
  uint32_t maxHigh = std::ceil((double)m_clientSize*m_percTraffic);

  std::vector<uint32_t> trafficClass;

  for(uint32_t i = 0; i < m_clientSize; i++)
  {
    trafficClass.push_back(0);
  }

  while(found < maxHigh)
  {
    while(1)
    {
      if(trafficClass[index] == 0)
      {
        break;
      }
      else
      {
        index++;
        if(index == m_clientSize)
        {
          index = 0;
        }
      }
    }
    double odds = randomSta -> GetValue();
    //std::cout << odds << std::endl;
    if(odds <= m_percTraffic)
    {
      trafficClass[index] = 1;
      //std::cout << "Index = " << index << std::endl;
      found++;
    }
    else
    {
      index++;
      if(index == m_clientSize)
      {
        index = 0;
      }
    }
  }

  return trafficClass;
}


int
main (int argc, char *argv[])
{
  double simTime = 100; // MilliSeconds
  uint32_t Nruns = 100;
  Time::SetResolution(Time::US);

  std::vector<uint32_t> clientSizeVec {100};
  std::vector<double> trafficRateVec {10, 100};
  std::vector<double> burstLengthVec {0.01, 0.1};
  std::vector<uint32_t> payloadSizeVec {12, 100};
  std::vector<uint32_t> aggSizeVec {1500, 10000};
  std::vector<bool> rtsCtsVec {true, false};

  for(uint32_t run = 1; run <= Nruns; run++)
  {
    for(double percTraffic = 1.0; percTraffic <= 1; percTraffic += 0.5)
    {
      for(auto trafficRateIter = trafficRateVec.begin(); trafficRateIter != trafficRateVec.end(); trafficRateIter++) // metres
      {
        for(auto burstLengthIter = burstLengthVec.begin(); burstLengthIter != burstLengthVec.end(); burstLengthIter++)
        {
          for(auto payloadSizeIter = payloadSizeVec.begin(); payloadSizeIter != payloadSizeVec.end(); payloadSizeIter++) // microseconds
          {
            for(auto aggSizeIter = aggSizeVec.begin(); aggSizeIter != aggSizeVec.end(); aggSizeIter++) // microseconds
            {
              for(auto rtsCtsIter = rtsCtsVec.begin(); rtsCtsIter != rtsCtsVec.end(); rtsCtsIter++) // microseconds
              {
                for(auto clientSizeIter = clientSizeVec.begin(); clientSizeIter != clientSizeVec.end(); clientSizeIter++) // microseconds
                {
                  std::cout << "Run: " << run
                  << ", Perc traffic: " << percTraffic
                  << ", Traffic rate: " << *trafficRateIter
                  << ", Burst Length: " << *burstLengthIter
                  << ", Payload Size: " << *payloadSizeIter
                  << ", Aggregated Size Max: " << *aggSizeIter
                  << ", RTS/CTS: " << *rtsCtsIter
                  << ", Client size: " << *clientSizeIter << std::endl;

                  Experiment experiment(run,percTraffic, *trafficRateIter,
                    *burstLengthIter, *payloadSizeIter, *aggSizeIter, *rtsCtsIter,
                    *clientSizeIter, simTime, "wifi_sensors_test.txt");
                  experiment.Run();
                }
              }
            }
          }
        }
      }
    }
  }
  return 0;
}

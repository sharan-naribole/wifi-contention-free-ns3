#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "liscan-run.h"
#include <cmath>
#include "ns3/yans-wifi-channel.h"
#include "ns3/propagation-loss-model.h"
#include "ns3/propagation-delay-model.h"
#include "ns3/nist-error-rate-model.h"
#include "ns3/position-allocator.h"
#include "output.h"
#include <fstream>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("liscan-class-test");

/*
static void GetChannelNumbers(ApPhyNode* node)
{
  node->GetChannelNumbers();
}
*/

static void StartCFP(LiScanRun *cfp)
{
  /*
  std::cout << "Starting Contention Free Period at "
  << Simulator::Now().GetSeconds()
  << std::endl;
  */
  cfp->StartCFP();
}

static void StopCFP(LiScanRun *cfp)
{
  /*
  std::cout << "Stopping Contention Free Period at "
  << Simulator::Now().GetSeconds()
  << std::endl;
  */
  cfp->StopCFP();
}

static void OutputCFP(LiScanRun *cfp)
{
  Output output = cfp->OutputCFP();
  std::ofstream myfile;
  myfile.open ("example.txt",std::ios_base::app);
  //myfile << "Metrics: " << std::endl;
  myfile << "Delay (ms): " << output.m_delayMean << std::endl;
  myfile << "Throughput (Mbps): " << output.m_throughput << std::endl;
  myfile << "Overhead (ms): " << output.m_overhead << std::endl;
  myfile.close();
}

int
main (int argc, char *argv[])
{

  uint32_t nSta = 2; //Number of stationary nodes
  double errorRate = 0.3;
  double simTime = 100;

  Time::SetResolution(Time::US);

  RngSeedManager::SetSeed (4);  // Changes seed from default of 1 to 3
  RngSeedManager::SetRun (3);   // Changes run number from default of 1 to 7

  // Enable the packet printing through Packet::Print command.
  Packet::EnablePrinting ();
  WifiPhyStandard standard = WIFI_PHY_STANDARD_80211a;
  uint8_t downlinkChannelNumber = 1;
  uint8_t uplinkChannelNumber = 4;

  Ptr<YansWifiChannel> dlChannel = CreateObject<YansWifiChannel> ();
  dlChannel->SetPropagationDelayModel (CreateObject<ConstantSpeedPropagationDelayModel> ());

  Ptr<YansWifiChannel> ulChannel = CreateObject<YansWifiChannel> ();
  ulChannel->SetPropagationDelayModel (CreateObject<ConstantSpeedPropagationDelayModel> ());

  Ptr<LogDistancePropagationLossModel> log = CreateObject<LogDistancePropagationLossModel> ();
  dlChannel->SetPropagationLossModel (log);
  ulChannel->SetPropagationLossModel (log);
  Ptr<ErrorRateModel> error = CreateObject<NistErrorRateModel> ();

  LiscanApNode* apNode;
  apNode = new LiscanApNode(0, "OfdmRate18Mbps",0, Vector(0,0,0));
  apNode->PhyDownlinkSetup(standard, dlChannel, error,downlinkChannelNumber);
  apNode->PhyUplinkSetup(standard, ulChannel, error,uplinkChannelNumber, true);
  apNode->InterferenceDLSetup(0.1*errorRate);
  apNode->InterferenceULSetup(errorRate);

  Ptr<RandomDiscPositionAllocator> location = CreateObject<RandomDiscPositionAllocator>();
  location -> SetAttribute("Rho", StringValue("ns3::UniformRandomVariable[Min=0.0|Max=25.0]"));

  std::vector<LiscanStaNode*> staNodes;
  for(uint32_t i = 0; i < nSta; i++)
  {
    LiscanStaNode* temp;
    Vector loc = location -> GetNext();
    std::cout << loc << std::endl;
    temp = new LiscanStaNode(apNode, i+1, "OfdmRate54Mbps", 0, loc);
    temp->PhyDownlinkSetup(standard, dlChannel, error,downlinkChannelNumber);
    temp->PhyUplinkSetup(standard, ulChannel, error,uplinkChannelNumber, true);
    temp->m_ppbp->SetAttribute("MeanBurstArrivals",DoubleValue(100));
    temp->InterferenceDLSetup(0.01*errorRate);
    temp->InterferenceULSetup(errorRate);
    staNodes.push_back(temp);
  }

  //std::cout << "Run begins .." << std::endl;

  LiScanRun cfp(staNodes,apNode);

  Simulator::Schedule(MilliSeconds(10),&StartCFP, &cfp);
  Simulator::Schedule(MilliSeconds(simTime),&StopCFP, &cfp);
  Simulator::Schedule(MilliSeconds(simTime),&OutputCFP, &cfp);
  Simulator::Stop(MilliSeconds(simTime));
  Simulator::Run();
  Simulator::Destroy();

  return 0;
}

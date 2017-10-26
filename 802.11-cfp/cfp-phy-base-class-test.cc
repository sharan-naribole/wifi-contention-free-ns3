#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "cfp-phy-base.h"
#include <cmath>
#include "ns3/yans-wifi-channel.h"
#include "ns3/propagation-loss-model.h"
#include "ns3/propagation-delay-model.h"
#include "ns3/nist-error-rate-model.h"
#include "ns3/position-allocator.h"
#include "output.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("wifi-phy-cfp-class-test");

/*
static void GetChannelNumbers(ApPhyNode* node)
{
  node->GetChannelNumbers();
}
*/

static void StartCFP(CFPPhyBase *cfp)
{
  std::cout << "Starting Contention Free Period at "
  << Simulator::Now().GetSeconds()
  << std::endl;
  cfp->StartCFP();
}

static void StopCFP(CFPPhyBase *cfp)
{
  std::cout << "Stopping Contention Free Period at "
  << Simulator::Now().GetSeconds()
  << std::endl;
  cfp->StopCFP();
}

static void OutputCFP(CFPPhyBase *cfp)
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
  uint8_t downlinkChannelNumber = 7;
  uint8_t uplinkChannelNumber = 7;

  Ptr<YansWifiChannel> dlChannel = CreateObject<YansWifiChannel> ();
  dlChannel->SetPropagationDelayModel (CreateObject<ConstantSpeedPropagationDelayModel> ());

  Ptr<YansWifiChannel> ulChannel = CreateObject<YansWifiChannel> ();
  ulChannel->SetPropagationDelayModel (CreateObject<ConstantSpeedPropagationDelayModel> ());

  Ptr<LogDistancePropagationLossModel> log = CreateObject<LogDistancePropagationLossModel> ();
  dlChannel->SetPropagationLossModel (log);
  ulChannel->SetPropagationLossModel (log);
  Ptr<ErrorRateModel> error = CreateObject<NistErrorRateModel> ();

  ApPhyNode* apNode;
  apNode = new ApPhyNode(0, "OfdmRate18Mbps",0, Vector(0,0,0));
  apNode->PhyDownlinkSetup(standard, dlChannel, error,downlinkChannelNumber);
  apNode->PhyUplinkSetup(standard, ulChannel, error,uplinkChannelNumber, false);
  apNode->InterferenceDLSetup(errorRate);
  apNode->InterferenceULSetup(errorRate);

  Ptr<RandomDiscPositionAllocator> location = CreateObject<RandomDiscPositionAllocator>();
  location -> SetAttribute("Rho", StringValue("ns3::UniformRandomVariable[Min=0.0|Max=25.0]"));

  std::vector<StaPhyNode*> staNodes;
  for(uint32_t i = 0; i < nSta; i++)
  {
    StaPhyNode* temp;
    Vector loc = location -> GetNext();
    std::cout << loc << std::endl;
    temp = new StaPhyNode(apNode, i+1, "OfdmRate54Mbps", 0, loc);
    temp->PhyDownlinkSetup(standard, dlChannel, error,downlinkChannelNumber);
    temp->PhyUplinkSetup(standard, ulChannel, error,uplinkChannelNumber, false);
    temp->InterferenceDLSetup(errorRate);
    temp->InterferenceDLSetup(errorRate);
    temp->m_ppbp->SetAttribute("MeanBurstArrivals",DoubleValue(100));
    staNodes.push_back(temp);
  }

  std::cout << "Run begins .." << std::endl;

  CFPPhyBase cfp(staNodes,apNode);

  Simulator::Schedule(MilliSeconds(10),&StartCFP, &cfp);
  Simulator::Schedule(MilliSeconds(simTime),&StopCFP, &cfp);
  Simulator::Schedule(MilliSeconds(simTime),&OutputCFP, &cfp);
  Simulator::Stop(MilliSeconds(simTime));
  Simulator::Run();
  Simulator::Destroy();

  return 0;
}

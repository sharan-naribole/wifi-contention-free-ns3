#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "liscan-run.h"
#include <cmath>
#include "ns3/yans-wifi-channel.h"
#include "ns3/propagation-loss-model.h"
#include "ns3/propagation-delay-model.h"
#include "ns3/nist-error-rate-model.h"

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
  std::cout << "Starting Contention Free Period at "
  << Simulator::Now().GetSeconds()
  << std::endl;
  cfp->StartCFP();
}

static void StopCFP(LiScanRun *cfp)
{
  std::cout << "Stopping Contention Free Period at "
  << Simulator::Now().GetSeconds()
  << std::endl;
  cfp->StopCFP();
}

static void OutputCFP(LiScanRun *cfp)
{
  cfp->OutputCFP();
}

int
main (int argc, char *argv[])
{

  uint32_t nSta = 2; //Number of stationary nodes
  double errorRate = 0.02;
  double simTime = 25;

  Time::SetResolution(Time::US);

  RngSeedManager::SetSeed (4);  // Changes seed from default of 1 to 3
  RngSeedManager::SetRun (3);   // Changes run number from default of 1 to 7

  // Enable the packet printing through Packet::Print command.
  Packet::EnablePrinting ();
  double distance = 5;
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
  apNode = new LiscanApNode(0, "OfdmRate18Mbps",0, 0);
  apNode->PhyDownlinkSetup(standard, dlChannel, error,downlinkChannelNumber);
  apNode->PhyUplinkSetup(standard, ulChannel, error,uplinkChannelNumber, true);
  apNode->InterferenceSetup(errorRate);

  std::vector<LiscanStaNode*> staNodes;
  for(uint32_t i = 0; i < nSta; i++)
  {
    LiscanStaNode* temp;
    temp = new LiscanStaNode(apNode, i+1, "OfdmRate54Mbps", 0, distance*pow(-1,i));
    temp->PhyDownlinkSetup(standard, dlChannel, error,downlinkChannelNumber);
    temp->PhyUplinkSetup(standard, ulChannel, error,uplinkChannelNumber, true);
    temp->InterferenceSetup(errorRate);
    staNodes.push_back(temp);
  }

  std::cout << "Run begins .." << std::endl;

  LiScanRun cfp(staNodes,apNode);

  Simulator::Schedule(MilliSeconds(10),&StartCFP, &cfp);
  Simulator::Schedule(MilliSeconds(simTime),&StopCFP, &cfp);
  Simulator::Schedule(MilliSeconds(simTime),&OutputCFP, &cfp);
  Simulator::Stop(MilliSeconds(simTime));
  Simulator::Run();
  Simulator::Destroy();

  return 0;
}

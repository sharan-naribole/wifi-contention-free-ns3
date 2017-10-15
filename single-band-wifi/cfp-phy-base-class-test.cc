#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "cfp-phy-base.h"
#include <cmath>
#include "ns3/yans-wifi-channel.h"
#include "ns3/propagation-loss-model.h"
#include "ns3/propagation-delay-model.h"
#include "ns3/nist-error-rate-model.h"
#include "ns3/constant-position-mobility-model.h"
#include "ns3/random-variable-stream.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("wifi-phy-cfp-class-test");

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

int
main (int argc, char *argv[])
{

  uint32_t nSta = 2; //Number of stationary nodes

  Time::SetResolution(Time::US);

  RngSeedManager::SetSeed (4);  // Changes seed from default of 1 to 3
  RngSeedManager::SetRun (3);   // Changes run number from default of 1 to 7

  // Enable the packet printing through Packet::Print command.
  Packet::EnablePrinting ();

  nSta = 2;
  double distance = 5;
  WifiPhyStandard standard = WIFI_PHY_STANDARD_80211a;

  Ptr<YansWifiChannel> channel = CreateObject<YansWifiChannel> ();
  channel->SetPropagationDelayModel (CreateObject<ConstantSpeedPropagationDelayModel> ());

  Ptr<LogDistancePropagationLossModel> log = CreateObject<LogDistancePropagationLossModel> ();
  channel->SetPropagationLossModel (log);
  Ptr<ErrorRateModel> error = CreateObject<NistErrorRateModel> ();

  ApPhyNode* apNode;
  apNode = new ApPhyNode(0, "OfdmRate6Mbps",0, 0);
  apNode->PhySetup(standard, channel, error);

  std::vector<StaPhyNode*> staNodes;
  for(uint32_t i = 0; i < nSta; i++)
  {
    StaPhyNode* temp;
    temp = new StaPhyNode(apNode, i+1, "OfdmRate54Mbps", 0, distance*pow(-1,i));
    temp->PhySetup(standard, channel, error);
    staNodes.push_back(temp);
  }

  std::cout << "Run begins .." << std::endl;

  CFPPhyBase cfp(staNodes,apNode);

  Simulator::Schedule(Seconds(0.1),&StartCFP, &cfp);
  Simulator::Schedule(Seconds(1),&StopCFP, &cfp);
  Simulator::Stop(Seconds(1));
  Simulator::Run();
  Simulator::Destroy();

  return 0;
}

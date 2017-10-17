#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include <cmath>
#include "ns3/applications-module.h"
#include "ns3/yans-wifi-channel.h"
#include "ns3/propagation-loss-model.h"
#include "ns3/propagation-delay-model.h"
#include "ns3/nist-error-rate-model.h"
#include "ns3/constant-position-mobility-model.h"
#include "ns3/random-variable-stream.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("phy-trailer-message-test");

std::vector<PhyNode*> staNodes;

void TransmitPacket(std::string msg)
{
  staNodes[0]->Send(2, 1470,msg);
}

void ReceivePacket(Ptr<Packet> p, double snr, WifiTxVector txVector)
{

  std::cout << "Received at "
  << Simulator::Now().GetSeconds()
  << std::endl;

  std::cout << "Total Packet Size = "
  << p->GetSize() << std::endl;

  BasicHeader header;
  p->RemoveHeader (header);
  std::cout << "Header = "
  << header.GetData() << std::endl;

  std::cout << "Size after Header Removal = "
  << p->GetSize() << std::endl;

  /*
  BasicTrailer trailer;
  p->RemoveTrailer (trailer);
  std::cout << "Trailer = "
  << trailer.GetData() << std::endl;

  p->RemoveAtEnd(trailer.GetData());
  */

  uint8_t *buffer = new uint8_t[p->GetSize ()];
  p->CopyData(buffer, p->GetSize ());

  std::string msg = std::string((char*)buffer);
  std::cout<<"Received:"<< msg << std::endl;

  Simulator::Schedule(Seconds(1.0),&TransmitPacket,"Hello Sharan");
}

int
main (int argc, char *argv[])
{

  uint32_t nSta = 2; //Number of stationary nodes

  Time::SetResolution(Time::US);

  // Enable the packet printing through Packet::Print command.
  Packet::EnablePrinting ();

  double distance = 5;
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

  for(uint32_t i = 0; i < nSta; i++)
  {
    PhyNode* temp;
    temp = new PhyNode(i+1, "OfdmRate54Mbps", 0, distance*pow(-1,i));
    temp->PhyDownlinkSetup(standard, dlChannel, error,downlinkChannelNumber);
    temp->PhyUplinkSetup(standard, ulChannel, error,uplinkChannelNumber, false);
    temp -> m_dl -> SetReceiveOkCallback(MakeCallback(&ReceivePacket));
    staNodes.push_back(temp);
  }

  std::cout << "Run begins .." << std::endl;

  Simulator::Schedule(Seconds(0.1),&TransmitPacket, "Hello");
  Simulator::Stop(Seconds(2));
  Simulator::Run();
  Simulator::Destroy();

  return 0;
}

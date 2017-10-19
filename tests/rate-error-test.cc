#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include <cmath>
#include <iomanip>
#include "ns3/yans-wifi-channel.h"
#include "ns3/propagation-loss-model.h"
#include "ns3/propagation-delay-model.h"
#include "ns3/error-model.h"
#include "ns3/nist-error-rate-model.h"
#include "ns3/applications-module.h"

using namespace ns3;

class Experiment{
private:
  PhyNode* txNode;
  PhyNode* rxNode;
  uint32_t m_nPackets = 1000;
  uint32_t m_packetSize = 1470;
  uint32_t m_index;
  double m_errorRate;
  Ptr<RateErrorModel> m_rem = CreateObject<RateErrorModel> ();
  Ptr<UniformRandomVariable> m_uv = CreateObject<UniformRandomVariable> ();
  uint32_t g_RxSuccess = 0;
  uint32_t g_RxDrop = 0;

  void TransmitPacket(PhyNode* node, uint32_t packetCount);
  void ReceivePacket(Ptr<Packet> p, double snr, WifiTxVector txVector);

public:
  Experiment(uint32_t index, double rate);
  void Setup();
  void Run();
  void Output();
};

Experiment::Experiment(uint32_t index, double rate)
: m_index(index), m_errorRate(rate)
{
}

void Experiment::Setup()
{
  m_rem->SetRandomVariable (m_uv);
  m_rem->SetRate (m_errorRate);
  m_rem->SetAttribute("ErrorUnit",EnumValue(RateErrorModel::ERROR_UNIT_PACKET));

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

  rxNode = new PhyNode(0, "OfdmRate6Mbps",0, 0);
  rxNode->PhyDownlinkSetup(standard, dlChannel, error,downlinkChannelNumber);
  rxNode->PhyUplinkSetup(standard, ulChannel, error,uplinkChannelNumber, false);
  rxNode->m_dl->SetReceiveOkCallback(MakeCallback(&Experiment::ReceivePacket,this));

  txNode = new PhyNode(1, "OfdmRate54Mbps", 0, 10.0);
  txNode->PhyDownlinkSetup(standard, dlChannel, error,downlinkChannelNumber);
  txNode->PhyUplinkSetup(standard, ulChannel, error,uplinkChannelNumber, false);
}

void Experiment::Run()
{
  TransmitPacket(txNode,m_nPackets);
  Simulator::Stop(MicroSeconds(1000*(m_nPackets + 1)));
  Simulator::Run();
  Simulator::Destroy();
}

void Experiment::TransmitPacket(PhyNode* node, uint32_t packetCount)
{
  if(packetCount == 0)
  {
    Output();
  }

  node->Send(node->m_dl, 1, m_packetSize);

  Simulator::Schedule(MicroSeconds(1000),&Experiment::TransmitPacket,this, node,
                      packetCount-1);
}

void Experiment::ReceivePacket(Ptr<Packet> p, double snr, WifiTxVector txVector)
{

  if (m_rem->IsCorrupt (p))
    {
      g_RxDrop++;
    } else {
      g_RxSuccess++;
    }
}


void Experiment::Output()
{
  std::cout << m_index <<
    std::setw (12) << m_errorRate <<
    std::setw (12) << g_RxSuccess <<
    std::setw (12) << g_RxDrop <<
  std::endl;
}

int main (int argc, char *argv[])
{

  std::cout << "Index" <<
    std::setw (12) << "Error Rate" <<
    std::setw (12) << "Success" <<
    std::setw (12) << "Drop" <<
    std::endl;

  uint32_t index = 1;

  for(double rateError = 0; rateError < 1; rateError += 0.05)
  {
    Experiment experiment(index, rateError);
    experiment.Setup();
    experiment.Run();
    index++;
  }

  return 0;
}

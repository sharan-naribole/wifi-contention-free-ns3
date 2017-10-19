#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include <cmath>
#include <iomanip>
#include "ns3/yans-wifi-channel.h"
#include "ns3/propagation-loss-model.h"
#include "ns3/propagation-delay-model.h"
#include "ns3/nist-error-rate-model.h"
#include "ns3/applications-module.h"

class Experiment{
private:
  PhyNode* apNode;
  PhyNode* staNode;
  PhyNode* intfNode;
  uint32_t m_nPackets = 100;
  uint32_t m_packetSize = 1470;
  uint32_t m_index;
  double m_staLoc = 10;
  uint8_t m_txPowerLevel;
  double m_intfLoc;
  double m_delayOffset;

  uint32_t g_rxSta = 0;
  uint32_t g_rxIntf = 0;
  double g_signalSum = 0;;
  double g_noiseSum = 0;

  void TransmitPacket(PhyNode* node, uint32_t packetCount);
  void ReceivePacket(Ptr<Packet> p, double snr, WifiTxVector txVector);
  void MonitorSnifferRx (Ptr<const Packet> p,
                       uint16_t channelFreqMhz,
                       WifiTxVector txVector,
                       MpduInfo aMpdu,
                       SignalNoiseDbm signalNoise);

public:
  Experiment(uint32_t index, uint8_t txPowerLevel, double distance, double delayOffset);
  void Setup();
  void Run();
  void Output();


};

Experiment::Experiment(uint32_t index, uint8_t txPowerLevel, double distance, double delayOffset)
: m_index(index), m_txPowerLevel(txPowerLevel), m_intfLoc(distance), m_delayOffset(delayOffset)
{
}

void Experiment::Setup()
{
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

  apNode = new PhyNode(0, "OfdmRate6Mbps",0, 0);
  apNode->PhyDownlinkSetup(standard, dlChannel, error,downlinkChannelNumber);
  apNode->PhyUplinkSetup(standard, ulChannel, error,uplinkChannelNumber, false);
  apNode->m_dl->SetReceiveOkCallback(MakeCallback(&Experiment::ReceivePacket,this));
  apNode->m_dl->TraceConnectWithoutContext("MonitorSnifferRx", MakeCallback(&Experiment::MonitorSnifferRx,this));

  staNode = new PhyNode(1, "OfdmRate54Mbps", 0, m_staLoc);
  staNode->PhyDownlinkSetup(standard, dlChannel, error,downlinkChannelNumber);
  staNode->PhyUplinkSetup(standard, ulChannel, error,uplinkChannelNumber, false);

  intfNode = new PhyNode(1, "OfdmRate54Mbps", 0, m_intfLoc);
  intfNode->PhyDownlinkSetup(standard, dlChannel, error,downlinkChannelNumber);
  intfNode->PhyUplinkSetup(standard, ulChannel, error,uplinkChannelNumber, false);
}

void Experiment::Run()
{
  TransmitPacket(staNode,m_nPackets);
  Simulator::Schedule(MicroSeconds(m_delayOffset),&Experiment::TransmitPacket, this, intfNode,
                      m_nPackets);
  Simulator::Stop(MicroSeconds(1000*(m_nPackets + 1)));
  Simulator::Run();
  Simulator::Destroy();
}

void Experiment::TransmitPacket(PhyNode* node, uint32_t packetCount)
{
  if(node == staNode)
  {
    /*
    std::cout << "Count = "
    << m_nPackets - packetCount << std::endl;
    */
    if(packetCount == 0)
    {
      Output();
    }

    node->Send(node->m_dl, 1, m_packetSize);
  }
  else
  {
    node->Send(node->m_dl, 2, m_packetSize);
  }
  Simulator::Schedule(MicroSeconds(1000),&Experiment::TransmitPacket,this, node,
                      packetCount-1);
}


void Experiment::MonitorSnifferRx (Ptr<const Packet> p,
                     uint16_t channelFreqMhz,
                     WifiTxVector txVector,
                     MpduInfo aMpdu,
                     SignalNoiseDbm signalNoise)
{
  g_signalSum += signalNoise.signal;
  g_noiseSum += signalNoise.noise;
}

void Experiment::ReceivePacket(Ptr<Packet> p, double snr, WifiTxVector txVector)
{
  BasicHeader destinationHeader;
  p->RemoveHeader (destinationHeader);

  if(destinationHeader.GetData() == 1)
  {
    g_rxSta++;
  }
  else if (destinationHeader.GetData() == 2)
  {
    g_rxIntf++;
  }
}


void Experiment::Output()
{

  double g_signalAvg = g_signalSum/(g_rxSta + g_rxIntf);
  double g_noiseAvg = g_noiseSum/(g_rxSta + g_rxIntf);
  double g_SNR = g_signalAvg - g_noiseAvg;

  std::cout << m_index <<
    std::setw (12) << (uint32_t) m_txPowerLevel <<
    std::setw (15) << m_intfLoc <<
    std::setw (12) << m_delayOffset <<
    std::setw (12) << g_rxSta <<
    std::setw (12) << g_rxIntf <<
    std::setw (12) << g_signalAvg <<
    std::setw (12) << g_noiseAvg <<
    std::setw (10) << g_SNR <<
  std::endl;
}

int main (int argc, char *argv[])
{

  std::cout << "Index" <<
    std::setw (12) << "Power Level" <<
    std::setw (15) << "Distance" <<
    std::setw (12) << "Delay Offset" <<
    std::setw (12) << "Rxed-Sta" <<
    std::setw (12) << "Rxed-Intf" <<
    std::setw (12) << "Signal(dBm)" <<
    std::setw (12) << "Noise(dBm)" <<
    std::setw (10) << "SNR(dB)" <<
    std::endl;

  uint32_t index = 1;

  for(uint8_t txPowerLevel = 0; txPowerLevel < 10; txPowerLevel++)
  {
    for(double distance = -100; distance < 0; distance += 10) // metres
    {
      for(double delayOffset = 0; delayOffset < 100; delayOffset += 10.0) // microseconds
      {
        Experiment experiment(index, txPowerLevel, distance, delayOffset);
        experiment.Setup();
        experiment.Run();
        index++;
      }
    }
  }

  return 0;
}

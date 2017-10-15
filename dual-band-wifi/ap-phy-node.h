#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"


using namespace ns3;

class ApPhyNode: public PhyNode {

public:
  ApPhyNode();
  virtual ~ApPhyNode();
  ApPhyNode(uint32_t, std::string, uint8_t, double);
  void PhyDownlinkSetup(WifiPhyStandard, Ptr<YansWifiChannel>, Ptr<ErrorRateModel>,
                        uint8_t);
  void PhyUplinkSetup(WifiPhyStandard, Ptr<YansWifiChannel>, Ptr<ErrorRateModel>,
                   uint8_t, bool);
  void StartPolling(uint32_t staIndex, uint32_t nSta);
  void StopPolling();

private:
  uint32_t m_staIndex;
  uint32_t m_nSta;
  bool m_stop = false;
  bool m_receiving = false;
  Time m_apIdleWaitTime = MicroSeconds(30);
  uint32_t m_pollSize = 100;
  double m_datarate;

  void TransmitPollRequest();
  void ReceivePollReply(Ptr<Packet> p, double snr, WifiTxVector txVector);
  void TransmitNextPoll();
  void StartIdleTimer();
  void CheckIdle();

  friend class StaPhyNode;
};

ApPhyNode::ApPhyNode ()
{
  // we must provide a public default constructor,
  // implicit or explicit, but never private.
}

ApPhyNode::~ApPhyNode ()
{
}

ApPhyNode::ApPhyNode(uint32_t nodeId, std::string txMode, uint8_t txPowerLevel,
  double distance)
: PhyNode(nodeId, txMode, txPowerLevel, distance)
{
}

void ApPhyNode::PhyDownlinkSetup(WifiPhyStandard standard, Ptr<YansWifiChannel> channel,
                Ptr<ErrorRateModel> error, uint8_t channelNumber = 0)
{
  m_dlChannelNumber = channelNumber;
  m_dl->SetChannel(channel);
  m_dl->SetErrorRateModel (error);
  m_dl->SetChannelNumber(channelNumber);
  m_dl->SetMobility(m_position);

  m_dl->ConfigureStandard(standard);

  WifiMode mode = WifiMode (m_txMode);
  WifiTxVector txVector;
  txVector.SetTxPowerLevel (m_txPowerLevel);
  txVector.SetMode (mode);
  txVector.SetPreambleType (WIFI_PREAMBLE_LONG);

  m_datarate = mode.GetDataRate(txVector);
}

void ApPhyNode::PhyUplinkSetup(WifiPhyStandard standard, Ptr<YansWifiChannel> channel,
                Ptr<ErrorRateModel> error, uint8_t channelNumber = 0, bool multiband = false)
{
  if(multiband)
  {
    m_ul->ConfigureStandard(standard);
    m_ul->SetChannel(channel);
    m_ul->SetChannelNumber(channelNumber);
    m_ul->SetErrorRateModel (error);
    m_ul->SetMobility(m_position);
  }
  else
  {
    m_ulChannelNumber = m_ulChannelNumber;
    m_ul = m_dl;
  }

  m_ul -> SetReceiveOkCallback(MakeCallback(&ApPhyNode::ReceivePollReply,this));

}


void ApPhyNode::StartPolling(uint32_t staIndex, uint32_t nSta)
{
  m_staIndex = staIndex;
  m_nSta = nSta;
  m_stop = false;

  Simulator::Schedule(Seconds(0.0001), &ApPhyNode::TransmitPollRequest, this);
}

void ApPhyNode::StopPolling()
{
  m_stop = true;
}

void ApPhyNode::TransmitPollRequest()
{
  //std::cout << "Test" << std::endl;

  if(m_stop == false)
  {
    //std::cout << "Station = " << m_staIndex << " out of "
    //<< m_nSta << std::endl;

    m_receiving = false;
    Send(m_dl, m_staIndex, m_pollSize);
    //std::cout << "Transmitted poll request at "
    //<< Simulator::Now ().GetSeconds ()
    //<< std::endl;

    Time pollTxTime (MicroSeconds ((double)(m_pollSize* 8.0*1000000) /((double) m_datarate)));
    //std::cout << "Idle time = " << pollTxTime + m_apIdleWaitTime << std::endl;

    Simulator::Schedule(pollTxTime + (m_apIdleWaitTime) + MicroSeconds(0), &ApPhyNode::CheckIdle, this);

    if(m_staIndex == m_nSta)
     {
       m_staIndex = 1;
     }
     else
     {
       m_staIndex++;
     }
   }
}

void ApPhyNode::ReceivePollReply (Ptr<Packet> p, double snr, WifiTxVector txVector)
{
  BasicHeader destinationHeader;
  p->RemoveHeader (destinationHeader);

  if(destinationHeader.GetData() == 0)
  {
    std::cout << "Received poll reply by Node "
    << destinationHeader.GetData() << " of size "
    << p->GetSize() << " at "
    << Simulator::Now().GetSeconds()
    << std::endl;
    Simulator::Schedule(m_apIdleWaitTime,&ApPhyNode::TransmitPollRequest,this);
  }
}

void ApPhyNode::TransmitNextPoll()
{
  Simulator::Schedule(m_apIdleWaitTime, &ApPhyNode::TransmitPollRequest, this);
}

void ApPhyNode::CheckIdle()
{
  if(m_receiving == false)
  {
    TransmitPollRequest();
  }
}

void ApPhyNode::StartIdleTimer()
{
  Simulator::Schedule(m_apIdleWaitTime, &ApPhyNode::CheckIdle, this);
}

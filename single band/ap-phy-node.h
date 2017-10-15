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
  void PhySetup(WifiPhyStandard, Ptr<YansWifiChannel>, Ptr<ErrorRateModel>);
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

void ApPhyNode::PhySetup(WifiPhyStandard standard, Ptr<YansWifiChannel> channel,
              Ptr<ErrorRateModel> error)
{
  m_wifi->SetChannel(channel);
  m_wifi->SetErrorRateModel (error);

  Ptr<MobilityModel> position = CreateObject<ConstantPositionMobilityModel> ();
  position->SetPosition (Vector (m_distance, 0.0, 0.0));
  m_wifi->SetMobility(position);

  m_wifi->ConfigureStandard(standard);

  WifiMode mode = WifiMode (m_txMode);
  WifiTxVector txVector;
  txVector.SetTxPowerLevel (m_txPowerLevel);
  txVector.SetMode (mode);
  txVector.SetPreambleType (WIFI_PREAMBLE_LONG);

  m_datarate = mode.GetDataRate(txVector);

  m_wifi -> SetReceiveOkCallback(MakeCallback(&ApPhyNode::ReceivePollReply,this));
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
    Send(m_staIndex, m_pollSize);
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

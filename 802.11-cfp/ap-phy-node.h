#ifndef APPHYNODE_H
#define APPHYNODE_H

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"

using namespace ns3;

class ApPhyNode: public PhyNode {

public:
  ApPhyNode();
  virtual ~ApPhyNode();
  ApPhyNode(uint32_t, std::string, uint8_t, Vector);
  void PhyDownlinkSetup(WifiPhyStandard, Ptr<YansWifiChannel>, Ptr<ErrorRateModel>,
                        uint8_t);
  void PhyUplinkSetup(WifiPhyStandard, Ptr<YansWifiChannel>, Ptr<ErrorRateModel>,
                   uint8_t, bool);
  void StartPolling(uint32_t staIndex, uint32_t nSta);
  void StopPolling();
  double GetOverhead();
  double GetThroughput();

private:
  uint32_t m_staIndex;
  uint32_t m_prevIndex;
  uint32_t m_nSta;
  bool m_stop = false;
  bool m_receiving = false;
  bool m_apIdleTimerStart = false;

  double m_overhead = 0;
  uint32_t m_rxBytes = 0;
  double m_runTime;
  double m_throughput;

  Time m_startTime;
  Time m_stopTime;

  void TransmitPollRequest();
  void ReceivePollReply(Ptr<Packet> p, double snr, WifiTxVector txVector);
  void TransmitACK(std::string msg);
  void CheckIdle();
  void StartIdleTimer();
  void PhyRxBegin(Ptr< const Packet > packet);
  void PhyTxEnd(Ptr< const Packet > packet);
  void RxAbort();


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
  Vector loc)
: PhyNode(nodeId, txMode, txPowerLevel, loc)
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

  m_dl -> TraceConnectWithoutContext("PhyTxEnd", MakeCallback(&ApPhyNode::PhyTxEnd,this));
}

void ApPhyNode::PhyUplinkSetup(WifiPhyStandard standard, Ptr<YansWifiChannel> channel,
                Ptr<ErrorRateModel> error, uint8_t channelNumber = 0, bool multiband = false)
{
  if(multiband)
  {
    m_multiband = true;
    m_ul->ConfigureStandard(standard);
    m_ul->SetChannel(channel);
    m_ul->SetChannelNumber(channelNumber);
    m_ul->SetErrorRateModel (error);
    m_ul->SetMobility(m_position);
  }
  else
  {
    m_ulChannelNumber = m_dlChannelNumber;
    m_ul = m_dl;
  }

  m_ul -> SetReceiveOkCallback(MakeCallback(&ApPhyNode::ReceivePollReply,this));
  m_ul -> TraceConnectWithoutContext("PhyRxBegin", MakeCallback(&ApPhyNode::PhyRxBegin,this));
}

void ApPhyNode::StartPolling(uint32_t staIndex, uint32_t nSta)
{
  m_staIndex = staIndex;
  m_nSta = nSta;
  m_stop = false;

  m_startTime = Simulator::Now();

  Simulator::Schedule(Seconds(0.0001), &ApPhyNode::TransmitPollRequest, this);
}

void ApPhyNode::StopPolling()
{
  m_stop = true;
  m_stopTime = Simulator::Now();
  m_runTime = (m_stopTime - m_startTime).GetMicroSeconds();
  m_throughput = m_rxBytes*8/(m_runTime); // Mbps
}

void ApPhyNode::TransmitPollRequest()
{
  //std::cout << "Test" << std::endl;

  if(m_stop == false)
  {
    //std::cout << "Station = " << m_staIndex << " out of "
    //<< m_nSta << std::endl;

    m_receiving = false;
    Send(m_dl,m_staIndex, pollSize, "REQ"); // Poll Request
    /*
    std::cout << "Transmitting poll request to Node "
    << m_staIndex << " at " << Simulator::Now ().GetMicroSeconds ()
    << std::endl;
    */

    // Need to change this based on poll TX Time
    m_overhead += pollTxTime; // from Globals

    Simulator::Schedule(MicroSeconds(pollTxTime), &ApPhyNode::StartIdleTimer, this);

    m_prevIndex = m_staIndex;
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
  // At this stage, I will check the Interference Model
  BasicHeader destinationHeader;
  p->RemoveHeader (destinationHeader);

  if(destinationHeader.GetData() == 0)
  {
    /*
    std::cout << "Received poll reply by Node "
    << destinationHeader.GetData() << " of size "
    << p->GetSize() << " at "
    << Simulator::Now().GetMicroSeconds()
    << std::endl;
    */

    m_rxBytes += p->GetSize();

    Simulator::Schedule(MicroSeconds(SIFS),&ApPhyNode::TransmitACK,this, "ACK");
    m_overhead += (SIFS - decodeDelay); // after Packet reception, switching from
                                        // RX to TX
  }
}

void ApPhyNode::TransmitACK(std::string message)
{
  Send(m_dl,m_prevIndex, ACKSize, message);

  //std::cout << "Transmitted " << message << " at "
  //<< Simulator::Now ().GetMicroSeconds ()
  //<< std::endl;

  //Time ACKTxTime (MicroSeconds ((double)(ACKSize* 8.0*1000000) /((double) m_datarate)));
  //Simulator::Schedule(MicroSeconds((double)1.2*ACKTxTime),&ApPhyNode::TransmitPollRequest,this);
}

void ApPhyNode::CheckIdle()
{
  /*
  std::cout << "Checking idle at "
  << Simulator::Now().GetMicroSeconds() << std::endl;
  */
  if(m_receiving == false)
  {
    m_overhead += PIFS;
    //std::cout << "Calling TransmitPollRequest()" << std::endl;
    TransmitPollRequest();
  }
  else{
    m_overhead += SIFS;
  }
  m_apIdleTimerStart = false;
}

void ApPhyNode::StartIdleTimer()
{
  /*
  std::cout << "Started Idle Timer at "
  << Simulator::Now().GetMicroSeconds() << std::endl;
  */
  Simulator::Schedule(MicroSeconds(PIFS), &ApPhyNode::CheckIdle, this);
}

double ApPhyNode::GetThroughput()
{
  return m_throughput;
}

double ApPhyNode::GetOverhead()
{
  return m_overhead;
}

void ApPhyNode::PhyRxBegin(Ptr< const Packet > packet)
{
  if(m_rvUL->GetValue() < m_remUL->GetRate())
  {
    m_receiving = false;
    Simulator::Schedule(MicroSeconds(preambleDetection),&ApPhyNode::RxAbort,this);
  }
  else
  {
    //std::cout << "Started Poll Reply Reception at "
    //<< Simulator::Now().GetMicroSeconds() << std::endl;
    m_receiving = true;
  }

}

void ApPhyNode::PhyTxEnd(Ptr< const Packet > packet)
{
  //std::cout << "Poll Reply Transmission ends at "
  //<< Simulator::Now().GetMicroSeconds() << std::endl;
}

void ApPhyNode::RxAbort()
{
  //std::cout << "Aborted Poll Reply Reception at "
  //<< Simulator::Now().GetMicroSeconds() << std::endl;
  m_ul -> AbortCurrentReception();
}

#endif

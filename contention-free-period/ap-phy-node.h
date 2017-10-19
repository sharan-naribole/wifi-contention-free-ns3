#ifndef APPHYNODE_H
#define APPHYNODE_H

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"

using namespace ns3;

class ApPhyNode: public PhyNode {

public:
  double m_overhead = 0;
  uint32_t m_rxBytes = 0;
  Time m_startTime;
  Time m_stopTime;
  double m_runTime;
  double m_throughput;
  ApPhyNode();
  virtual ~ApPhyNode();
  ApPhyNode(uint32_t, std::string, uint8_t, double);
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
  uint32_t m_pollSize = 100;

  void TransmitPollRequest();
  void ReceivePollReply(Ptr<Packet> p, double snr, WifiTxVector txVector);
  void TransmitACK();
  void CheckIdle();
  void StartIdleTimer();

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

  //m_dl -> TraceConnectWithoutContext("PhyTxEnd", MakeCallback(&ApPhyNode::TxEnd,this));
  //m_dl -> TraceConnectWithoutContext("PhyTxBegin", MakeCallback(&ApPhyNode::TxEnd,this));
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
    Send(m_dl,m_staIndex, m_pollSize, "REQ"); // Poll Request
    //std::cout << "Transmitting poll request at "
    //<< Simulator::Now ().GetMicroSeconds ()
    //<< std::endl;

    Time pollTxTime (MicroSeconds ((double)(pollSize* 8.0*1000000) /((double) m_datarate)));

    m_overhead += pollTxTime.GetMicroSeconds();
    //std::cout << "Waiting time = " << pollTxTime + (MicroSeconds(PIFS))
    //<< std::endl;

    //Simulator::Schedule(pollTxTime + (MicroSeconds(PIFS)) + MicroSeconds(0), &ApPhyNode::CheckIdle, this);

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

    Simulator::Schedule(MicroSeconds(SIFS),&ApPhyNode::TransmitACK,this);
    //Simulator::Schedule(MicroSeconds(PIFS),&ApPhyNode::TransmitPollRequest,this);
  }
}

void ApPhyNode::TransmitACK()
{
  Send(m_dl,m_prevIndex, ACKSize, "ACK");
  /*
  std::cout << "Transmitted ACK at "
  << Simulator::Now ().GetMicroSeconds ()
  << std::endl;
  */
  //Time ACKTxTime (MicroSeconds ((double)(ACKSize* 8.0*1000000) /((double) m_datarate)));
  //Simulator::Schedule(MicroSeconds((double)1.2*ACKTxTime),&ApPhyNode::TransmitPollRequest,this);
}

void ApPhyNode::CheckIdle()
{
  if(m_receiving == false)
  {
    m_overhead += PIFS;
    TransmitPollRequest();
  }
  else{
    m_overhead += SIFS;
  }
  m_apIdleTimerStart = false;
}

void ApPhyNode::StartIdleTimer()
{
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


#endif

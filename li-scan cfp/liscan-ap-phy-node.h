#ifndef LISCANLiscanApNode_H
#define LISCANLiscanApNode_H

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"

using namespace ns3;

class LiscanApNode: public PhyNode {

public:
  LiscanApNode();
  virtual ~LiscanApNode();
  LiscanApNode(uint32_t, std::string, uint8_t, Vector);
  void PhyDownlinkSetup(WifiPhyStandard, Ptr<YansWifiChannel>, Ptr<ErrorRateModel>,
                        uint8_t);
  void PhyUplinkSetup(WifiPhyStandard, Ptr<YansWifiChannel>, Ptr<ErrorRateModel>,
                   uint8_t, bool);
  void StartPolling(uint32_t staIndex, uint32_t nSta);
  void StopPolling();
  double GetOverhead();
  double GetThroughput();
  uint32_t GetRxBytes();

private:
  uint32_t m_staIndex;
  uint32_t m_prevIndex;
  uint32_t m_ackId;
  uint32_t m_nSta;
  bool m_stop = false;
  bool m_receiving = false;
  bool m_apIdleTimerStart = false;
  bool m_waitingACK = false;

  double m_overhead = -1*pollTxTime;
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
  void PhyRxEnd(Ptr< const Packet > packet);
  void PhyRxDrop(Ptr< const Packet > packet);
  void ReceptionState();
  void CheckBusy();
  //void TriggerNext(std::string msg);

  friend class LiscanStaNode;
};

LiscanApNode::LiscanApNode ()
{
  // we must provide a public default constructor,
  // implicit or explicit, but never private.
}

LiscanApNode::~LiscanApNode ()
{
}

LiscanApNode::LiscanApNode(uint32_t nodeId, std::string txMode, uint8_t txPowerLevel,
  Vector loc)
: PhyNode(nodeId, txMode, txPowerLevel, loc)
{
}

void LiscanApNode::PhyDownlinkSetup(WifiPhyStandard standard, Ptr<YansWifiChannel> channel,
                Ptr<ErrorRateModel> error, uint8_t channelNumber = 0)
{
  m_dlChannelNumber = channelNumber;
  m_dl->SetChannel(channel);
  m_dl->SetErrorRateModel (error);
  m_dl->SetChannelNumber(channelNumber);
  m_dl->SetMobility(m_position);

  m_dl->ConfigureStandard(standard);

  //m_dl -> TraceConnectWithoutContext("PhyTxEnd", MakeCallback(&LiscanApNode::TxEnd,this));
  //m_dl -> TraceConnectWithoutContext("PhyTxBegin", MakeCallback(&LiscanApNode::TxEnd,this));
}

void LiscanApNode::PhyUplinkSetup(WifiPhyStandard standard, Ptr<YansWifiChannel> channel,
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

  m_ul -> SetReceiveOkCallback(MakeCallback(&LiscanApNode::ReceivePollReply,this));
  m_ul -> TraceConnectWithoutContext("PhyRxBegin", MakeCallback(&LiscanApNode::PhyRxBegin,this));
  m_ul -> TraceConnectWithoutContext("PhyRxEnd", MakeCallback(&LiscanApNode::PhyRxEnd,this));
  m_ul -> TraceConnectWithoutContext("PhyRxDrop", MakeCallback(&LiscanApNode::PhyRxDrop,this));
}

void LiscanApNode::StartPolling(uint32_t staIndex, uint32_t nSta)
{
  m_staIndex = staIndex;
  m_nSta = nSta;
  m_stop = false;

  m_startTime = Simulator::Now();

  Simulator::Schedule(Seconds(0.0001), &LiscanApNode::TransmitPollRequest, this);
}

void LiscanApNode::StopPolling()
{
  m_stop = true;
  m_stopTime = Simulator::Now();
  m_runTime = (m_stopTime - m_startTime).GetMicroSeconds();
  m_throughput = m_rxBytes*8/(m_runTime); // Mbps
}

void LiscanApNode::TransmitPollRequest()
{
  //std::cout << "Test" << std::endl;
  if(m_waitingACK == true)
  {
    TransmitACK("ACK");
  }

  else if(m_stop == false && m_receiving == false
    && m_waitingACK == false && m_dl->IsStateTx() == false)
  {
    //std::cout << "Station = " << m_staIndex << " out of "
    //<< m_nSta << std::endl;


    Send(m_dl,m_staIndex, pollSize, "REQ"); // Poll Request


    //std::cout << "Transmitting poll request to Node "
    //<< m_staIndex << " at " << Simulator::Now ().GetMicroSeconds ()
    //<< std::endl;

    m_prevIndex = m_staIndex;
    if(m_staIndex == m_nSta)
     {
       m_staIndex = 1;
     }
     else
     {
       m_staIndex++;
     }

     Simulator::Schedule(MicroSeconds(pollTxTime + 1), &LiscanApNode::TransmitPollRequest, this);
   }
}

void LiscanApNode::ReceivePollReply (Ptr<Packet> p, double snr, WifiTxVector txVector)
{
  // At this stage, I will check the Interference Model
  if(m_remUL->IsCorrupt (p) == false)
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

      Simulator::Schedule(MicroSeconds(decodeDelay),&LiscanApNode::TransmitACK,this, "ACK");
    }
  }
  else
  {
    /*
    std::cout << "Received data corrupted by channel at "
    << Simulator::Now().GetMicroSeconds() << std::endl;
    */
    //std::cout << "TX state: " << m_dl -> IsStateTx() << std::endl;
    TransmitPollRequest();
    //Simulator::Schedule(MicroSeconds(decodeDelay),&LiscanApNode::TransmitPollRequest,this);
    //m_overhead += ACKTxTime;
  }
}

/*
void LiscanApNode::TriggerNext(std::string msg)
{
  std::cout << "Transmitting Trigger packet to Node "
  << m_prevIndex << " at " << Simulator::Now ().GetMicroSeconds ()
  << std::endl;
  Send(m_dl,m_prevIndex, triggerSize, "NXT");
  if(msg == "ACK")
  {
    Simulator::Schedule(MicroSeconds(decodeDelay + triggerTime),&LiscanApNode::TransmitACK,this, "ACK");
  }
  else if(msg == "DROP")
  {
    Simulator::Schedule(MicroSeconds(triggerTime),&LiscanApNode::TransmitPollRequest,this);
  }
}
*/

void LiscanApNode::TransmitACK(std::string message)
{
  //std::cout << "Transmitting " << message << " at "
  //<< Simulator::Now ().GetMicroSeconds ()
  //<< std::endl;
  //std::cout << "TX status: " << m_dl -> IsStateTx() << std::endl;
  //std::cout << "Switch status: " << m_dl -> IsStateSwitching() << std::endl;
  //std::cout << "ACK waiting status: " << m_waitingACK
  //<< " at " << Simulator::Now().GetMicroSeconds() << std::endl;
  if(m_dl -> IsStateTx())
  {
    m_waitingACK = true;
    //std::cout << "Updated ACK waiting status: " << m_waitingACK
    //<< " at " << Simulator::Now().GetMicroSeconds() << std::endl;
    //Simulator::Schedule(MicroSeconds(2), &LiscanApNode::TransmitACK, this, "ACK");
  }
  else
  {
    m_waitingACK = false;
    //std::cout << "Updated ACK waiting status: " << m_waitingACK
    //<< " at " << Simulator::Now().GetMicroSeconds() << std::endl;
    //std::cout << "Transmitting ACK at "
    //<< Simulator::Now().GetMicroSeconds() << std::endl;
    Send(m_dl,m_ackId, ACKSize, message);
    Simulator::Schedule(MicroSeconds(ACKTxTime + 1),&LiscanApNode::TransmitPollRequest,this);
  }
  //Time ACKTxTime (MicroSeconds ((double)(ACKSize* 8.0*1000000) /((double) m_datarate)));
  //Simulator::Schedule(MicroSeconds((double)1.2*ACKTxTime),&LiscanApNode::TransmitPollRequest,this);
}

double LiscanApNode::GetThroughput()
{
  return m_throughput;
}

double LiscanApNode::GetOverhead()
{
  return m_overhead;
}

uint32_t LiscanApNode::GetRxBytes()
{
  return m_rxBytes;
}

void LiscanApNode::PhyRxBegin(Ptr< const Packet > packet)
{
  //std::cout << "Started Poll Reply Reception at "
  //<< Simulator::Now().GetMicroSeconds() << std::endl;

  //std::cout << packet ->GetSize() << std::endl;

  Simulator::Schedule(MicroSeconds(3), &LiscanApNode::ReceptionState,this);
}

void LiscanApNode::ReceptionState()
{
  m_receiving = true;
}

void LiscanApNode::PhyRxEnd(Ptr< const Packet > packet)
{
  //std::cout << "Ended Poll Reply Reception at "
  //<< Simulator::Now().GetMicroSeconds() << std::endl;

  m_receiving = false;
}

void LiscanApNode::PhyRxDrop(Ptr< const Packet > packet)
{
  //std::cout << "Poll Reply Dropped by channel error at "
  //<< Simulator::Now().GetMicroSeconds() << std::endl;
  CheckBusy();
  //std::cout << m_ul -> IsStateCcaBusy() << std::endl;
  //std::cout << m_ul -> IsStateBusy() << std::endl;
  //std::cout << m_ul -> IsStateIdle() << std::endl;
}

void LiscanApNode::CheckBusy()
{
  if(m_ul -> IsStateBusy())
  {
    Simulator::Schedule(MicroSeconds(5), &LiscanApNode::CheckBusy, this);
  }
  else
  {
    if(m_ul -> IsStateIdle())
    {
      m_receiving = false;
      if(m_dl -> IsStateTx() == false)
      {
        //std::cout << "Calling TransmitPollRequest after drop at: "
        //<< Simulator::Now().GetMicroSeconds() << std::endl;
        TransmitPollRequest();
      }
    }
    else
    {
      Simulator::Schedule(MicroSeconds(5), &LiscanApNode::CheckBusy, this);
    }
  }
}


#endif

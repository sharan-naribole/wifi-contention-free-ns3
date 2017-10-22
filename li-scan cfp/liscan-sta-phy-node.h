#ifndef LiscanStaNode_H
#define LiscanStaNode_H

#include "liscan-ap-phy-node.h"

using namespace ns3;

class LiscanStaNode: public PhyNode {

private:
  LiscanApNode* m_apNode;
  Ptr<PPBPQueue> m_ppbp = CreateObject<PPBPQueue>();
  std::queue<Ptr<Packet>> m_reTxQueue;
  double m_delaySum = 0;
  uint32_t m_NPacketsTX = 0;
  bool m_waitingACK = false;
  bool m_nextTx = false;

  void PacketQueuePop();
  void ReceivePacket (Ptr<Packet> p, double snr, WifiTxVector txVector);
  void CheckACKRx();
  void PhyRxEnd(Ptr< const Packet > packet);

public:
  LiscanStaNode(LiscanApNode*, uint32_t, std::string, uint8_t, double);
  void PhyDownlinkSetup(WifiPhyStandard, Ptr<YansWifiChannel>, Ptr<ErrorRateModel>,
                        uint8_t);
  void PhyUplinkSetup(WifiPhyStandard, Ptr<YansWifiChannel>, Ptr<ErrorRateModel>,
                   uint8_t, bool);
  void StartTraffic();
  void StopTraffic();
  double GetDelaySum();
  uint32_t GetNPacketsTX();
};

LiscanStaNode::LiscanStaNode(LiscanApNode* apNode, uint32_t nodeId, std::string txMode,
                        uint8_t txPowerLevel, double distance)
: PhyNode(nodeId, txMode, txPowerLevel, distance), m_apNode(apNode)
{
  //m_ppbp = new PPBPGenerator(nodeId);
  m_ppbp -> SetNodeId(nodeId);
}

void LiscanStaNode::PhyDownlinkSetup(WifiPhyStandard standard, Ptr<YansWifiChannel> channel,
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

  m_dl -> SetReceiveOkCallback(MakeCallback(&LiscanStaNode::ReceivePacket,this));
  //m_dl -> TraceConnectWithoutContext("PhyRxEnd", MakeCallback(&LiscanStaNode::PhyRxEnd,this));
}

void LiscanStaNode::PhyUplinkSetup(WifiPhyStandard standard, Ptr<YansWifiChannel> channel,
                Ptr<ErrorRateModel> error, uint8_t channelNumber = 0, bool multiband = false)
{
  if(multiband)
  {
    m_ulChannelNumber = channelNumber;
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
}

void LiscanStaNode::StartTraffic()
{
  m_ppbp -> StartPPBP();
}

void LiscanStaNode::StopTraffic()
{
  m_ppbp->StopPPBP();
}

void LiscanStaNode::PacketQueuePop()
{
   uint32_t Npackets = m_ppbp->m_packetQueue.size();

    if(Npackets > 0)
    {
      m_reTxQueue = m_ppbp->m_packetQueue;
    //Transmitting Poll reply

    /*
    std::cout << "Transmitting "
    << Npackets << " packets by Node "
    << m_nodeId<< " at "
    << Simulator::Now ().GetMicroSeconds ()
    << std::endl;
    */

    //m_apNode -> m_receiving = true;
    //std::cout << "AP's RX status changed to true at "
    //<< Simulator::Now().GetMicroSeconds() << std::endl;

      uint32_t aggPktSize = 0;
      //std::cout << "Queue size = "
      //<< currQueue.size() << std::endl;

      for(uint32_t i = 0; i< m_reTxQueue.size(); i++)
      {
        aggPktSize += m_ppbp->m_packetQueue.front()->GetSize();
        m_NPacketsTX += 1;
        m_delaySum += (Simulator::Now() - m_ppbp->m_packetGenTime.front()).GetMicroSeconds();

        m_ppbp->m_packetQueue.pop();
        m_ppbp->m_packetGenTime.pop();
      }

      Send(m_ul, 0,aggPktSize);
      m_waitingACK = true;
      m_apNode -> m_ackId = m_nodeId;
      std::cout << "Started Poll Reply Transmission at "
      << Simulator::Now().GetMicroSeconds()
      << " by Node " << m_nodeId << std::endl;
    }

    // Need to begin timer for ACK reception
    // If no ACK, retransmission queue won't be cleared
    // Those packets will be pushed back into main queue
    // with new generated Timestamps
    //Time pollReplyTxTime (MicroSeconds ((double)(aggPktSize* 8.0*1000000) /((double) m_datarate)));
    //Simulator::Schedule(pollReplyTxTime + (MicroSeconds(SIFS + ACKTimeout)), &StaPhyNode::CheckACKRx, this);
}

void LiscanStaNode::ReceivePacket(Ptr<Packet> p, double snr, WifiTxVector txVector)
{
  BasicHeader destinationHeader;
  p->RemoveHeader (destinationHeader);

  uint8_t *buffer = new uint8_t[p->GetSize ()];
  p->CopyData(buffer, p->GetSize ());

  std::string msg = std::string((char*)buffer);


  std::cout << "Node ID: " << m_nodeId
  << " RX header = " << destinationHeader.GetData()
  << " message = " << msg
  << " at "
  << Simulator::Now ().GetMicroSeconds ()
  << std::endl;


  if(destinationHeader.GetData() != m_nodeId)
  {
    if(m_waitingACK == true)
    {
      m_ppbp -> PushQueue(m_reTxQueue);
      //std::cout << "NACK received" << std::endl;
      // Clearing the retransmission queue on receiving ACK
      std::queue<Ptr<Packet>> temp;
      temp.swap(m_reTxQueue);
      m_waitingACK = false;
    }
    else if(msg == "ACK" && m_nextTx == true)
    {
      PacketQueuePop();
    }
  }
  else
  {
    //We know the packet is for this Station
    //We have to figure out if it is a Poll request
    //or an ACK frame. Accordingly, the packet queue
    // will pop the recently transmitted packets or not

    //std::cout<<"Received: "<< msg << std::endl;
    if(msg == "ACK")
    {
      // Clearing the retransmission queue on receiving ACK
      std::queue<Ptr<Packet>> temp;
      temp.swap(m_reTxQueue);
    }
    else if(msg == "REQ")
    {
      m_nextTx = false;
      if(m_rem->IsCorrupt (p) == false)
      {
        std::cout << "REQ received SUCCESS" << std::endl;
        if(m_apNode->m_receiving == true)
        {
          m_nextTx = true;
          std::cout << "Next in line: Node " << m_nodeId
          << std::endl;
        }
        else{
          PacketQueuePop();
        }
      }
    }
    //std::cout << "AP Rx status = " << m_apNode->m_receiving << std::end
    m_apNode -> TransmitPollRequest();
  }
}

void LiscanStaNode::PhyRxEnd(Ptr< const Packet > packet)
{
  if(m_apNode->m_apIdleTimerStart == false)
  {
    m_apNode->m_apIdleTimerStart = true;
    /*
    std::cout << "RX ends at "
    << Simulator::Now().GetMicroSeconds()
    << std::endl;
    */
    m_apNode -> StartIdleTimer();
  }
}

double LiscanStaNode::GetDelaySum()
{
  return m_delaySum;
}

uint32_t LiscanStaNode::GetNPacketsTX()
{
  return m_NPacketsTX;
}

#endif

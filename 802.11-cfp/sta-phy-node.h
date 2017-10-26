#ifndef STAPHYNODE_H
#define STAPHYNODE_H

#include "ap-phy-node.h"

using namespace ns3;

class StaPhyNode: public PhyNode {

private:
  ApPhyNode* m_apNode;
  std::queue<Ptr<Packet>> m_reTxQueue;
  double m_delaySum = 0;
  uint32_t m_NPacketsTX = 0;
  bool m_waitingACK = false;

  void PacketQueuePop();
  void ReceivePacket (Ptr<Packet> p, double snr, WifiTxVector txVector);
  void CheckACKRx();
  void PhyRxEnd(Ptr< const Packet > packet);

public:
  Ptr<PPBPQueue> m_ppbp = CreateObject<PPBPQueue>();
  StaPhyNode(ApPhyNode*, uint32_t, std::string, uint8_t, Vector);
  void PhyDownlinkSetup(WifiPhyStandard, Ptr<YansWifiChannel>, Ptr<ErrorRateModel>,
                        uint8_t);
  void PhyUplinkSetup(WifiPhyStandard, Ptr<YansWifiChannel>, Ptr<ErrorRateModel>,
                   uint8_t, bool);
  void StartTraffic();
  void StopTraffic();
  double GetDelaySum();
  uint32_t GetNPacketsTX();

};

StaPhyNode::StaPhyNode(ApPhyNode* apNode, uint32_t nodeId, std::string txMode,
                        uint8_t txPowerLevel, Vector loc)
: PhyNode(nodeId, txMode, txPowerLevel, loc), m_apNode(apNode)
{
  //m_ppbp = new PPBPGenerator(nodeId);
  m_ppbp -> SetNodeId(nodeId);
}

void StaPhyNode::PhyDownlinkSetup(WifiPhyStandard standard, Ptr<YansWifiChannel> channel,
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

  m_dl -> SetReceiveOkCallback(MakeCallback(&StaPhyNode::ReceivePacket,this));
  //m_dl -> TraceConnectWithoutContext("PhyRxEnd", MakeCallback(&StaPhyNode::PhyRxEnd,this));
}

void StaPhyNode::PhyUplinkSetup(WifiPhyStandard standard, Ptr<YansWifiChannel> channel,
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

void StaPhyNode::StartTraffic()
{
  m_ppbp -> StartPPBP();
}

void StaPhyNode::StopTraffic()
{
  m_ppbp->StopPPBP();
}

void StaPhyNode::PacketQueuePop()
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
      //std::cout << "Started Poll Reply Transmission at "
      //<< Simulator::Now().GetMicroSeconds() << std::endl;
    }

    // Need to begin timer for ACK reception
    // If no ACK, retransmission queue won't be cleared
    // Those packets will be pushed back into main queue
    // with new generated Timestamps
    //Time pollReplyTxTime (MicroSeconds ((double)(aggPktSize* 8.0*1000000) /((double) m_datarate)));
    //Simulator::Schedule(pollReplyTxTime + (MicroSeconds(SIFS + ACKTimeout)), &StaPhyNode::CheckACKRx, this);
}

void StaPhyNode::ReceivePacket(Ptr<Packet> p, double snr, WifiTxVector txVector)
{
  BasicHeader destinationHeader;
  p->RemoveHeader (destinationHeader);

  uint8_t *buffer = new uint8_t[p->GetSize ()];
  p->CopyData(buffer, p->GetSize ());

  std::string msg = std::string((char*)buffer);

  /*
  std::cout << "Node ID: " << m_nodeId
  << " RX header = " << destinationHeader.GetData()
  << " msg = "
  << msg << " at "
  << Simulator::Now ().GetMicroSeconds ()
  << std::endl;
  */

  if(m_waitingACK == true && destinationHeader.GetData() != m_nodeId)
  {
    m_ppbp -> PushQueue(m_reTxQueue);
    //std::cout << "NACK received" << std::endl;
    // Clearing the retransmission queue on receiving ACK
    std::queue<Ptr<Packet>> temp;
    temp.swap(m_reTxQueue);
    m_waitingACK = false;
  }

  // Checking whether packet is intended for the Station
  if(destinationHeader.GetData() == m_nodeId)
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
      m_apNode->TransmitPollRequest();
    }
    else if( msg == "REQ")
    {
      // Works only if Poll Request is 100% successfully Transmitted
      // Otherwise Polling completely stops
       //m_apNode -> StartIdleTimer();
       //Extracting WiFi Socket

       // At this stage, I will check the Interference Model
       if(m_remDL->IsCorrupt (p) == false)
         {
           PacketQueuePop();
         }
       else
       {
         //std::cout << "Poll request Dropped" << std::endl;
       }
    }
  }
}

void StaPhyNode::PhyRxEnd(Ptr< const Packet > packet)
{
  if(m_apNode->m_apIdleTimerStart == false)
  {
    m_apNode->m_apIdleTimerStart = true;
    /*
    std::cout << "RX ends at "
    << Simulator::Now().GetMicroSeconds()
    << std::endl;
    */
    //m_apNode -> StartIdleTimer();
  }
}

double StaPhyNode::GetDelaySum()
{
  return m_delaySum;
}

uint32_t StaPhyNode::GetNPacketsTX()
{
  return m_NPacketsTX;
}

#endif

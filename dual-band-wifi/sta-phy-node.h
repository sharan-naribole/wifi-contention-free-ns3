#ifndef STAPHYNODE_H
#define STAPHYNODE_H

#include "ap-phy-node.h"

using namespace ns3;

class StaPhyNode: public PhyNode {

protected:
  ApPhyNode* m_apNode;
  Ptr<PPBPQueue> m_ppbp = CreateObject<PPBPQueue>();
  //PPBPGenerator* m_ppbp;
  void PacketQueuePop(std::queue<Ptr<Packet>>);
  void ReceivePollRequest (Ptr<Packet> p, double snr, WifiTxVector txVector);

public:
  StaPhyNode(ApPhyNode*, uint32_t, std::string, uint8_t, double);
  void PhyDownlinkSetup(WifiPhyStandard, Ptr<YansWifiChannel>, Ptr<ErrorRateModel>,
                        uint8_t);
  void PhyUplinkSetup(WifiPhyStandard, Ptr<YansWifiChannel>, Ptr<ErrorRateModel>,
                   uint8_t, bool);
  void StartTraffic();
  void StopTraffic();
};

StaPhyNode::StaPhyNode(ApPhyNode* apNode, uint32_t nodeId, std::string txMode,
                        uint8_t txPowerLevel, double distance)
: PhyNode(nodeId, txMode, txPowerLevel, distance), m_apNode(apNode)
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

  m_dl -> SetReceiveOkCallback(MakeCallback(&StaPhyNode::ReceivePollRequest,this));
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

void StaPhyNode::PacketQueuePop(std::queue<Ptr<Packet>> currQueue)
{
    uint32_t aggPktSize = 0;
    //std::cout << "Queue size = "
    //<< currQueue.size() << std::endl;

    while(currQueue.size() > 0)
    {
      aggPktSize += currQueue.front()->GetSize();
      currQueue.pop();
      m_ppbp->m_packetQueue.pop();
    }

    Send(m_ul, 0,aggPktSize);
}

void StaPhyNode::ReceivePollRequest(Ptr<Packet> p, double snr, WifiTxVector txVector)
{
  BasicHeader destinationHeader;
  p->RemoveHeader (destinationHeader);

  /*
  std::cout << "Node ID: " << m_nodeId
  << " RX header = " << destinationHeader.GetData()
  << " at "
  << Simulator::Now ().GetSeconds ()
  << std::endl;
  */

  if(destinationHeader.GetData() == m_nodeId)
  {
     //Extracting WiFi Socket
     uint32_t Npackets = m_ppbp->m_packetQueue.size();

     if(Npackets > 0)
     {
       std::queue<Ptr<Packet>> currQueue = m_ppbp->m_packetQueue;
       //Transmitting Poll reply

       /*
       std::cout << "Transmitting "
       << Npackets << " packets by Node "
       << m_nodeId<< " at "
       << Simulator::Now ().GetSeconds ()
       << std::endl;
       */

       m_apNode -> m_receiving = true;
       //std::cout << "AP's RX status changed to true at "
       //<< Simulator::Now().GetSeconds() << std::endl;

       Simulator::Schedule(Seconds(0.0),&StaPhyNode::PacketQueuePop,this,currQueue);
     }

     else
     {
       //m_apNode -> StartIdleTimer();
       //std::cout << "Zero traffic in node "
       //<< m_nodeId << std::endl;
       //m_apNode -> TransmitNextPoll();
     }

   }
}

#endif

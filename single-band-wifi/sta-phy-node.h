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
  void PhySetup(WifiPhyStandard, Ptr<YansWifiChannel>, Ptr<ErrorRateModel>);
  void StartTraffic();
  void StopTraffic();
};

StaPhyNode::StaPhyNode(ApPhyNode* apNode, uint32_t nodeId, std::string txMode,
                        uint8_t txPowerLevel, double distance)
: PhyNode(nodeId, txMode, txPowerLevel, distance), m_apNode(apNode)
{
  m_ppbp -> SetNodeId(nodeId);
}

void StaPhyNode::PhySetup(WifiPhyStandard standard, Ptr<YansWifiChannel> channel,
              Ptr<ErrorRateModel> error)
{
  m_wifi->SetChannel(channel);
  m_wifi->SetErrorRateModel (error);

  Ptr<MobilityModel> position = CreateObject<ConstantPositionMobilityModel> ();
  position->SetPosition (Vector (m_distance, 0.0, 0.0));
  m_wifi->SetMobility(position);

  m_wifi->ConfigureStandard(standard);

  m_wifi -> SetReceiveOkCallback(MakeCallback(&StaPhyNode::ReceivePollRequest,this));
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

    Send(0,aggPktSize);
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

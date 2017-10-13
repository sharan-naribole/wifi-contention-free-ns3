#include "ns3/core-module.h"
#include "ns3/simulator.h"
#include "ns3/applications-module.h"
#include <cmath>
#include "ns3/network-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"

using namespace ns3;

NS_OBJECT_ENSURE_REGISTERED (PhyNode);

PhyNode::PhyNode ()
{
  // we must provide a public default constructor,
  // implicit or explicit, but never private.
}

PhyNode::~PhyNode ()
{
}

TypeId
PhyNode::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::PhyNode")
    .SetParent<Object> ()
    .AddConstructor<PhyNode> ()
    .AddAttribute ("Distance", "The distance from AP",
             DoubleValue (5.0),
             MakeDoubleAccessor (&PhyNode::m_distance),
             MakeDoubleChecker<uint32_t> (1))
		.AddAttribute ("WiFiMode", "WifiMode in string format",
					   StringValue ("OfdmRate6Mbps"),
					   MakeStringAccessor (&PhyNode::m_txMode),
					   MakeStringChecker())
    .AddAttribute ("TXPowerLevel", "The transmission power level",
  					 UintegerValue (0),
  					 MakeUintegerAccessor (&PhyNode::m_txPowerLevel),
  					 MakeUintegerChecker<uint8_t> (1))
    .AddAttribute ("NodeID", "The identifier of PhyNode",
					   UintegerValue (0),
					   MakeUintegerAccessor (&PhyNode::m_nodeId),
					   MakeUintegerChecker<uint32_t> (1))
  ;
  return tid;
}

PhyNode::PhyNode (uint32_t nodeId, std::string txMode, double distance)
  : m_distance (distance),
    m_txMode (txMode),
    m_nodeId(nodeId)
{
}

void PhyNode::PhySetup(WifiPhyStandard standard, Ptr<YansWifiChannel> channel,
              Ptr<ErrorRateModel> error)
{
  m_tx->SetChannel(channel);
  m_tx->SetErrorRateModel (error);

  Ptr<MobilityModel> position = CreateObject<ConstantPositionMobilityModel> ();
  position->SetPosition (Vector (m_distance, 0.0, 0.0));
  m_tx->SetMobility(position);

  m_tx->ConfigureStandard(standard);

}

void
PhyNode::Send (uint32_t nodeId, uint32_t packetSize)
{
  Ptr<Packet> p = Create<Packet> (packetSize);

  BasicHeader sourceHeader;
  sourceHeader.SetData (nodeId);
  p->AddHeader (sourceHeader);

  WifiMode mode = WifiMode (m_txMode);
  WifiTxVector txVector;
  txVector.SetTxPowerLevel (m_txPowerLevel);
  txVector.SetMode (mode);
  txVector.SetPreambleType (WIFI_PREAMBLE_LONG);

  m_tx->SendPacket (p, txVector);
}

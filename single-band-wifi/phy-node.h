#include "ns3/core-module.h"
#include "ns3/yans-wifi-channel.h"

using namespace ns3;

///
class PhyNode : public Object
{
public:
  /// Input structure
  double m_distance; ///< distance
  std::string m_txMode; ///< transmit mode; Modulation
  uint8_t m_txPowerLevel = 0; ///< transmit power level
  uint32_t m_nodeId;

  PhyNode();
  virtual ~PhyNode();
  static TypeId GetTypeId (void);
  PhyNode (uint32_t, std::string, double);
  virtual void PhySetup(WifiPhyStandard, Ptr<YansWifiChannel>, Ptr<ErrorRateModel>);
  void Send (uint32_t nodeId, uint32_t packetSize);

protected:
  /// Output structure
  Ptr<YansWifiPhy> m_tx = CreateObject<YansWifiPhy> (); ///< transmit
};

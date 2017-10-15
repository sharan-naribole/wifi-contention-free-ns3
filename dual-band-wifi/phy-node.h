#include "ns3/core-module.h"
#include "ns3/yans-wifi-channel.h"
#include "ns3/mobility-module.h"

using namespace ns3;

///
class PhyNode : public Object
{
public:
  PhyNode();
  virtual ~PhyNode();
  static TypeId GetTypeId (void);
  PhyNode (uint32_t, std::string, uint8_t, double);
  void Send (Ptr<YansWifiPhy>, uint32_t nodeId, uint32_t packestSize);
  void GetChannelNumbers();

protected:
  double m_distance; ///< distance
  std::string m_txMode; ///< transmit mode; Modulation
  uint8_t m_txPowerLevel; ///< transmit power level
  uint32_t m_nodeId;
  bool multiband;
  uint8_t m_dlChannelNumber;
  uint8_t m_ulChannelNumber;
  Ptr<YansWifiPhy> m_dl = CreateObject<YansWifiPhy> (); ///< transmit
  Ptr<YansWifiPhy> m_ul = CreateObject<YansWifiPhy> ();
  Ptr<MobilityModel> m_position = CreateObject<ConstantPositionMobilityModel> ();
};

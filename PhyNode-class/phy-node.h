#include "ns3/core-module.h"
#include "ns3/yans-wifi-channel.h"
#include "ns3/mobility-module.h"
#include "ns3/error-model.h"

using namespace ns3;

///
class PhyNode : public Object
{
public:
  PhyNode();
  virtual ~PhyNode();
  static TypeId GetTypeId (void);
  PhyNode (uint32_t, std::string, uint8_t, double);
  void InterferenceSetup();
  void PhyDownlinkSetup(WifiPhyStandard, Ptr<YansWifiChannel>, Ptr<ErrorRateModel>,
                        uint8_t);
  void PhyUplinkSetup(WifiPhyStandard, Ptr<YansWifiChannel>, Ptr<ErrorRateModel>,
                   uint8_t, bool);
  void Send (Ptr<YansWifiPhy>, uint32_t, uint32_t);
  void Send (Ptr<YansWifiPhy>, uint32_t, uint32_t, std::string);
  void GetChannelNumbers();

  Ptr<YansWifiPhy> m_dl = CreateObject<YansWifiPhy> (); ///< transmit
  Ptr<YansWifiPhy> m_ul = CreateObject<YansWifiPhy> ();

protected:
  double m_distance; ///< distance
  std::string m_txMode; ///< transmit mode; Modulation
  uint8_t m_txPowerLevel; ///< transmit power level
  uint32_t m_nodeId;
  uint64_t m_datarate;
  double m_intfErrorRate;
  bool m_multiband = false;
  uint8_t m_dlChannelNumber;
  uint8_t m_ulChannelNumber;
  Ptr<MobilityModel> m_position = CreateObject<ConstantPositionMobilityModel> ();
  Ptr<RateErrorModel> m_rem = CreateObject<RateErrorModel> ();
};

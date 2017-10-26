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
  PhyNode (uint32_t, std::string, uint8_t, Vector);
  void InterferenceDLSetup(double rate);
  void InterferenceULSetup(double rate);
  virtual void PhyDownlinkSetup(WifiPhyStandard, Ptr<YansWifiChannel>, Ptr<ErrorRateModel>,
                        uint8_t);
  virtual void PhyUplinkSetup(WifiPhyStandard, Ptr<YansWifiChannel>, Ptr<ErrorRateModel>,
                   uint8_t, bool);
  void Send (Ptr<YansWifiPhy>, uint32_t, uint32_t);
  void Send (Ptr<YansWifiPhy>, uint32_t, uint32_t, std::string);
  void GetChannelNumbers();

  Ptr<YansWifiPhy> m_dl = CreateObject<YansWifiPhy> (); ///< transmit
  Ptr<YansWifiPhy> m_ul = CreateObject<YansWifiPhy> ();

protected:
  Vector m_loc; ///< distance
  std::string m_txMode; ///< transmit mode; Modulation
  uint8_t m_txPowerLevel; ///< transmit power level
  uint32_t m_nodeId;
  uint64_t m_datarate;
  double m_intfErrorRateDL;
  double m_intfErrorRateUL;
  bool m_multiband = false;
  uint8_t m_dlChannelNumber;
  uint8_t m_ulChannelNumber;
  Ptr<MobilityModel> m_position = CreateObject<ConstantPositionMobilityModel> ();
  Ptr<UniformRandomVariable> m_rvDL = CreateObject<UniformRandomVariable> ();
  Ptr<UniformRandomVariable> m_rvUL = CreateObject<UniformRandomVariable> ();
  Ptr<RateErrorModel> m_remDL = CreateObject<RateErrorModel> ();
  Ptr<RateErrorModel> m_remUL = CreateObject<RateErrorModel> ();
};

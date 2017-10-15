#include "ns3/core-module.h"
#include "ns3/random-variable-stream.h"
#include "ns3/data-rate.h"
#include "ns3/packet.h"
#include <queue>

using namespace ns3;

class PPBPQueue: public Object
{
public:
  double	m_burstArrivals;  // Mean rate of burst arrivals
  double	m_burstLength; // Mean burst time length
  double	m_h;					 // Hurst parameter	(Pareto distribution)
  uint32_t m_packetSize;
  DataRate m_dataRate;
  uint32_t m_nodeId;

  std::queue<Ptr<Packet>> m_packetQueue;
  PPBPQueue();
  virtual ~PPBPQueue();
  void SetNodeId(uint32_t);
  void SetBurstArrivals(double);
  PPBPQueue(double, double, double,uint32_t,DataRate, int32_t);
  static TypeId GetTypeId (void);
  void StartPPBP();
  void StopPPBP();
  std::queue<Ptr<Packet>> GetQueue();
  std::queue<Ptr<Packet>>* GetQueueAddress();

private:
  uint32_t m_activebursts;
  bool  m_onPeriod;
  bool m_stop;
  Ptr<ExponentialRandomVariable> m_expVar;
  Ptr<ParetoRandomVariable> m_paretoVar;

  TracedCallback< Ptr<const Packet> > m_txTrace;	// Trace callback for each sent packet

  void ScheduleNextTx();
  void PushPacket();
  void PoissonArrival();
  void ParetoDeparture();
  void NextBurst();


};

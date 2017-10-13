#include "ns3/core-module.h"
#include "ns3/random-variable-stream.h"
#include "ns3/data-rate.h"
#include "ns3/packet.h"
#include <queue>

using namespace ns3;

class PPBPQueue: public Object
{
  double	m_burstArrivals = 20;  // Mean rate of burst arrivals
  double	m_burstLength = 0.2; // Mean burst time length
  double	m_h = 0.7;					 // Hurst parameter	(Pareto distribution)
  uint32_t m_packetSize = 1470;
  DataRate m_dataRate = DataRate ("1Mbps");
  uint32_t m_nodeId = 0;

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

  public:
    std::queue<Ptr<Packet>> m_packetQueue;
    PPBPQueue();
    virtual ~PPBPQueue();
    PPBPQueue(uint32_t);
    PPBPQueue(double);
    PPBPQueue(double, double, double,uint32_t,DataRate, int32_t);
    static TypeId GetTypeId (void);
    void StartPPBP();
    void StopPPBP();
    std::queue<Ptr<Packet>> GetQueue();
    std::queue<Ptr<Packet>>* GetQueueAddress();
};

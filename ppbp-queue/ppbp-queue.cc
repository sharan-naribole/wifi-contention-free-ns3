#include "ns3/core-module.h"
#include "ns3/random-variable-stream.h"
#include "ns3/data-rate.h"
#include "ns3/packet.h"
#include <queue>
#include "ppbp-queue.h"

using namespace ns3;

NS_OBJECT_ENSURE_REGISTERED (PPBPQueue);

PPBPQueue::PPBPQueue ()
{
  // we must provide a public default constructor,
  // implicit or explicit, but never private.
}
PPBPQueue::~PPBPQueue ()
{
}

TypeId
PPBPQueue::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::PPBPQueue")
    .SetParent<Object> ()
    .AddConstructor<PPBPQueue> ()
    .AddAttribute ("BurstIntensity", "The data rate of each burst.",
					   DataRateValue (DataRate ("1Mb/s")),
					   MakeDataRateAccessor (&PPBPQueue::m_dataRate),
					   MakeDataRateChecker ())
		.AddAttribute ("PacketSize", "The size of packets sent in on state",
					   UintegerValue (1470),
					   MakeUintegerAccessor (&PPBPQueue::m_packetSize),
					   MakeUintegerChecker<uint32_t> (1))
    .AddAttribute ("NodeID", "The identifier of PhyNode",
					   UintegerValue (0),
					   MakeUintegerAccessor (&PPBPQueue::m_nodeId),
					   MakeUintegerChecker<uint32_t> (1))
		.AddAttribute ("MeanBurstArrivals", "Mean Active Sources",
					   DoubleValue (20.0),
 					   MakeDoubleAccessor (&PPBPQueue::m_burstArrivals),
					   MakeDoubleChecker<double> (1))
		.AddAttribute ("MeanBurstTimeLength", "Pareto distributed burst durations",
					   DoubleValue (0.2),
					   MakeDoubleAccessor (&PPBPQueue::m_burstLength),
					   MakeDoubleChecker<double> ())
		.AddAttribute ("H", "Hurst parameter",
					   DoubleValue (0.7),
					   MakeDoubleAccessor (&PPBPQueue::m_h),
					   MakeDoubleChecker<double> ())
    .AddTraceSource ("Tx", "A new packet is queued",
						 MakeTraceSourceAccessor (&PPBPQueue::m_txTrace),
						 "ns3::Packet::TracedCallback")
  ;
  return tid;
}

void PPBPQueue::SetNodeId(uint32_t nodeId)
{
  m_nodeId = nodeId;
}

void PPBPQueue::SetBurstArrivals(double burstArrivals)
{
  m_burstArrivals = burstArrivals;
}

PPBPQueue::PPBPQueue(double burstArrivals,
  double burstLength, double hurst,uint32_t packetSize, DataRate dataRate,
  int32_t nodeId)
  : m_burstArrivals(burstArrivals), m_burstLength(burstLength), m_h(hurst),
  m_packetSize(packetSize),  m_dataRate(dataRate), m_nodeId(nodeId)
{
}

void PPBPQueue::StartPPBP()
{

  m_activebursts = 0;
  m_onPeriod = true;
  m_stop = false;


  NS_LOG_UNCOND("Starting PPBP first burst for Node " << m_nodeId);

  //std::cout << "Initial Queue Size of Node : "
  //<< m_nodeId << " = " << m_packetQueue.size() << std::endl;


  double inter_burst_intervals;
	inter_burst_intervals = (double)1/m_burstArrivals;

  m_expVar = CreateObject<ExponentialRandomVariable> ();
  m_paretoVar = CreateObject<ParetoRandomVariable> ();

  m_expVar->SetAttribute ("Mean", DoubleValue (inter_burst_intervals));
  //m_expVar->SetAttribute ("Bound", DoubleValue (3.0));

  double m_shape = 3 - (2 * m_h);
  double m_scale = (double) (m_shape - 1) * m_burstLength / m_shape;

  m_paretoVar->SetAttribute ("Scale", DoubleValue (m_scale));
  m_paretoVar->SetAttribute ("Shape", DoubleValue (m_shape));

  Simulator::Schedule(Seconds(0.0),&PPBPQueue::NextBurst, this);
}

void PPBPQueue::StopPPBP()
{
  m_stop = true;
}


void PPBPQueue::ScheduleNextTx()
{
    if(m_stop == false)
    {
      //std::cout << "Scheduling packet into Node "
      //<< m_socket->GetNode()->GetId()
      //<< "'s queue at "
      //<< Simulator::Now().GetSeconds() << std::endl;

      uint32_t bits = (m_packetSize + 30) * 8;
      Time nextTime(Seconds (bits /
                   static_cast<double>(m_dataRate.GetBitRate())));

      if (m_activebursts != 0)
      {
        m_onPeriod= false;
        double data_rate = (double) nextTime.GetSeconds() / m_activebursts;
        Simulator::Schedule(Seconds(data_rate),&PPBPQueue::PushPacket, this);
      }
      else
      {
        m_onPeriod= true;
      }
    }
}


void PPBPQueue::PushPacket()
{
  if(m_stop == false)
  {
    Ptr<Packet> packet = Create<Packet> (m_packetSize);
    m_packetQueue.push(packet);

    m_txTrace (packet);

    /*
    std::cout << "Queue size of Node "
    << m_nodeId
    << " = "
    << m_packetQueue.size()
    << " at " << Simulator::Now().GetSeconds()
    << std::endl;
    */


    Simulator::Schedule(Seconds(0.0),&PPBPQueue::ScheduleNextTx,this);
  }
}

void PPBPQueue::PoissonArrival()
{
  if(m_stop == false)
  {
    ++m_activebursts;
    //std::cout << "Active bursts of Node "
    //<< m_socket->GetNode()->GetId()
    //<< " increased to "
    //<< m_activebursts
    //<< " at " << Simulator::Now().GetSeconds()
    //<< std::endl;

    if (m_onPeriod)
    {
      Simulator::Schedule(Seconds(0.0),&PPBPQueue::ScheduleNextTx,this);
    }
  }
}

void PPBPQueue::ParetoDeparture()
{
   if(m_stop == false)
   {
      --m_activebursts;
      //std::cout << "Active bursts of Node "
      //<< m_socket->GetNode()->GetId()
      //<< " reduced to "
      //<< m_activebursts
      //<< " at " << Simulator::Now().GetSeconds()
      //<< std::endl;
  }
}

void PPBPQueue::NextBurst()
{
  if(m_stop == false)
  {
    double tExp = m_expVar->GetValue();
    Simulator::Schedule(Seconds(tExp),&PPBPQueue::PoissonArrival,this);
    //std::cout << "exp = " << tExp << std::endl;

    double tPareto = m_paretoVar->GetValue();
    //std::cout << "pareto = " << tPareto << std::endl;

    Simulator::Schedule(Seconds(tExp + tPareto) ,&PPBPQueue::ParetoDeparture,this);
    Simulator::Schedule(Seconds(tExp),&PPBPQueue::NextBurst,this);
  }
}

std::queue<Ptr<Packet>> PPBPQueue::GetQueue()
{
  return m_packetQueue;
}

std::queue<Ptr<Packet>>* PPBPQueue::GetQueueAddress()
{
  return &m_packetQueue;
}

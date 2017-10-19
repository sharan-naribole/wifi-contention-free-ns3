#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include <cmath>
#include <iomanip>
#include "ns3/applications-module.h"

using namespace ns3;

class Experiment{
private:
  double simTime = 100; // milliseconds
  uint32_t m_index;
  double m_rate;
  uint32_t m_iterations;
  Ptr<PPBPQueue> m_queue = CreateObject<PPBPQueue>();
  double m_queueSizeAvg = 0;
  double m_burstSizeAvg = 0;

public:
  Experiment(uint32_t index, double rate, uint32_t Niterations);
  void Setup();
  void Run();
  void StopRun();
  void Output();
};

Experiment::Experiment(uint32_t index, double rate, uint32_t Niterations)
: m_index(index), m_rate(rate), m_iterations(Niterations)
{
}

void Experiment::Setup()
{
  m_queue->SetAttribute("MeanBurstArrivals", DoubleValue(m_rate));
}

void Experiment::Run()
{
  for(uint32_t iter = 0; iter < m_iterations; iter++)
  {
    m_queue -> StartPPBP();
    Simulator::Schedule(MilliSeconds(simTime), &Experiment::StopRun,this);
    Simulator::Run();
    Simulator::Destroy();
  }
  Output();
}

void Experiment::StopRun()
{
  m_queueSizeAvg += m_queue->GetQueueSize();
  m_burstSizeAvg += m_queue->GetTotalBursts();
  m_queue -> StopPPBP();
}

void Experiment::Output()
{
  m_queueSizeAvg = std::floor(m_queueSizeAvg / (double)m_iterations);
  m_burstSizeAvg = std::floor(m_burstSizeAvg / (double)m_iterations);
  std::cout << m_index <<
    std::setw (12) << m_rate <<
    std::setw (12) << m_queueSizeAvg  <<
    std::setw (12) <<  m_burstSizeAvg <<
  std::endl;
}

int main (int argc, char *argv[])
{

  uint32_t Niterations = 2;

  std::cout << "Index" <<
    std::setw (12) << "Rate" <<
    std::setw (12) << "Packets" <<
    std::setw (12) << "Bursts" <<
    std::endl;

  uint32_t index = 1;

  for(double rate = 5; rate <= 100; rate += 5)
  {
    Experiment experiment(index, rate, Niterations);
    experiment.Setup();
    experiment.Run();
    index++;

  }

  return 0;
}

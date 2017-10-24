#ifndef LiScanRun_H
#define LiScanRun_H
#include "ns3/core-module.h"
#include <cmath>
#include <queue>
#include "ns3/simulator.h"
#include "liscan-sta-phy-node.h"
#include <cmath>
#include "output.h"

using namespace ns3;

class LiScanRun
{
  protected:
    uint32_t m_nSta;
    uint32_t m_staIndex;
    bool m_stop;
    Output m_output;

  public:
    std::vector<LiscanStaNode*> m_staNodes;
    LiscanApNode* m_apNode;
    std::string filename;

    LiScanRun(std::vector<LiscanStaNode*> staNodes,
    LiscanApNode* apNode);
    ~LiScanRun();
    void StartCFP();
    void StopCFP();
    Output OutputCFP();
};

LiScanRun::LiScanRun(std::vector<LiscanStaNode*> staNodes,
LiscanApNode* apNode)
: m_staNodes(staNodes), m_apNode(apNode)
{
}

LiScanRun::~LiScanRun()
{
  //std::cout << "Destroying LiScan Run.." << std::endl;
  //Simulator::Stop();
}

void LiScanRun::StartCFP()
{
   m_nSta = m_staNodes.size();
   //std::cout << "Nsta = " << m_nSta << std::endl;
   m_staIndex = 1;
   m_stop = false;

   for(uint32_t i = 0; i < m_nSta; i++)
   {
     m_staNodes[i]->StartTraffic();
   }

   //std::cout << "LiScan beginning.." << std::endl;

   // Begin Transmitting Poll requests
   m_apNode -> StartPolling(m_staIndex, m_nSta);
}

void LiScanRun::StopCFP()
{
  m_stop = true;
  m_apNode -> StopPolling();
  for(uint32_t i = 0; i < m_nSta; ++i)
  {
    m_staNodes[i]->StopTraffic();
  }
}

Output LiScanRun::OutputCFP()
{
  // Computing Metrics
  m_output.m_throughput = m_apNode->GetThroughput();
  m_output.m_overhead = 0.001*m_apNode->GetOverhead(); // milliseconds

  double delaySum = 0;
  uint32_t NpacketsTotal = 0;

  for(uint32_t staIter = 0; staIter < m_staNodes.size(); staIter++)
  {
    delaySum += m_staNodes[staIter]->GetDelaySum();
    NpacketsTotal += m_staNodes[staIter]->GetNPacketsTX();
  }

  if(NpacketsTotal > 0)
  {
    // milliseconds
    m_output.m_delayMean = (double) 0.001*delaySum / (double) NpacketsTotal;
  }

  return m_output;
  //std::cout << "Metrics: " << std::endl;
  //std::cout << "Delay (ms): " << 0.001*m_output.m_delayMean << std::endl;
  //std::cout << "Throughput (Mbps): " << m_output.m_throughput << std::endl;
  //std::cout << "Overhead (ms): " << 0.001*m_output.m_overhead << std::endl;
}


#endif

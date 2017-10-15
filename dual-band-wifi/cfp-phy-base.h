#ifndef CFPPhyBase_H
#define CFPPhyBase_H
#include "ns3/core-module.h"
#include <cmath>
#include <queue>
#include "ns3/simulator.h"
#include "sta-phy-node.h"
#include <cmath>

using namespace ns3;

class CFPPhyBase
{
  protected:
    uint32_t m_nSta;
    uint32_t m_staIndex;
    bool m_stop;

  public:
    std::vector<StaPhyNode*> m_staNodes;
    ApPhyNode* m_apNode;

    CFPPhyBase(std::vector<StaPhyNode*> staNodes,
    ApPhyNode* apNode);
    ~CFPPhyBase();
    void StartCFP();
    void StopCFP();
};

CFPPhyBase::CFPPhyBase(std::vector<StaPhyNode*> staNodes,
ApPhyNode* apNode)
: m_staNodes(staNodes), m_apNode(apNode)
{
}

CFPPhyBase::~CFPPhyBase()
{
  std::cout << "Destroying CFP Run.." << std::endl;
  //Simulator::Stop();
}

void CFPPhyBase::StartCFP()
{
   m_nSta = m_staNodes.size();
   //std::cout << "Nsta = " << m_nSta << std::endl;
   m_staIndex = 1;
   m_stop = false;

   for(uint32_t i = 0; i < m_nSta; i++)
   {
     m_staNodes[i]->StartTraffic();
   }

   std::cout << "CFP beginning.." << std::endl;

   // Begin Transmitting Poll requests
   m_apNode -> StartPolling(m_staIndex, m_nSta);
}

void CFPPhyBase::StopCFP()
{
  m_stop = true;
  m_apNode -> StopPolling();
  for(uint32_t i = 0; i < m_nSta; ++i)
  {
    m_staNodes[i]->StopTraffic();
  }
  Simulator::Stop();
}


#endif

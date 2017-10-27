#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "liscan-run.h"
#include <cmath>
#include "ns3/yans-wifi-channel.h"
#include "ns3/propagation-loss-model.h"
#include "ns3/propagation-delay-model.h"
#include "ns3/nist-error-rate-model.h"
#include "output.h"
#include <fstream>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("cfp-data-collection");

class Experiment
{
public:
  Experiment(uint32_t, double, double, double, uint32_t, double,std::string);
  void Setup();
  void Run();

private:
  uint32_t m_runIndex;
  double m_percTraffic;
  double m_trafficRate;
  double m_errorRate; // For Wi-Fi uplink
  uint32_t m_clientSize;
  double m_simTime;
  LiScanRun m_cfp;
  std::string m_filename;

  void Stop();
  std::vector<uint32_t> TrafficClassifier();
};

Experiment::Experiment(uint32_t runIndex, double percTraffic, double trafficRate,
  double errorRate, uint32_t clientSize, double simTime, std::string filename)
: m_runIndex(runIndex), m_percTraffic(percTraffic), m_trafficRate(trafficRate),
m_errorRate(errorRate),m_clientSize(clientSize), m_simTime(simTime), m_filename(filename)
{
}

void Experiment::Setup()
{
  WifiPhyStandard standard = WIFI_PHY_STANDARD_80211a;
  uint8_t downlinkChannelNumber = 1;
  uint8_t uplinkChannelNumber = 4;

  Ptr<YansWifiChannel> dlChannel = CreateObject<YansWifiChannel> ();
  dlChannel->SetPropagationDelayModel (CreateObject<ConstantSpeedPropagationDelayModel> ());

  Ptr<YansWifiChannel> ulChannel = CreateObject<YansWifiChannel> ();
  ulChannel->SetPropagationDelayModel (CreateObject<ConstantSpeedPropagationDelayModel> ());

  Ptr<LogDistancePropagationLossModel> log = CreateObject<LogDistancePropagationLossModel> ();
  dlChannel->SetPropagationLossModel (log);
  ulChannel->SetPropagationLossModel (log);
  Ptr<ErrorRateModel> error = CreateObject<NistErrorRateModel> ();

  LiscanApNode* apNode;
  apNode = new LiscanApNode(0, "OfdmRate18Mbps",0, Vector(0,0,0));
  apNode->PhyDownlinkSetup(standard, dlChannel, error,downlinkChannelNumber);
  apNode->PhyUplinkSetup(standard, ulChannel, error,uplinkChannelNumber, true);
  apNode->InterferenceDLSetup(0.01); // fixed for VLC
  apNode->InterferenceULSetup(m_errorRate); // UL Wi-Fi

  std::vector<LiscanStaNode*> staNodes;
  std::vector<uint32_t> trafficClassifier = TrafficClassifier();

  Ptr<RandomDiscPositionAllocator> location = CreateObject<RandomDiscPositionAllocator>();
  location -> SetAttribute("Rho", StringValue("ns3::UniformRandomVariable[Min=0.0|Max=25.0]"));

  for(uint32_t i = 0; i < m_clientSize; i++)
  {
    LiscanStaNode* temp;
    Vector loc = location -> GetNext();
    //std::cout << loc << std::endl;
    temp = new LiscanStaNode(apNode, i+1, "OfdmRate54Mbps", 0, loc);
    temp->PhyDownlinkSetup(standard, dlChannel, error,downlinkChannelNumber);
    temp->PhyUplinkSetup(standard, ulChannel, error,uplinkChannelNumber, true);
    temp->InterferenceDLSetup(0.01);
    temp->InterferenceULSetup(m_errorRate);

    if(trafficClassifier[i] == 1)
    {
      temp->m_ppbp->SetAttribute("MeanBurstArrivals",DoubleValue(m_trafficRate));
    }
    staNodes.push_back(temp);
  }

  LiScanRun cfp(staNodes,apNode);
  m_cfp = cfp;
}

std::vector<uint32_t> Experiment::TrafficClassifier()
{
  // 0 means low traffic for the client
  // 1 means high traffic for the client

  Ptr<UniformRandomVariable> randomSta = CreateObject<UniformRandomVariable> ();
  randomSta ->SetAttribute ("Min", DoubleValue (0));
  randomSta ->SetAttribute ("Max", DoubleValue (1));

  uint32_t index = 0;
  uint32_t found = 0;
  uint32_t maxHigh = std::ceil((double)m_clientSize*m_percTraffic);

  std::vector<uint32_t> trafficClass;

  for(uint32_t i = 0; i < m_clientSize; i++)
  {
    trafficClass.push_back(0);
  }

  while(found < maxHigh)
  {
    while(1)
    {
      if(trafficClass[index] == 0)
      {
        break;
      }
      else
      {
        index++;
        if(index == m_clientSize)
        {
          index = 0;
        }
      }
    }
    double odds = randomSta -> GetValue();
    //std::cout << odds << std::endl;
    if(odds <= m_percTraffic)
    {
      trafficClass[index] = 1;
      //std::cout << "Index = " << index << std::endl;
      found++;
    }
    else
    {
      index++;
      if(index == m_clientSize)
      {
        index = 0;
      }
    }
  }

  return trafficClass;
}


void Experiment::Run()
{
  m_cfp.StartCFP();
  Simulator::Schedule(MilliSeconds(m_simTime),&Experiment::Stop, this);
  Simulator::Run();
  Simulator::Destroy();
}

void Experiment::Stop()
{
  m_cfp.StopCFP();

  Output output = m_cfp.OutputCFP();

  std::ofstream myfile;
  myfile.open (m_filename,std::ios_base::app);
  myfile << m_runIndex
  << ", " << m_percTraffic
  << ", " << m_trafficRate
  << ", " << m_errorRate
  << ", " << m_clientSize
  << ", " << output.m_delayMean
  << ", " << output.m_throughput
  << ", " << output.m_overhead << std::endl;
  myfile.close();
}


int
main (int argc, char *argv[])
{

  double simTime = 100; // MilliSeconds
  uint32_t Nruns = 100;
  Time::SetResolution(Time::US);

  std::vector<uint32_t> clientSizeVec {1, 5, 10, 15, 20, 25, 30};

  for(double percTraffic = 0.1; percTraffic <= 1; percTraffic += 0.15)
  {
    for(double trafficRate = 20; trafficRate <= 200; trafficRate += 30) // metres
    {
      for(double errorRate = 0; errorRate < 0.3; errorRate += 0.05) // microseconds
      {
        for(auto clientSizeIter = clientSizeVec.begin(); clientSizeIter != clientSizeVec.end(); clientSizeIter++) // microseconds
        {
          std::cout << "Perc traffic: " << percTraffic
          << ", Traffic rate: " << trafficRate
          << ", Error rate: " << errorRate
          << ", Client size: " << *clientSizeIter << std::endl;
          for(uint32_t run = 1; run <= Nruns; run++)
          {
            Experiment experiment(run,percTraffic, trafficRate, errorRate,
              *clientSizeIter, simTime, "liscan_cfp_data.txt");
            experiment.Setup();
            experiment.Run();
          }
        }
      }
    }
  }
  return 0;
}

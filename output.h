#ifndef OUTPUT_H
#define OUTPUT_H

struct Output
{
  double m_delayMean = 0;
  uint32_t m_overhead = 0; ///< packet size
  double m_throughput = 0; ///< number of packets
};

#endif

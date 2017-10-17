/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#include "ns3/ptr.h"
#include "ns3/packet.h"
#include "ns3/trailer.h"
#include <iostream>
#include "basic-trailer.h"

using namespace ns3;

NS_OBJECT_ENSURE_REGISTERED (BasicTrailer);

/**
 * \ingroup network
 * A simple example of an Header implementation
 */

BasicTrailer::BasicTrailer ()
{
  // we must provide a public default constructor,
  // implicit or explicit, but never private.
}
BasicTrailer::~BasicTrailer ()
{
}

TypeId
BasicTrailer::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::BasicTrailer")
    .SetParent<Header> ()
    .AddConstructor<BasicTrailer> ()
  ;
  return tid;
}
TypeId
BasicTrailer::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

void
BasicTrailer::Print (std::ostream &os) const
{
  // This method is invoked by the packet printing
  // routines to print the content of my header.
  //os << "data=" << m_data << std::endl;
  os << "data=" << m_data;
}
uint32_t
BasicTrailer::GetSerializedSize (void) const
{
  // we reserve 2 bytes for our header.
  return 2;
}
void
BasicTrailer::Serialize (Buffer::Iterator start) const
{
  // we can serialize two bytes at the start of the buffer.
  // we write them in network byte order.
  start.WriteHtonU16 (m_data);
}
uint32_t
BasicTrailer::Deserialize (Buffer::Iterator start)
{
  // we can deserialize two bytes from the start of the buffer.
  // we read them in network byte order and store them
  // in host byte order.
  m_data = start.ReadNtohU16 ();

  // we return the number of bytes effectively read.
  return 2;
}

void
BasicTrailer::SetData (uint16_t data)
{
  m_data = data;
}
uint16_t
BasicTrailer::GetData (void) const
{
  return m_data;
}

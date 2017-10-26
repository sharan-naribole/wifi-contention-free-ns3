/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#include "ns3/ptr.h"
#include "ns3/packet.h"
#include "ns3/header.h"
#include <iostream>
#include "basic-header.h"

using namespace ns3;

NS_OBJECT_ENSURE_REGISTERED (BasicHeader);

/**
 * \ingroup network
 * A simple example of an Header implementation
 */

BasicHeader::BasicHeader ()
{
  // we must provide a public default constructor,
  // implicit or explicit, but never private.
}
BasicHeader::~BasicHeader ()
{
}

TypeId
BasicHeader::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::BasicHeader")
    .SetParent<Header> ()
    .AddConstructor<BasicHeader> ()
  ;
  return tid;
}
TypeId
BasicHeader::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

void
BasicHeader::Print (std::ostream &os) const
{
  // This method is invoked by the packet printing
  // routines to print the content of my header.
  //os << "data=" << m_data << std::endl;
  os << "data=" << m_data;
}
uint32_t
BasicHeader::GetSerializedSize (void) const
{
  // we reserve 2 bytes for our header.
  return 2;
}
void
BasicHeader::Serialize (Buffer::Iterator start) const
{
  // we can serialize two bytes at the start of the buffer.
  // we write them in network byte order.
  start.WriteHtonU16 (m_data);
}
uint32_t
BasicHeader::Deserialize (Buffer::Iterator start)
{
  // we can deserialize two bytes from the start of the buffer.
  // we read them in network byte order and store them
  // in host byte order.
  m_data = start.ReadNtohU16 ();

  // we return the number of bytes effectively read.
  return 2;
}

void
BasicHeader::SetData (uint16_t data)
{
  m_data = data;
}
uint16_t
BasicHeader::GetData (void) const
{
  return m_data;
}

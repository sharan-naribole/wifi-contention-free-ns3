#include "ns3/ptr.h"
#include "ns3/packet.h"
#include "ns3/header.h"
#include <iostream>

using namespace ns3;

/**
 * \ingroup network
 * A simple example of an Header implementation
 */
class BasicHeader : public Header
{
public:

    BasicHeader ();
  virtual ~BasicHeader ();

  /**
   * Set the header data.
   * \param data The data.
   */
  void SetData (uint16_t data);
  /**
   * Get the header data.
   * \return The data.
   */
  uint16_t GetData (void) const;

  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;
  virtual void Print (std::ostream &os) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);
  virtual uint32_t GetSerializedSize (void) const;
private:
  uint16_t m_data;  //!< Header data
};

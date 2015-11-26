/* -*- Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil -*- */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Authors: Yaogong Wang <ywang15@ncsu.edu>
 */

#ifndef NDN_FW_PMIN_TAG_H
#define NDN_FW_PMIN_TAG_H

#include "ns3/tag.h"

namespace ns3 {
namespace ndn {

/**
 * @brief Packet tag that is used to piggyback pmin for multipath
 */
class FwPminTag : public Tag
{
public:
  static TypeId
  GetTypeId (void);

  /**
   * @brief Default Constructor
   */
  FwPminTag () : m_pmin (0.0) { };

  /**
   * @brief Destructor
   */
  ~FwPminTag () { }

  /**
   * @brief Set pmin
   */
  void
  SetPmin (double pmin) { m_pmin = pmin; }

  /**
   * @brief Get pmin
   */
  double
  GetPmin () const { return m_pmin; }

  ////////////////////////////////////////////////////////
  // from ObjectBase
  ////////////////////////////////////////////////////////
  virtual TypeId
  GetInstanceTypeId () const;

  ////////////////////////////////////////////////////////
  // from Tag
  ////////////////////////////////////////////////////////

  virtual uint32_t
  GetSerializedSize () const;

  virtual void
  Serialize (TagBuffer i) const;

  virtual void
  Deserialize (TagBuffer i);

  virtual void
  Print (std::ostream &os) const;

private:
  double m_pmin;
};

} // namespace ndn
} // namespace ns3

#endif // NDN_FW_PMIN_TAG_H

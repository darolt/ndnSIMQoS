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

#include "ndn-fw-pmin-tag.h"

namespace ns3 {
namespace ndn {

TypeId
FwPminTag::GetTypeId ()
{
  static TypeId tid = TypeId("ns3::ndn::FwPminTag")
    .SetParent<Tag>()
    .AddConstructor<FwPminTag>()
    ;
  return tid;
}

TypeId
FwPminTag::GetInstanceTypeId () const
{
  return FwPminTag::GetTypeId ();
}

uint32_t
FwPminTag::GetSerializedSize () const
{
  return sizeof(double);
}

void
FwPminTag::Serialize (TagBuffer i) const
{
  i.WriteDouble (m_pmin);
}

void
FwPminTag::Deserialize (TagBuffer i)
{
  m_pmin = i.ReadDouble ();
}

void
FwPminTag::Print (std::ostream &os) const
{
  os << "pmin=" << m_pmin;
}

} // namespace ndn
} // namespace ns3

/* -*- Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2011 University of California, Los Angeles 
 *
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
 * Author: Alexander Afanasyev <alexander.afanasyev@ucla.edu>
 */

#include "ccnx-fib.h"

#include "ccnx.h"
#include "ccnx-face.h"
#include "ccnx-interest-header.h"

#include "ns3/node.h"
#include "ns3/assert.h"

#define NDN_RTO_ALPHA 0.125
#define NDN_RTO_BETA 0.25
#define NDN_RTO_K 4

//#define NDN_DEBUG_OSPF	0
//#define NDN_DEBUG_OSPF_NODES 0

//#define NDN_DUMP_FIB		0
namespace ns3 {


//////////////////////////////////////////////////////////////////////
// Helpers
//////////////////////////////////////////////////////////////////////
namespace __ccnx_private {

struct CcnxFibFaceMetricByFace
{
  typedef CcnxFibFaceMetricContainer::type::index<i_face>::type
  type;
};

struct ChangeStatus
{
  ChangeStatus (CcnxFibFaceMetric::Status status) : m_status (status) { }
  void operator() (CcnxFibFaceMetric &entry)
  {
    entry.m_status = m_status;
  }
private:
  CcnxFibFaceMetric::Status m_status;
};


// struct SearchByFace {
//   /**
//    * \brief To perform effective searches by CcnxFace
//    */
//   bool
//   operator() (const CcnxFibFaceMetric &m, const Ptr<CcnxFace> &face) const
//   {
//     return *(m.m_face) < *face;
//   } 

//   /**
//    * \brief To perform effective searches by CcnxFace
//    */
//   bool
//   operator() (const Ptr<CcnxFace> &face, const CcnxFibFaceMetric &m) const
//   {
//     return *face < *(m.m_face);
//   } 
// };

}
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

using namespace __ccnx_private;

void
CcnxFibFaceMetric::UpdateRtt::operator() (CcnxFibFaceMetric &entry)
{
  // const Time & this->m_rttSample
  
  //update srtt and rttvar (RFC 2988)
  if (entry.m_sRtt.IsZero ())
    {
      //first RTT measurement
      NS_ASSERT_MSG (entry.m_rttVar.IsZero (), "SRTT is zero, but variation is not");
      
      entry.m_sRtt = m_rttSample;
      entry.m_rttVar = Time (entry.m_sRtt / 2.0);
    }
  else
    {
      entry.m_rttVar = Time ((1 - NDN_RTO_BETA) * entry.m_rttVar + NDN_RTO_BETA * Abs(entry.m_sRtt - m_rttSample));
      entry.m_sRtt = Time ((1 - NDN_RTO_ALPHA) * entry.m_sRtt + NDN_RTO_ALPHA * m_rttSample);
    }
}

void
CcnxFibEntry::UpdateStatus (Ptr<CcnxFace> face, CcnxFibFaceMetric::Status status)
{
  CcnxFibFaceMetricByFace::type::iterator record = m_faces.get<i_face> ().find (face);
  NS_ASSERT_MSG (record != m_faces.get<i_face> ().end (), "Update status can be performed only on existing faces of CcxnFibEntry");

  m_faces.modify (record, ChangeStatus (status));

  // reordering random access index same way as by metric index
  m_faces.get<i_nth> ().rearrange (m_faces.get<i_metric> ().begin ());
}


Ptr<CcnxFace>
CcnxFibEntry::FindBestCandidate (int skip/* = 0*/)
{
  skip = skip % m_faces.size();
  return m_faces.get<i_nth> () [skip].GetFace ();
}


CcnxFib::CcnxFib ()
{
}

std::pair<CcnxFibEntryContainer::type::iterator, bool>
CcnxFib::LongestPrefixMatch (const CcnxInterestHeader &interest) const
{
  const CcnxNameComponents &name = interest.GetName ();
  for (size_t componentsCount = name.GetComponents ().size ();
       componentsCount > 0;
       componentsCount--)
    {
      CcnxNameComponents subPrefix (name.GetSubComponents (componentsCount));
      CcnxFibEntryContainer::type::iterator match = m_fib.find (subPrefix);
      if (match != m_fib.end())
        return std::pair<CcnxFibEntryContainer::type::iterator, bool> (match,true);
    }
  
  return std::pair<CcnxFibEntryContainer::type::iterator, bool> (m_fib.end(),false);
}

std::ostream& operator<< (std::ostream& os, const CcnxFib &fib)
{
  // os << "Node " << fib.m_node->GetObject<Node> ()->GetId () << "\n";
  os << "  Dest prefix      Interfaces(Costs)                  \n";
  os << "+-------------+--------------------------------------+\n";
  
  for (CcnxFibEntryContainer::type::iterator entry = fib.m_fib.begin ();
       entry != fib.m_fib.end ();
       entry++)
    {
      os << *entry << "\n";
    }
  return os;
}

std::ostream& operator<< (std::ostream& os, const CcnxFibEntry &entry)
{
  os << entry.m_prefix << "\t";
  for (CcnxFibFaceMetricByFace::type::iterator metric = entry.m_faces.get<i_face> ().begin ();
       metric != entry.m_faces.get<i_face> ().end ();
       metric++)
    {
      if (metric != entry.m_faces.get<i_face> ().begin ())
        os << ", ";

      os << *metric;
    }
  return os;
}

std::ostream& operator<< (std::ostream& os, const CcnxFibFaceMetric &metric)
{
  static const std::string statusString[] = {"","g","y","r"};

  os << metric.m_face << "(" << metric.m_routingCost << ","<< statusString [metric.m_status] << ")";
  return os;
}

// void CcnxFib::resetProbing()
// {
//     for(FibRangeIterator fib = _fib.begin(); fib != _fib.end(); fib++)
//         VALUE(fib).needsProbing = true;
// }

// void CcnxFib::updateInterfaceStatus( int interface, int status )
// {
// 	for( FibRangeIterator fib = _fib.begin(); fib!=_fib.end(); fib++ )
// 	{
// 		VALUE(fib).updateStatus( interface, status );
// 	}
// }

} // namespace ns3
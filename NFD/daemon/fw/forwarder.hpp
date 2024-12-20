/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2019,  Regents of the University of California,
 *                           Arizona Board of Regents,
 *                           Colorado State University,
 *                           University Pierre & Marie Curie, Sorbonne University,
 *                           Washington University in St. Louis,
 *                           Beijing Institute of Technology,
 *                           The University of Memphis.
 *
 * This file is part of NFD (Named Data Networking Forwarding Daemon).
 * See AUTHORS.md for complete list of NFD authors and contributors.
 *
 * NFD is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * NFD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * NFD, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef NFD_DAEMON_FW_FORWARDER_HPP
#define NFD_DAEMON_FW_FORWARDER_HPP

#include "face-table.hpp"
#include "forwarder-counters.hpp"
#include "unsolicited-data-policy.hpp"
#include "face/face-endpoint.hpp"
#include "table/fib.hpp"
#include "table/pit.hpp"
#include "table/cs.hpp"
#include "table/measurements.hpp"
#include "table/strategy-choice.hpp"
#include "table/dead-nonce-list.hpp"
#include "table/network-region-table.hpp"


namespace nfd {

namespace fw {
class Strategy;
} // namespace fw

/** \brief Main class of NFD's forwarding engine.
 *
 *  Forwarder owns all tables and implements the forwarding pipelines.
 */
class Forwarder
{

public:
  explicit
  Forwarder(FaceTable& faceTable);

  VIRTUAL_WITH_TESTS
  ~Forwarder();

  const ForwarderCounters&
  getCounters() const
  {
    return m_counters;
  }

  fw::UnsolicitedDataPolicy&
  getUnsolicitedDataPolicy() const
  {
    return *m_unsolicitedDataPolicy;
  }

  void
  setUnsolicitedDataPolicy(unique_ptr<fw::UnsolicitedDataPolicy> policy)
  {
    BOOST_ASSERT(policy != nullptr);
    m_unsolicitedDataPolicy = std::move(policy);
  }

public: // forwarding entrypoints and tables
  /** \brief start incoming Interest processing
   *  \param ingress face on which Interest is received and endpoint of the sender
   *  \param interest the incoming Interest, must be well-formed and created with make_shared
   */
  void
  startProcessInterest(const FaceEndpoint& ingress, const Interest& interest)
  {
    this->onIncomingInterest(ingress, interest);
  }

  /** \brief start incoming Data processing
   *  \param ingress face on which Data is received and endpoint of the sender
   *  \param data the incoming Data, must be well-formed and created with make_shared
   */
  void
  startProcessData(const FaceEndpoint& ingress, const Data& data)
  {
    this->onIncomingData(ingress, data);
  }

  /** \brief start incoming Nack processing
   *  \param ingress face on which Nack is received and endpoint of the sender
   *  \param nack the incoming Nack, must be well-formed
   */
  void
  startProcessNack(const FaceEndpoint& ingress, const lp::Nack& nack)
  {
    this->onIncomingNack(ingress, nack);
  }

  /** \brief start new nexthop processing
   *  \param prefix the prefix of the FibEntry containing the new nexthop
   *  \param nextHop the new NextHop
   */
  void
  startProcessNewNextHop(const Name& prefix, const fib::NextHop& nextHop)
  {
    this->onNewNextHop(prefix, nextHop);
  }

  NameTree&
  getNameTree()
  {
    return m_nameTree;
  }

  Fib&
  getFib()
  {
    return m_fib;
  }

  Pit&
  getPit()
  {
    return m_pit;
  }

  Cs&
  getCs()
  {
    return m_cs;
  }

  Measurements&
  getMeasurements()
  {
    return m_measurements;
  }

  StrategyChoice&
  getStrategyChoice()
  {
    return m_strategyChoice;
  }

  DeadNonceList&
  getDeadNonceList()
  {
    return m_deadNonceList;
  }

  NetworkRegionTable&
  getNetworkRegionTable()
  {
    return m_networkRegionTable;
  }

public:
  /** \brief trigger before PIT entry is satisfied
   *  \sa Strategy::beforeSatisfyInterest
   */
  signal::Signal<Forwarder, pit::Entry, Face, Data> beforeSatisfyInterest;

  /** \brief trigger before PIT entry expires
   *  \sa Strategy::beforeExpirePendingInterest
   */
  signal::Signal<Forwarder, pit::Entry> beforeExpirePendingInterest;

  /** \brief Signals when the incoming interest pipeline gets a hit from the content store
   */
  signal::Signal<Forwarder, Interest, Data> afterCsHit;

  /** \brief Signals when the incoming interest pipeline gets a miss from the content store
   */
  signal::Signal<Forwarder, Interest> afterCsMiss;

PUBLIC_WITH_TESTS_ELSE_PRIVATE: // pipelines
  /** \brief incoming Interest pipeline
   */
  VIRTUAL_WITH_TESTS void
  onIncomingInterest(const FaceEndpoint& ingress, const Interest& interest);

  /** \brief Interest loop pipeline
   */
  VIRTUAL_WITH_TESTS void
  onInterestLoop(const FaceEndpoint& ingress, const Interest& interest);

  /** \brief Content Store miss pipeline
  */
  VIRTUAL_WITH_TESTS void
  onContentStoreMiss(const FaceEndpoint& ingress,
                     const shared_ptr<pit::Entry>& pitEntry, const Interest& interest);

  /** \brief Content Store hit pipeline
  */
  VIRTUAL_WITH_TESTS void
  onContentStoreHit(const FaceEndpoint& ingress, const shared_ptr<pit::Entry>& pitEntry,
                    const Interest& interest, const Data& data);

  /** \brief outgoing Interest pipeline
   */
  VIRTUAL_WITH_TESTS void
  onOutgoingInterest(const shared_ptr<pit::Entry>& pitEntry,
                     const FaceEndpoint& egress, const Interest& interest);

  /** \brief Interest finalize pipeline
   */
  VIRTUAL_WITH_TESTS void
  onInterestFinalize(const shared_ptr<pit::Entry>& pitEntry);

  /** \brief incoming Data pipeline
   */
  VIRTUAL_WITH_TESTS void
  onIncomingData(const FaceEndpoint& ingress, const Data& data);

  /** \brief Data unsolicited pipeline
   */
  VIRTUAL_WITH_TESTS void
  onDataUnsolicited(const FaceEndpoint& ingress, const Data& data);

  /** \brief outgoing Data pipeline
   */
  VIRTUAL_WITH_TESTS void
  onOutgoingData(const Data& data, const FaceEndpoint& egress);

  /** \brief incoming Nack pipeline
   */
  VIRTUAL_WITH_TESTS void
  onIncomingNack(const FaceEndpoint& ingress, const lp::Nack& nack);

  /** \brief outgoing Nack pipeline
   */
  VIRTUAL_WITH_TESTS void
  onOutgoingNack(const shared_ptr<pit::Entry>& pitEntry,
                 const FaceEndpoint& egress, const lp::NackHeader& nack);

  VIRTUAL_WITH_TESTS void
  onDroppedInterest(const FaceEndpoint& egress, const Interest& interest);

  VIRTUAL_WITH_TESTS void
  onNewNextHop(const Name& prefix, const fib::NextHop& nextHop);

PROTECTED_WITH_TESTS_ELSE_PRIVATE:
  /** \brief set a new expiry timer (now + \p duration) on a PIT entry
   */
  void
  setExpiryTimer(const shared_ptr<pit::Entry>& pitEntry, time::milliseconds duration);

  /** \brief insert Nonce to Dead Nonce List if necessary
   *  \param upstream if null, insert Nonces from all out-records;
   *                  if not null, insert Nonce only on the out-records of this face
   */
   

/********** ACK **************/
void
Check_onOutgoingInterest_Ack(const shared_ptr<pit::Entry>& pitEntry, const FaceEndpoint& egress, const Interest& interest, const uint32_t x, const uint32_t y);

void
Check_onOutgoingData_Ack(const FaceEndpoint& egress, const Data& data);

/********** ACK **************/


   
/********** BF **************/
void
Check_onOutgoingInterest(const shared_ptr<pit::Entry>& pitEntry, const FaceEndpoint& egress, const Interest& interest);
void
Check_onOutgoingData(const FaceEndpoint& egress, const Data& data);

 
void
 d_onOutgoingInterest(const shared_ptr<pit::Entry>& pitEntry, const FaceEndpoint& egress, const Interest& interest);
 
 void
 d_onOutgoingData(const FaceEndpoint& egress, const Data& data);
 
 
/********** BF **************/  
   

   
  VIRTUAL_WITH_TESTS void
  insertDeadNonceList(pit::Entry& pitEntry, Face* upstream);

  /** \brief call trigger (method) on the effective strategy of pitEntry
   */
#ifdef WITH_TESTS
  virtual void
  dispatchToStrategy(pit::Entry& pitEntry, std::function<void(fw::Strategy&)> trigger)
#else
  template<class Function>
  void
  dispatchToStrategy(pit::Entry& pitEntry, Function trigger)
#endif
  {
    trigger(m_strategyChoice.findEffectiveStrategy(pitEntry));
  }


 void
 Periodic_Check_I_HopsFreqs();


 void
 Periodic_Check_D_HopsFreqs();

 void
 Bayes_Interest(const Interest& interest);

 void
 Bayes_Data(const Data& data);


 void
 Bayes_onOutgoingInterest();
 
 void
 Naive_Bayes_onOutgoingInterest();
 
 void
 Bayes_onOutgoingData();

 void
 Naive_Bayes_onOutgoingData();
 
 void
 Print_Report();
 
 void
 Print_Stat();
 
  
 void
 SendInterest_Gossip5(const shared_ptr<pit::Entry>& pitEntry,const FaceEndpoint& egress, const Interest& interest);
 
 void
 SendData_Gossip5(const FaceEndpoint& egress, const Data& data);
 
 void
 Decide_onOutgoingInterest(const shared_ptr<pit::Entry>& pitEntry,const FaceEndpoint& egress, const Interest& interest);

 void
 Decide_onOutgoingData(const FaceEndpoint& egress, const Data& data);
 
 
 
typedef std::vector<std::tuple <int, int, ns3::Time>> HopsFreqs;

private:
  ForwarderCounters m_counters;

  FaceTable& m_faceTable;
  unique_ptr<fw::UnsolicitedDataPolicy> m_unsolicitedDataPolicy;

  NameTree           m_nameTree;
  Fib                m_fib;
  Pit                m_pit;
  
  Cs                 m_cs;
  Measurements       m_measurements;
  StrategyChoice     m_strategyChoice;
  DeadNonceList      m_deadNonceList;
  NetworkRegionTable m_networkRegionTable;
  shared_ptr<Face>   m_csFace;

  
  
  
  int** m_i_Freqs = new int*[4];
  ns3::Time** m_i_Times = new ns3::Time*[4];
  
  std::vector<int> m_i_NamesDiameter;
  std::vector<int> m_i_NamesDiameter2;
  std::vector<ns3::Time> m_i_NamesTime;
  std::vector<int> m_i_NamesValid;
   

  int** m_d_Freqs = new int*[4];
  ns3::Time** m_d_Times = new ns3::Time*[4];

  std::vector<int>  m_d_NamesDiameter;
  std::vector<ns3::Time>  m_d_NamesTime;
  std::vector<int> m_d_NamesValid;
    
 bool m_Print_bool=true;
 
 uint32_t m_iBF;
 
 int nInInterests257=0 ;
 
 int nOutInterests257=0 ; 
 
 int nInData257=0;
 int nOutData257=0; 


 int e_f_counter=0;
 int e_m_counter=0;
 int e_l_counter=0;
 int e_v_counter=0;
 
 int a_y_counter=0;
 int a_m_counter=0;
 int a_o_counter=0;


 int n_s_counter=0;
 int n_c_counter=0;
 int n_f_counter=0;  
 
 

 int a_i_counter=0;
 int a_d_counter=0;
 
 int h_i_counter=0;
 int h_d_counter=0;
 
 int c_i_counter=0;
 int c_d_counter=0;
 
 int p_i_counter=0;
 int p_d_counter=0;

 int b_i_counter=0;
 int b_d_counter=0;
    
 bool m_i_start=true;
 bool m_d_start=true;
  
 bool m_i_learn=true;

 
  int* i_energy_yes=new int[4];     
  int* i_energy_no=new int[4]; 
 
 int*  i_age_yes=new int[4];     
 int*  i_age_no=new int[4];     
               
               
 int*  i_energy_yes1=new int[4];     
 int*  i_energy_no1=new int[4]; 
               
 int*  i_age_yes1=new int[4];
 int*  i_age_no1=new int[4];
 
 
 
 int*  i_near_yes=new int[9];     
 int*  i_near_no=new int[9];     
               
 int*  i_near_yes1=new int[9];
 int*  i_near_no1=new int[9];
 
 
 
 int*  i_valid_yes=new int[2];     
 int*  i_valid_no=new int[2];     
               
 int*  i_valid_yes1=new int[2];
 int* i_valid_no1=new int[2];
 
 
 
  int m_i_number=0;
  int m_i_yes=0;
  int m_i_no=0;
  
  int m_i_number1=0;
  int m_i_yes1=0;
  int m_i_no1=0;
  
  
 bool m_d_learn=true;
 
 
 int*  d_age_yes=new int[4];     
 int*  d_age_no=new int[4];     
               
 int*  d_age_yes1=new int[4];
 int*  d_age_no1=new int[4];



 int*   d_near_yes=new int[9];     
 int*  d_near_no=new int[9];     
               
 int*   d_near_yes1=new int[9];
 int*   d_near_no1=new int[9];
 
 
 
 int*   d_valid_yes=new int[2];     
 int*   d_valid_no=new int[2];     
               
 int*   d_valid_yes1=new int[2];
 int*   d_valid_no1=new int[2];
 

 
 
  int m_d_number=0;
  int m_d_yes=0;
  int m_d_no=0; 
  
  int m_d_number1=0;
  int m_d_yes1=0;
  int m_d_no1=0;
  
  
   int Diameter;
   ns3::Time DiameterTime;
   
  
   ns3::Time Wait_Time_max;
   ns3::Time pathValidityTime;


  // allow Strategy (base class) to enter pipelines
  friend class fw::Strategy;
};

} // namespace nfd

#endif // NFD_DAEMON_FW_FORWARDER_HPP

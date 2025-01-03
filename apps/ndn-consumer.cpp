/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2011-2015  Regents of the University of California.
 *
 * This file is part of ndnSIM. See AUTHORS for complete list of ndnSIM authors and
 * contributors.
 *
 * ndnSIM is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * ndnSIM is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * ndnSIM, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
 **/

#define max_hop 30

#define Consumer_Delay 2000

const int initial_Diameter=max_hop;


#include "ndn-consumer.hpp"
#include "ns3/ptr.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/packet.h"
#include "ns3/callback.h"
#include "ns3/string.h"
#include "ns3/boolean.h"
#include "ns3/uinteger.h"
#include "ns3/integer.h"
#include "ns3/double.h"

#include "utils/ndn-ns3-packet-tag.hpp"
#include "utils/ndn-rtt-mean-deviation.hpp"

#include <ndn-cxx/lp/tags.hpp>

#include <boost/lexical_cast.hpp>
#include <boost/ref.hpp>

#include "ns3/node-list.h"
#include "ns3/node.h"
#include "ns3/core-module.h"

#include "ndn-cxx/encoding/block-helpers.hpp"
#include "ndn-cxx/encoding/encoding-buffer.hpp"
#include "ndn-cxx/tag.hpp"
#include <ndn-cxx/lp/geo-tag.hpp>

#include <vector>
#include "ns3/random-variable-stream.h"
#include "ns3/nstime.h"
#include <algorithm>
#include <iostream>
#include <string>
#include <fstream>



/*********************************************************************************************/

#include "../NFD/daemon/face/generic-link-service.hpp" 

#include "../helper/ndn-stack-helper.hpp"
#include "../model/ndn-block-header.hpp"
#include <ndn-cxx/encoding/block.hpp>
#include <ndn-cxx/interest.hpp>
#include <ndn-cxx/data.hpp>

#include "../LogManager.cpp"

#include "utils/ndn-consumer-hop-distance-tag.hpp"
#include "utils/ndn-fw-hop-count-tag.hpp"
#include "utils/ndn-ns3-packet-tag.hpp"


/*********************************************************************************************/


NS_LOG_COMPONENT_DEFINE("ndn.Consumer");

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED(Consumer);

 
TypeId
Consumer::GetTypeId(void)
{

 static TypeId tid =
    TypeId("ns3::ndn::Consumer")
      .SetGroupName("Ndn")
      .SetParent<App>()
      .AddAttribute("StartSeq", "Initial sequence number", IntegerValue(0),
                    MakeIntegerAccessor(&Consumer::m_seq), MakeIntegerChecker<int32_t>())

      .AddAttribute("Prefix", "Name of the Interest", StringValue("/"),
                    MakeNameAccessor(&Consumer::m_interestName), MakeNameChecker())
      .AddAttribute("LifeTime", "LifeTime for interest packet", StringValue("2s"), //2s
                    MakeTimeAccessor(&Consumer::m_interestLifeTime), MakeTimeChecker())

      .AddAttribute("RetxTimer",
                    "Timeout defining how frequent retransmission timeouts should be checked",
                    StringValue("50ms"),  //50ms
                    MakeTimeAccessor(&Consumer::GetRetxTimer, &Consumer::SetRetxTimer),
                    MakeTimeChecker())

      .AddTraceSource("LastRetransmittedInterestDataDelay",
                      "Delay between last retransmitted Interest and received Data",
                      MakeTraceSourceAccessor(&Consumer::m_lastRetransmittedInterestDataDelay),
                      "ns3::ndn::Consumer::LastRetransmittedInterestDataDelayCallback")

      .AddTraceSource("FirstInterestDataDelay",
                      "Delay between first transmitted Interest and received Data",
                      MakeTraceSourceAccessor(&Consumer::m_firstInterestDataDelay),
                      "ns3::ndn::Consumer::FirstInterestDataDelayCallback");

  return tid;
}

Consumer::Consumer()
  : m_rand(CreateObject<UniformRandomVariable>())
  , m_seq(0)
  , m_seqMax(0) // don't request anything
  , Diameter(initial_Diameter)


{
  NS_LOG_FUNCTION_NOARGS();

  m_rtt = CreateObject<RttMeanDeviation>();
}

void
Consumer::SetRetxTimer(Time retxTimer)
{
  m_retxTimer = retxTimer;
  if (m_retxEvent.IsRunning()) {
    // m_retxEvent.Cancel (); // cancel any scheduled cleanup events
    Simulator::Remove(m_retxEvent); // slower, but better for memory
  }

  // schedule even with new timeout
  m_retxEvent = Simulator::Schedule(m_retxTimer, &Consumer::CheckRetxTimeout, this);
}

Time
Consumer::GetRetxTimer() const
{
  return m_retxTimer;
}

void
Consumer::CheckRetxTimeout()
{
  Time now = Simulator::Now();

  Time rto = m_rtt->RetransmitTimeout();
  // NS_LOG_DEBUG ("Current RTO: " << rto.ToDouble (Time::S) << "s");

  while (!m_seqTimeouts.empty()) {
    SeqTimeoutsContainer::index<i_timestamp>::type::iterator entry =
      m_seqTimeouts.get<i_timestamp>().begin();
    if (entry->time + rto <= now) // timeout expired?
    {
      uint32_t seqNo = entry->seq;
      m_seqTimeouts.get<i_timestamp>().erase(entry);
      OnTimeout(seqNo);
    }
    else
      break; // nothing else to do. All later packets need not be retransmitted
  }

  m_retxEvent = Simulator::Schedule(m_retxTimer, &Consumer::CheckRetxTimeout, this);
}

// Application Methods
void
Consumer::StartApplication() // Called at time specified by Start
{
  NS_LOG_FUNCTION_NOARGS();

  // do base stuff
  App::StartApplication();

  ScheduleNextPacket();
}

void
Consumer::StopApplication() // Called at time specified by Stop
{
  NS_LOG_FUNCTION_NOARGS();

  // cancel periodic packet generation
  Simulator::Cancel(m_sendEvent);

  // cleanup base stuff
  App::StopApplication();
}

void
Consumer::SendPacket()
{
  
    

  
  if (!m_active)
    return;
    
 

    NS_LOG_FUNCTION_NOARGS();
  
  

  uint32_t seq = std::numeric_limits<uint32_t>::max(); // invalid

  while (m_retxSeqs.size()) {
    seq = *m_retxSeqs.begin();
    m_retxSeqs.erase(m_retxSeqs.begin());
    break;
  }

  if (seq == std::numeric_limits<uint32_t>::max()) {
    if (m_seqMax != std::numeric_limits<uint32_t>::max()) {
      if (m_seq >= m_seqMax) {
        return; // we are totally done
      }
    }

    seq = m_seq++;
  }

 
        shared_ptr<Name> nameWithSequence = make_shared<Name>(m_interestName);
        nameWithSequence->appendSequenceNumber(seq);
  

 

        shared_ptr<Interest> interest = make_shared<Interest>();
        interest->setNonce(m_rand->GetValue(0, std::numeric_limits<uint32_t>::max()));
  
       interest->setName(*nameWithSequence);
       interest->setCanBePrefix(false);

        time::milliseconds interestLifeTime(m_interestLifeTime.GetMilliSeconds());
        interest->setInterestLifetime(interestLifeTime);
      
          
        uint32_t currentNodeId = LogHelper::GetNodeId();  
 
    //    ns3::Time now = ns3::Simulator::Now ();
   //     auto interval=(now.GetMilliSeconds()-DiameterTime.GetMilliSeconds());
   //     if((interval)>ns3::MilliSeconds(Consumer_Delay)) Diameter=initial_Diameter;
     
     
            
        std::shared_ptr<lp::GeoTag> tag = make_shared<lp::GeoTag>();
 
        std::tuple<uint32_t,uint32_t,uint32_t> location;
       
     
       
      
 // std::cout<< std::endl; 
         
    WillSendOutInterest(seq);
        
   int Retxcounts1= m_seqRetxCounts[seq];
     
    switch (Retxcounts1) {
  
     case int(1):  
     { 
       
        location={Diameter, 1, Diameter};
        std::cout<< "node"<< currentNodeId<< " generates interest 1st, seq= "<< seq<<" diameter= "<< Diameter<< std::endl;   
     break;
     }
 
    case int(2):  
     {  
         
        location={Diameter, 2,Diameter};
        std::cout<< "node"<< currentNodeId<< " generates interest 2nd, seq= "<< seq<< " diameter= " << Diameter<< std::endl;     
        break;
    
    }
  
    case int(3):  
    {        
       location={Diameter, 3, Diameter};  
       std::cout<< "node"<< currentNodeId<<" generates interest 3rd, seq= "<< seq<< " diameter= " << Diameter<< std::endl; 
       break;
    }

    case int(4):  
    {  
            
        location={Diameter, 4, Diameter};  
        std::cout<< "node"<< currentNodeId<<" generates interest 4th, seq =" << seq<< " diameter= " << Diameter<< std::endl; 
        break;
    }
    
    case int(5):  
     {
        location={Diameter, 5, Diameter}; 
        std::cout<< "node"<< currentNodeId<<" generates interest 5th, seq= "<< seq<< " diameter= " << Diameter<< std::endl;  
        break;
     }
    
     default:  
     {

        location={Diameter, 6, Diameter};  
        std::cout<< "node"<< currentNodeId<< " generates interest 6th, seq= "<< seq<< " diameter= "<< Diameter<< std::endl; 
        break;
     }
 }
 
 

       tag->setPosX(location);           
       interest->setTag<lp::GeoTag>(tag);   
   
       interest->setTag<lp::HopCountTag>(0);


 
 
      m_transmittedInterests(interest, this, m_face);
      m_appLink->onReceiveInterest(*interest);

     ScheduleNextPacket();
      
}




///////////////////////////////////////////////////
//          Process incoming packets             //
///////////////////////////////////////////////////

void
Consumer::OnData(shared_ptr<const Data> data)
{  

   
  if (!m_active)
    return;
   

   uint32_t currentNodeId = LogHelper::GetNodeId();   

  App::OnData(data); // tracing inside

  NS_LOG_FUNCTION(this << data);

   NS_LOG_INFO ("Received content object: " << boost::cref(*data));

  // This could be a problem......
  uint32_t seq = data->getName().at(-1).toSequenceNumber();
  
 //  NS_LOG_INFO("node " << node2->GetId()<< "  receives  ");
   NS_LOG_INFO("< DATA for " << seq);

  

                   
      int hopCount = 0;
      auto hopCountTag = data->getTag<lp::HopCountTag>();
      if (hopCountTag != nullptr) { // e.g., packet came from local node's cache
       hopCount = *hopCountTag;
       }
  
  
  
  
  
     if( hopCount>=max_hop)  
       { 
          std::cout<< "Consumer OnData hopCount= " << hopCount<< std::endl;
       }
     
       
       std::shared_ptr<lp::GeoTag> tag = data->getTag<lp::GeoTag>();
       std::tuple<uint32_t, uint32_t, uint32_t> location=tag->getPos();
       uint32_t RETX=get<1>(location);
     
     
      std::cout<< "node"<<currentNodeId<< " data arrived " <<" seq= "<< seq<<  " hopcount= "<< hopCount << " C_RETX= "<< RETX<<std::endl;
    
    
    
 
   

    
/**********************************************/

  Diameter=hopCount;
  
  ns3::Time now = ns3::Simulator::Now ();
  DiameterTime=now;
  
  /**********************************************/
  
  
  
 
 
  NS_LOG_DEBUG("Hop count: " << hopCount);
 
  
   
  


  SeqTimeoutsContainer::iterator entry = m_seqLastDelay.find(seq);
  if (entry != m_seqLastDelay.end()) {
    m_lastRetransmittedInterestDataDelay(this, seq, Simulator::Now() - entry->time, hopCount);
  }



  entry = m_seqFullDelay.find(seq);
  if (entry != m_seqFullDelay.end()) {
    m_firstInterestDataDelay(this, seq, Simulator::Now() - entry->time, m_seqRetxCounts[seq], hopCount);
  }

  m_seqRetxCounts.erase(seq);
  m_seqFullDelay.erase(seq);
  m_seqLastDelay.erase(seq);

  m_seqTimeouts.erase(seq);
  m_retxSeqs.erase(seq);

  m_rtt->AckSeq(SequenceNumber32(seq));
 
}

void
Consumer::OnNack(shared_ptr<const lp::Nack> nack)
{
  /// tracing inside
  App::OnNack(nack);
  
  auto node3 = ns3::NodeList::GetNode(ns3::Simulator::GetContext());
  NS_LOG_INFO("node " << node3->GetId()<< "  receives  NACK");
  NS_LOG_INFO("NACK received for: " << nack->getInterest().getName()
              << ", reason: " << nack->getReason());
}

void
Consumer::OnTimeout(uint32_t sequenceNumber)
{
  NS_LOG_FUNCTION(sequenceNumber);
  
   auto node4 = ns3::NodeList::GetNode(ns3::Simulator::GetContext());
  NS_LOG_INFO("node " << node4->GetId()<< "  Timeout");
  NS_LOG_INFO("for seq: " << sequenceNumber);
  
  // std::cout << Simulator::Now () << ", TO: " << sequenceNumber << ", current RTO: " <<
  // m_rtt->RetransmitTimeout ().ToDouble (Time::S) << "s\n";

  m_rtt->IncreaseMultiplier(); // Double the next RTO
  m_rtt->SentSeq(SequenceNumber32(sequenceNumber),
                 1); // make sure to disable RTT calculation for this sample
 
   NS_LOG_INFO("RetxCounts: " <<  m_seqRetxCounts[sequenceNumber]);
  
  
  
  m_retxSeqs.insert(sequenceNumber);
  
  ScheduleNextPacket();
  
  
}

void
Consumer::WillSendOutInterest(uint32_t sequenceNumber)
{
  NS_LOG_INFO("Trying to add " << sequenceNumber << " with " << Simulator::Now() << ". already "
                                << m_seqTimeouts.size() << " items");

  m_seqTimeouts.insert(SeqTimeout(sequenceNumber, Simulator::Now()));
  m_seqFullDelay.insert(SeqTimeout(sequenceNumber, Simulator::Now()));

  m_seqLastDelay.erase(sequenceNumber);
  m_seqLastDelay.insert(SeqTimeout(sequenceNumber, Simulator::Now()));

  m_seqRetxCounts[sequenceNumber]++;

  m_rtt->SentSeq(SequenceNumber32(sequenceNumber), 1);
}

} // namespace ndn
} // namespace ns3

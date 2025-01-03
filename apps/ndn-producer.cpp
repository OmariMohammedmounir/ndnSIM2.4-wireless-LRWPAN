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
#define Producer_Delay 2000

const int initial_Dimameter=3;

#include "ndn-producer.hpp"
#include "ns3/log.h"
#include "ns3/string.h"
#include "ns3/uinteger.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"

#include "model/ndn-l3-protocol.hpp"
#include "helper/ndn-fib-helper.hpp"

#include <memory>

#include "ndn-cxx/encoding/block-helpers.hpp"
#include "ndn-cxx/encoding/encoding-buffer.hpp"
#include "ndn-cxx/tag.hpp"
#include <ndn-cxx/lp/geo-tag.hpp>
#include <vector>

#include <ns3/log.h>
#include <ns3/ptr.h>
#include <ns3/assert.h>
#include <vector>
#include <ns3/mobility-model.h>

#include "../NFD/daemon/face/generic-link-service.hpp" 

#include "utils/ndn-consumer-hop-distance-tag.hpp"
#include "utils/ndn-fw-hop-count-tag.hpp"
#include "utils/ndn-ns3-packet-tag.hpp"

#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include "ns3/random-variable-stream.h"
#include "ns3/nstime.h"
#include <algorithm>



 /***** DMIF ********/
#include "../src/ndnSIM/LogManager.cpp"
 /***** DMIF ********/

NS_LOG_COMPONENT_DEFINE("ndn.Producer");

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED(Producer);

TypeId
Producer::GetTypeId(void)
{
  static TypeId tid =
    TypeId("ns3::ndn::Producer")
      .SetGroupName("Ndn")
      .SetParent<App>()
      .AddConstructor<Producer>()
      .AddAttribute("Prefix", "Prefix, for which producer has the data", StringValue("/"),
                    MakeNameAccessor(&Producer::m_prefix), MakeNameChecker())
      .AddAttribute(
         "Postfix",
         "Postfix that is added to the output data (e.g., for adding producer-uniqueness)",
         StringValue("/"), MakeNameAccessor(&Producer::m_postfix), MakeNameChecker())
      .AddAttribute("PayloadSize", "Virtual payload size for Content packets", UintegerValue(1024),
                    MakeUintegerAccessor(&Producer::m_virtualPayloadSize),
                    MakeUintegerChecker<uint32_t>())
      .AddAttribute("Freshness", "Freshness of data packets, if 0, then unlimited freshness",
                    TimeValue(Seconds(0)), MakeTimeAccessor(&Producer::m_freshness),
                    MakeTimeChecker())
      .AddAttribute(
         "Signature",
         "Fake signature, 0 valid signature (default), other values application-specific",
         UintegerValue(0), MakeUintegerAccessor(&Producer::m_signature),
         MakeUintegerChecker<uint32_t>())
      .AddAttribute("KeyLocator",
                    "Name to be used for key locator.  If root, then key locator is not used",
                    NameValue(), MakeNameAccessor(&Producer::m_keyLocator), MakeNameChecker());
  return tid;
}

Producer::Producer()

{
  NS_LOG_FUNCTION_NOARGS();
}

// inherited from Application base class.
void
Producer::StartApplication()
{
  NS_LOG_FUNCTION_NOARGS();
  App::StartApplication();

  FibHelper::AddRoute(GetNode(), m_prefix, m_face, 0);
}

void
Producer::StopApplication()
{
  NS_LOG_FUNCTION_NOARGS();

  App::StopApplication();
}






void
Producer::OnInterest(shared_ptr<const Interest> interest)
{
  App::OnInterest(interest); // tracing inside

  NS_LOG_FUNCTION(this << interest);

  if (!m_active)
    return;
    
 
 
 
 Name dataName(interest->getName());
 
 auto name= dataName.getSubName(0, 1);  

 

 
 //Determine hopCount
 
    int hopCount = 0; 
    auto hopCountTag = interest->getTag<lp::HopCountTag>();
    if (hopCountTag != nullptr) { hopCount = *hopCountTag; } 

    std::shared_ptr<lp::GeoTag> tag = interest->getTag<lp::GeoTag>();
    std::tuple<uint32_t, uint32_t, uint32_t> location=tag->getPos(); 

    uint32_t RETX=get<1>(location);
    
                          
                      
    if( hopCount>=max_hop)  
       { 
          std::cout<< "Producer OnData hopCount= " << hopCount<< std::endl;
          hopCount=max_hop-1;
       }                 

/**********************************************/

  Diameter=hopCount;
  
  ns3::Time now = ns3::Simulator::Now ();
  DiameterTime=now;
  /**********************************************/
     
  // dataName.append(m_postfix);
  // dataName.appendVersion();

  auto data = make_shared<Data>();
  data->setName(dataName);
  data->setFreshnessPeriod(::ndn::time::milliseconds(m_freshness.GetMilliSeconds()));

  data->setContent(make_shared< ::ndn::Buffer>(m_virtualPayloadSize));
   
 



  Signature signature;
  SignatureInfo signatureInfo(static_cast< ::ndn::tlv::SignatureTypeValue>(255));

  if (m_keyLocator.size() > 0) {
    signatureInfo.setKeyLocator(m_keyLocator);
  }

  signature.setInfo(signatureInfo);
  signature.setValue(::ndn::makeNonNegativeIntegerBlock(::ndn::tlv::SignatureValue, m_signature));

  data->setSignature(signature);
  
  NS_LOG_INFO("node(" << GetNode()->GetId() << ") responding with Data: " << data->getName());



  /***** DMIF ********/
  
       
 uint32_t currentNodeId = LogHelper::GetNodeId();  
 
 std::cout<< "node" << currentNodeId<< " interest arrived hopcount= "<<  hopCount<< " P_RETX= "<< RETX<<std::endl;;
                          
 std::shared_ptr<lp::GeoTag> tag_d = make_shared<lp::GeoTag>();
 
 std::tuple<uint32_t,uint32_t,uint32_t> location_d={Diameter, RETX, Diameter};
 tag_d->setPosX(location_d);           
 data->setTag<lp::GeoTag>(tag_d);
      
 
  /***** DMIF ********/



  // to create real wire encoding
  data->wireEncode();

  m_transmittedDatas(data, this, m_face);
  m_appLink->onReceiveData(*data);
 
   
}

} // namespace ndn
} // namespace ns3


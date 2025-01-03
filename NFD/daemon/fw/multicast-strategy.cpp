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

#include "multicast-strategy.hpp"
#include "algorithm.hpp"
#include "common/logger.hpp"
#include <ndn-cxx/lp/tags.hpp>

namespace nfd {
namespace fw {

NFD_REGISTER_STRATEGY(MulticastStrategy);

NFD_LOG_INIT(MulticastStrategy);

const time::microseconds MulticastStrategy::RETX_SUPPRESSION_INITIAL(10);     //10
const time::milliseconds MulticastStrategy::RETX_SUPPRESSION_MAX(250);     //250

MulticastStrategy::MulticastStrategy(Forwarder& forwarder, const Name& name)
  : Strategy(forwarder)
  , ProcessNackTraits(this)
  , m_retxSuppression(RETX_SUPPRESSION_INITIAL,
                      RetxSuppressionExponential::DEFAULT_MULTIPLIER, 
                      RETX_SUPPRESSION_MAX)
{
  ParsedInstanceName parsed = parseInstanceName(name);
  if (!parsed.parameters.empty()) {
    NDN_THROW(std::invalid_argument("MulticastStrategy does not accept parameters"));
  }
  if (parsed.version && *parsed.version != getStrategyName()[-1].toVersion()) {
    NDN_THROW(std::invalid_argument(
      "MulticastStrategy does not support version " + to_string(*parsed.version)));
  }
  this->setInstanceName(makeInstanceName(name, getStrategyName()));
}

const Name&
MulticastStrategy::getStrategyName()
{
  static Name strategyName("/localhost/nfd/strategy/multicast/%FD%03");
  return strategyName;
}

void
MulticastStrategy::afterReceiveInterest(const FaceEndpoint& ingress, const Interest& interest,
                                        const shared_ptr<pit::Entry>& pitEntry)
{
  const fib::Entry& fibEntry = this->lookupFib(*pitEntry);
  const fib::NextHopList& nexthops = fibEntry.getNextHops();

  int nEligibleNextHops = 0;

  bool isSuppressed = false;

  for (const auto& nexthop : nexthops) {
    Face& outFace = nexthop.getFace();

  
 //added 
 /*
  int x = 0;
  auto incomingFaceIdTag = interest.getTag<lp::IncomingFaceIdTag>();
  if (incomingFaceIdTag != nullptr) { x = *incomingFaceIdTag;} 
 
  int y= outFace.getId();
 
  if(x==257 && y==257) 
    {    
       std::shared_ptr<lp::GeoTag> tag = interest.getTag<lp::GeoTag>();
       std::tuple<uint32_t, uint32_t, uint32_t> location=tag->getPos();
       uint32_t old_distance=get<0>(location);
       uint32_t RETX=get<1>(location);
       uint32_t d_distance=get<2>(location);
       
       
       shared_ptr<lp::GeoTag> tag_i = make_shared<lp::GeoTag>();
 
       std::tuple<uint32_t,uint32_t,uint32_t> location_i={old_distance, RETX, d_distance};
       tag_i->setPosX(location_i);           
       interest.setTag<lp::GeoTag>(tag_i);  
       
       if(RETX>=2)  
           {
          //    std::cout<<"RETX= "<<RETX<<std::endl;
              
             this->sendInterest(pitEntry, FaceEndpoint(outFace, 0), interest);
             return;

           }
    }
   
 //added 

 */  
      RetxSuppressionResult suppressResult = m_retxSuppression.decidePerUpstream(*pitEntry, outFace);




    if (suppressResult == RetxSuppressionResult::SUPPRESS) {
      NFD_LOG_DEBUG(interest << " from=" << ingress << " to=" << outFace.getId() << " suppressed");
      isSuppressed = true;
      std::cout<< "yes suppression" << std::endl;
      continue;
    }

  
    
    if ((outFace.getId() == ingress.face.getId() && outFace.getLinkType() != ndn::nfd::LINK_TYPE_AD_HOC) ||
        wouldViolateScope(ingress.face, interest, outFace)) {
      continue;
    }

    this->sendInterest(pitEntry, FaceEndpoint(outFace, 0), interest);
    NFD_LOG_DEBUG(interest << " from=" << ingress << " pitEntry-to=" << outFace.getId());

    if (suppressResult == RetxSuppressionResult::FORWARD) {
      m_retxSuppression.incrementIntervalForOutRecord(*pitEntry->getOutRecord(outFace));
    }
    ++nEligibleNextHops;
  }

  if (nEligibleNextHops == 0 && !isSuppressed) {
    NFD_LOG_DEBUG(interest << " from=" << ingress << " noNextHop");

    lp::NackHeader nackHeader;
    nackHeader.setReason(lp::NackReason::NO_ROUTE);
    this->sendNack(pitEntry, ingress, nackHeader);

    this->rejectPendingInterest(pitEntry);
  }
}

void
MulticastStrategy::afterReceiveNack(const FaceEndpoint& ingress, const lp::Nack& nack,
                                    const shared_ptr<pit::Entry>& pitEntry)
{
  this->processNack(ingress.face, nack, pitEntry);
}

} // namespace fw
} // namespace nfd

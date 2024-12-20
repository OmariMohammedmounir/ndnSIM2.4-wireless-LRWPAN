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

#include "name-tree-entry.hpp"
#include "name-tree.hpp"
#include <ctime>
#include <iostream>


namespace nfd {
namespace name_tree {

Entry::Entry(const Name& name, Node* node)
  : m_name(name)
  , m_node(node)
{
  BOOST_ASSERT(node != nullptr);
  BOOST_ASSERT(name.size() <= NameTree::getMaxDepth());
}

void
Entry::setParent(Entry& entry)
{
  BOOST_ASSERT(this->getParent() == nullptr);
  BOOST_ASSERT(!this->getName().empty());
  BOOST_ASSERT(entry.getName() == this->getName().getPrefix(-1));

  m_parent = &entry;
  m_parent->m_children.push_back(this);
}

void
Entry::unsetParent()
{
  BOOST_ASSERT(this->getParent() != nullptr);

  auto i = std::find(m_parent->m_children.begin(), m_parent->m_children.end(), this);
  BOOST_ASSERT(i != m_parent->m_children.end());
  m_parent->m_children.erase(i);

  m_parent = nullptr;
}

bool
Entry::hasTableEntries() const
{
  return m_fibEntry != nullptr ||
         !m_pitEntries.empty() ||
         m_measurementsEntry != nullptr ||
         m_strategyChoiceEntry != nullptr;
}

void
Entry::setFibEntry(unique_ptr<fib::Entry> fibEntry)
{
  BOOST_ASSERT(fibEntry == nullptr || fibEntry->m_nameTreeEntry == nullptr);

  if (m_fibEntry != nullptr) {
    m_fibEntry->m_nameTreeEntry = nullptr;
  }
  m_fibEntry = std::move(fibEntry);

  if (m_fibEntry != nullptr) {
    m_fibEntry->m_nameTreeEntry = this;
  }
}

void
Entry::insertPitEntry(shared_ptr<pit::Entry> pitEntry)
{
  BOOST_ASSERT(pitEntry != nullptr);
  BOOST_ASSERT(pitEntry->m_nameTreeEntry == nullptr);

  m_pitEntries.push_back(pitEntry);
  pitEntry->m_nameTreeEntry = this;
}

void
Entry::erasePitEntry(pit::Entry* pitEntry)
{
  BOOST_ASSERT(pitEntry != nullptr);
  BOOST_ASSERT(pitEntry->m_nameTreeEntry == this);

  auto it = std::find_if(m_pitEntries.begin(), m_pitEntries.end(),
                         [pitEntry] (const auto& pitEntry2) { return pitEntry2.get() == pitEntry; });
  BOOST_ASSERT(it != m_pitEntries.end());

  pitEntry->m_nameTreeEntry = nullptr; // must be done before pitEntry is deallocated
  *it = m_pitEntries.back(); // may deallocate pitEntry
  m_pitEntries.pop_back();
}

void
Entry::setMeasurementsEntry(unique_ptr<measurements::Entry> measurementsEntry)
{
  BOOST_ASSERT(measurementsEntry == nullptr || measurementsEntry->m_nameTreeEntry == nullptr);

  if (m_measurementsEntry != nullptr) {
    m_measurementsEntry->m_nameTreeEntry = nullptr;
  }
  m_measurementsEntry = std::move(measurementsEntry);

  if (m_measurementsEntry != nullptr) {
    m_measurementsEntry->m_nameTreeEntry = this;
  }
}

void
Entry::setStrategyChoiceEntry(unique_ptr<strategy_choice::Entry> strategyChoiceEntry)
{
  BOOST_ASSERT(strategyChoiceEntry == nullptr || strategyChoiceEntry->m_nameTreeEntry == nullptr);

  if (m_strategyChoiceEntry != nullptr) {
    m_strategyChoiceEntry->m_nameTreeEntry = nullptr;
  }
  m_strategyChoiceEntry = std::move(strategyChoiceEntry);

  if (m_strategyChoiceEntry != nullptr) {
    m_strategyChoiceEntry->m_nameTreeEntry = this;
  }
}

/************ BF *************/

void
Entry::setName(const Name& prefix)
  {
    m_name= prefix;
  }
  

void
Entry::setCounter(const uint32_t counter)
{

	m_BF_Counter = counter;
}
uint32_t
Entry::getCounter() const
{
  return m_BF_Counter;
}

void
Entry::clearCounter()
{
	m_BF_Counter=0;	
}


void
Entry::setBFTime(const ns3::Time time)
{ 
	m_BF_TimeStamp = time;
}

ns3::Time
Entry::getBFTime() const
{
    return m_BF_TimeStamp;

}


void
Entry::setInput(const uint32_t input)
{ 
	m_BF_Input = input;
}

uint32_t
Entry::getInput() const
{
  return m_BF_Input;

}

void
Entry::setOutput(const uint32_t output)
{
 
	m_BF_Output = output;
}

uint32_t
Entry::getOutput() const
{
  return m_BF_Output;
}


uint32_t
Entry::getIoD ()const
{
  return m_BF_IoD;

}

void
Entry::setIoD(const uint32_t IoD)
{
 
	m_BF_IoD = IoD;
}




/************ BF *************/





/************ DMIF *************/
void
Entry::setForwarderId(const uint32_t id){
	m_DMIF_forwarderId = id;
}
uint32_t
Entry::getForwarderId() const
{
	
  return m_DMIF_forwarderId;
	
}

void
Entry::setInterestOrData(const uint32_t iOrd){ 
	m_DMIF_InterestOrData = iOrd;
}

uint32_t
Entry::getInterestOrData() const
{
		
    return m_DMIF_InterestOrData;
}


void
Entry::setDMIFTime(const ns3::Time time)
{ 
	m_DMIF_TimeStamp = time;
}

ns3::Time
Entry::getDMIFTime() const
{
    return m_DMIF_TimeStamp;

}




/************ DMIF *************/



} // namespace name_tree
} // namespace nfd

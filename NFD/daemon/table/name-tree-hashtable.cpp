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

 const int  DMIF_Delay=5000;

const int  BF_Delay=400000;

 
#include "name-tree-hashtable.hpp"
#include "common/city-hash.hpp"
#include "common/logger.hpp"
#include "ns3/random-variable-stream.h"


#include "ns3/ndnSIM/NFD/daemon/table/fib.hpp"
#include "ns3/ndnSIM/NFD/daemon/fw/forwarder.hpp"
#include "ns3/ndnSIM/NFD/daemon/table/fib-entry.hpp"
#include "ns3/ndnSIM/NFD/daemon/table/name-tree.hpp"
#include "ns3/ndnSIM/NFD/daemon/table/name-tree-entry.hpp"

#include <ctime>
#include <ns3/simulator.h>
#include "ns3/core-module.h"
#include "ns3/nstime.h"


#include "../src/ndnSIM/LogManager.cpp"


namespace nfd {
namespace name_tree {

NFD_LOG_INIT(NameTreeHashtable);

class Hash32
{
public:
  static HashValue
  compute(const void* buffer, size_t length)
  {
    return static_cast<HashValue>(CityHash32(reinterpret_cast<const char*>(buffer), length));
  }
};

class Hash64
{
public:
  static HashValue
  compute(const void* buffer, size_t length)
  {
    return static_cast<HashValue>(CityHash64(reinterpret_cast<const char*>(buffer), length));
  }
};

/** \brief a type with compute static method to compute hash value from a raw buffer
 */
using HashFunc = std::conditional<(sizeof(HashValue) > 4), Hash64, Hash32>::type;

HashValue
computeHash(const Name& name, size_t prefixLen)
{
  name.wireEncode(); // ensure wire buffer exists

  HashValue h = 0;
  for (size_t i = 0, last = std::min(prefixLen, name.size()); i < last; ++i) {
    const name::Component& comp = name[i];
    h ^= HashFunc::compute(comp.wire(), comp.size());
  }
  return h;
}

HashSequence
computeHashes(const Name& name, size_t prefixLen)
{
  name.wireEncode(); // ensure wire buffer exists

  size_t last = std::min(prefixLen, name.size());
  HashSequence seq;
  seq.reserve(last + 1);

  HashValue h = 0;
  seq.push_back(h);

  for (size_t i = 0; i < last; ++i) {
    const name::Component& comp = name[i];
    h ^= HashFunc::compute(comp.wire(), comp.size());
    seq.push_back(h);
  }
  return seq;
}

Node::Node(HashValue h, const Name& name)
  : hash(h)
  , prev(nullptr)
  , next(nullptr)
  , entry(name, this)
{
}

Node::~Node()
{
  BOOST_ASSERT(prev == nullptr);
  BOOST_ASSERT(next == nullptr);
}

Node*
getNode(const Entry& entry)
{
  return entry.m_node;
}

HashtableOptions::HashtableOptions(size_t size)
  : initialSize(size)
  , minSize(size)
{
}

Hashtable::Hashtable(const Options& options)
  : m_options(options)
  , m_size(0)
{
  BOOST_ASSERT(m_options.minSize > 0);
  BOOST_ASSERT(m_options.initialSize >= m_options.minSize);
  BOOST_ASSERT(m_options.expandLoadFactor > 0.0);
  BOOST_ASSERT(m_options.expandLoadFactor <= 1.0);
  BOOST_ASSERT(m_options.expandFactor > 1.0);
  BOOST_ASSERT(m_options.shrinkLoadFactor >= 0.0);
  BOOST_ASSERT(m_options.shrinkLoadFactor < 1.0);
  BOOST_ASSERT(m_options.shrinkFactor > 0.0);
  BOOST_ASSERT(m_options.shrinkFactor < 1.0);

  m_buckets.resize(options.initialSize);
  this->computeThresholds();
}

Hashtable::~Hashtable()
{
  for (size_t i = 0; i < m_buckets.size(); ++i) {
    foreachNode(m_buckets[i], [] (Node* node) {
      node->prev = node->next = nullptr;
      delete node;
    });
  }
}

   
          


void
Hashtable::attach(size_t bucket, Node* node)
{
  node->prev = nullptr;
  node->next = m_buckets[bucket];

  if (node->next != nullptr) {
    BOOST_ASSERT(node->next->prev == nullptr);
    node->next->prev = node;
  }

  m_buckets[bucket] = node;
}

void
Hashtable::detach(size_t bucket, Node* node)
{
  if (node->prev != nullptr) {
    BOOST_ASSERT(node->prev->next == node);
    node->prev->next = node->next;
  }
  else {
    BOOST_ASSERT(m_buckets[bucket] == node);
    m_buckets[bucket] = node->next;
  }

  if (node->next != nullptr) {
    BOOST_ASSERT(node->next->prev == node);
    node->next->prev = node->prev;
  }

  node->prev = node->next = nullptr;
}



void
Hashtable::DMIF_PeriodicCheck_Fib_Entries()
{

   
 
 for (size_t i = 0; i < m_buckets.size(); ++i) 
 {
    foreachNode(m_buckets[i], [this] (Node* node) 
    
    {
        size_t bucket = this->computeBucketIndex(node->hash);
       
       auto iOrd = node->entry.getInterestOrData();
       auto TimeStamp= node->entry.getDMIFTime();
       
       ns3::Time now = ns3::Simulator::Now ();
       auto interval=(now.GetMilliSeconds()-TimeStamp.GetMilliSeconds());
    
       
       if(interval>=DMIF_Delay && iOrd==1) 
       {
        
   
        this->detach(bucket, node); delete node;
         --m_size;
         
          if (m_size < m_shrinkThreshold) 
            {       
              size_t newNBuckets = std::max(m_options.minSize,
              static_cast<size_t>(m_options.shrinkFactor * this->getNBuckets()));
              this->resize(newNBuckets);
           }  
         
       }
       
    if(interval>=DMIF_Delay && iOrd==2) 
       {
          
             this->detach(bucket,node); delete node;
            --m_size;
          
          
          if (m_size < m_shrinkThreshold) 
            {  
              size_t newNBuckets = std::max(m_options.minSize,
              static_cast<size_t>(m_options.shrinkFactor * this->getNBuckets()));
              this->resize(newNBuckets);
            } 
        
        
        
       }
     } );
}

     ns3::Simulator::Schedule(ns3::MilliSeconds(DMIF_Delay+0.01), &Hashtable::DMIF_PeriodicCheck_Fib_Entries, this);
}




void
Hashtable::BF_Periodic_check_fib_entries() 
{

 for (size_t i = 0; i < m_buckets.size(); ++i) 
 {
    
  
    
    
    foreachNode(m_buckets[i], [this] (Node* node) 
    
    {
        size_t bucket = this->computeBucketIndex(node->hash);
       
  //     auto input = node->entry.getInput();
  //     auto output = node->entry.getOutput();
       
   //    uint32_t counter=node->entry.getCounter(); 
       
       uint32_t IoD=node->entry.getIoD(); 
       
       
       auto TimeStamp= node->entry.getBFTime();
       
       ns3::Time now = ns3::Simulator::Now ();
       auto interval=(now.GetMilliSeconds()-TimeStamp.GetMilliSeconds());
    
    
       
       if(interval>BF_Delay && IoD==1) 
       {
        this->detach(bucket, node); delete node;
         --m_size;
         
           if (m_size < m_shrinkThreshold) 
            {  
              size_t newNBuckets = std::max(m_options.minSize,
              static_cast<size_t>(m_options.shrinkFactor * this->getNBuckets()));
              this->resize(newNBuckets);
            } 
     
     }
     
     
      if(interval>BF_Delay && IoD==2) 
       {
        this->detach(bucket, node); delete node;
         --m_size;
         
           if (m_size < m_shrinkThreshold) 
            {  
              size_t newNBuckets = std::max(m_options.minSize,
              static_cast<size_t>(m_options.shrinkFactor * this->getNBuckets()));
              this->resize(newNBuckets);
            } 
     
     }
     
     
       
     } );
  }
     ns3::Simulator::Schedule(ns3::MilliSeconds(BF_Delay+0.01), &Hashtable::BF_Periodic_check_fib_entries, this);
     
}



std::pair<const Node*, bool>
Hashtable::findOrInsert(const Name& name, size_t prefixLen, HashValue h, bool allowInsert)
{
  size_t bucket = this->computeBucketIndex(h);

  for (const Node* node = m_buckets[bucket]; node != nullptr; node = node->next) {
    if (node->hash == h && name.compare(0, prefixLen, node->entry.getName()) == 0 && (node->entry.getInterestOrData()==0)&& (node->entry.getCounter()==0)&& (node->entry.getInput()==0) && (node->entry.getOutput()==0))  
    {
      NFD_LOG_TRACE("found " << name.getPrefix(prefixLen) << " hash=" << h << " bucket=" << bucket);
      return {node, false};
    }
  }

  if (!allowInsert) {
    NFD_LOG_TRACE("not-found " << name.getPrefix(prefixLen) << " hash=" << h << " bucket=" << bucket);
    return {nullptr, false};
  }

  Node* node = new Node(h, name.getPrefix(prefixLen));
  
  node->entry.setForwarderId(0);

  node->entry.setInterestOrData(0);
  node->entry.setCounter(0);
  node->entry.setInput(0);
  node->entry.setOutput(0);
   
  this->attach(bucket, node);
  NFD_LOG_TRACE("insert " << node->entry.getName() << " hash=" << h << " bucket=" << bucket);
  ++m_size;

  if (m_size > m_expandThreshold) {
    this->resize(static_cast<size_t>(m_options.expandFactor * this->getNBuckets()));
  }

  return {node, true};
}


/********* BF **********/
void
Hashtable::Entry_insert_name(const Name& name, uint32_t x, uint32_t IoD)
{

        
       if(m_BF_bool1==0) 
        {
         m_BF_bool1=1;
 //          this->BF_Periodic_check_fib_entries();
        }
        
        auto prefixLen=name.size();
        HashValue h = computeHash(name, prefixLen);  
                 
        size_t bucket = this->computeBucketIndex(h);

        int bool1=0;
		
	for (Node* node = m_buckets[bucket]; node != nullptr; node = node->next) 
	 {
				 
	   Name  name1=node->entry.getName();
	   uint32_t input = node->entry.getInput();	
//	   uint32_t output = node->entry.getOutput();
	   
	   uint32_t counter=node->entry.getCounter();
           uint32_t IoD1=node->entry.getIoD();

           if (node->hash == h  && name.compare(name1)==0 && input==x && IoD1==IoD && counter!=0 )
           {
	      bool1=1;
	      auto counter1=node->entry.getCounter();
              counter1=counter1+1;
              node->entry.setCounter(counter1);
	      return;
	   }		     
    	
    	}	  
		  

        
      if(bool1==0)
       {
          Node* node = new Node(h, name.getPrefix(prefixLen));
      
       	 this->attach(bucket, node);

      
          node->entry.setName(name);
          node->entry.setCounter(1);
          node->entry.setInput(x);
          node->entry.setOutput(0);
          node->entry.setIoD(IoD);

          ns3::Time now = ns3::Simulator::Now ();
          node->entry.setBFTime(now);
          
    

	++m_size;
	
	 if (m_size > m_expandThreshold) 
	  {
		this->resize(static_cast<size_t>(m_options.expandFactor * this->getNBuckets()));
	  }

      }
      else
         std::cout<< "Hashtable::Entry_insert_name WRONG CASE"<< std::endl;
     
   
}


void
Hashtable::Entry_insert_name2(const Name& name,  uint32_t x, uint32_t y, uint32_t IoD)
{

        
       if(m_BF_bool1==0) 
        {
          m_BF_bool1=1;
           this->BF_Periodic_check_fib_entries();
      }
        
        auto prefixLen=name.size();
        HashValue h = computeHash(name, prefixLen);  
                 
        size_t bucket = this->computeBucketIndex(h);

        int bool2=0;
		
	for (Node* node = m_buckets[bucket]; node != nullptr; node = node->next) 
	 {
				 
	   Name  name1=node->entry.getName();
	   uint32_t input = node->entry.getInput();	
	   uint32_t output = node->entry.getOutput();
	   
	   uint32_t counter=node->entry.getCounter();
           uint32_t IoD1=node->entry.getIoD();

           if (node->hash == h  && name.compare(name1)==0 && input==x && output==y && IoD1==IoD && counter!=0 )
           {
	      bool2=1;
	      auto counter1=node->entry.getCounter();
              counter1=counter1+1;
              node->entry.setCounter(counter1);
	      return;
	   }		     
    	
    	}	  
		  

        
      if(bool2==0)
       {
          Node* node = new Node(h, name.getPrefix(prefixLen));
      
       	 this->attach(bucket, node);

      
          node->entry.setName(name);
          node->entry.setCounter(1);
          node->entry.setInput(x);
          node->entry.setOutput(y);
          node->entry.setIoD(IoD);

          ns3::Time now = ns3::Simulator::Now ();
          node->entry.setBFTime(now);
          
    

	++m_size;
	
	 if (m_size > m_expandThreshold) 
	  {
		this->resize(static_cast<size_t>(m_options.expandFactor * this->getNBuckets()));
	  }

      }
      else
         std::cout<< "Hashtable::Entry_insert_name2 WRONG CASE"<< std::endl;
     
   
}



uint32_t
Hashtable::Entry_check_name1(const Name& name, const uint32_t x, uint32_t IoD)
{

  
    auto prefixLen=name.size();
    HashValue h = computeHash(name, prefixLen);  
                 
   size_t bucket = this->computeBucketIndex(h);



   int bool2=0;
		
   for (const Node* node = m_buckets[bucket]; node != nullptr; node = node->next) 
    {
			 
      Name  name1=node->entry.getName();
      uint32_t input = node->entry.getInput();	
      
    
  //  uint32_t output = node->entry.getOutput();
      uint32_t counter=node->entry.getCounter();
      uint32_t IoD1=node->entry.getIoD(); 

    
 
     if (node->hash == h && name.compare(name1)==0 && input==257 && IoD1==IoD && counter!=0)
      {
         bool2=1;
         return(counter);   
         
      }		     
    	
  }	  
		     
  if(bool2==0)
   {      	 
     uint32_t counter=0;
     std::cout<< "Hashtable::Entry_check_name1 counter= "<< counter<< std::endl;
     return(counter);    
   }
   
}


uint32_t
Hashtable::Entry_check_name2(const Name& name,  uint32_t x,  uint32_t y, uint32_t IoD)
{

  
    auto prefixLen=name.size();
    HashValue h = computeHash(name, prefixLen);  
                 
   size_t bucket = this->computeBucketIndex(h);



   int bool2=0;
		
   for (const Node* node = m_buckets[bucket]; node != nullptr; node = node->next) 
    {
			 
      Name  name1=node->entry.getName();
      uint32_t input = node->entry.getInput();	
      uint32_t output = node->entry.getOutput();
      uint32_t counter=node->entry.getCounter();
      uint32_t IoD1=node->entry.getIoD();  
 
     if (node->hash == h && name.compare(name1)==0 && input==x && output==y && IoD1==IoD && counter!=0)
      {
         bool2=1;
         return(counter);   
         
      }		     
    	
  }	  
		     
  if(bool2==0)
   {      	 
     uint32_t counter=0;
//    std::cout<< "Hashtable::Entry_check_name2 counter= "<< counter<< std::endl;
     return(counter);    
   }
   
}



uint32_t
Hashtable::Entry_check_input1(const Name& name, uint32_t IoD)
{
 
    auto prefixLen=name.size();
    HashValue h = computeHash(name, prefixLen);  
                 
   size_t bucket = this->computeBucketIndex(h);


   int bool2=0;
		
   for (const Node* node = m_buckets[bucket]; node != nullptr; node = node->next) 
    {
			 
      Name  name1=node->entry.getName();
      uint32_t input = node->entry.getInput();	
      uint32_t counter=node->entry.getCounter();   
      uint32_t IoD1=node->entry.getIoD();   
 
 
     if (node->hash == h  && name.compare(name1)==0 && input!=0 && IoD1==IoD && counter!=0)
      {
         bool2=1;
         return(input);   
         
      }		     
    	
  }	  
		     
  if(bool2==0)
   {      	 
     std::cout<< "Hashtable::Entry_check_input1 WRONG CASE"<< std::endl;
     return(10000);    
   }
   
}

uint32_t
Hashtable::Entry_check_input2(const Name& name,  uint32_t x, uint32_t y, uint32_t IoD)
{
 
    auto prefixLen=name.size();
    HashValue h = computeHash(name, prefixLen);  
                 
   size_t bucket = this->computeBucketIndex(h);


   int bool2=0;
		
   for (const Node* node = m_buckets[bucket]; node != nullptr; node = node->next) 
    {
			 
      Name  name1=node->entry.getName();
      uint32_t input = node->entry.getInput();	
      uint32_t output = node->entry.getOutput();	
      uint32_t counter=node->entry.getCounter();  
      uint32_t IoD1=node->entry.getIoD();  
      
      
     if (node->hash == h  && name.compare(name1)==0 && input==x && output==y && IoD1==IoD && counter!=0)
      {
         bool2=1;
         return(input);   
         
      }		     
    	
  }	  
		     
  if(bool2==0)
   {      	 
 //    std::cout<< "Hashtable::Entry_check_input2 WRONG CASE"<< std::endl;
     return(10000);    
   }
   
}

uint32_t
Hashtable::Entry_check_output2(const Name& name,  uint32_t x, uint32_t y, uint32_t IoD)
{

    auto prefixLen=name.size();
    HashValue h = computeHash(name, prefixLen);  
                 
   size_t bucket = this->computeBucketIndex(h);


   int bool2=0;
		
   for (const Node* node = m_buckets[bucket]; node != nullptr; node = node->next) 
    {
			 
      Name  name1=node->entry.getName();
      uint32_t input = node->entry.getInput();	
      uint32_t output = node->entry.getOutput();	
      uint32_t counter=node->entry.getCounter();  
      uint32_t IoD1=node->entry.getIoD();  

      
     if (node->hash == h  && name.compare(name1)==0 && input==x && output==y && IoD1==IoD && counter!=0)
      {
         bool2=1;
         return(output);   
         
      }		     
    	
  }	  
		     
  if(bool2==0)
   {      	 
  //   std::cout<< "Hashtable::Entry_check_output2 WRONG CASE"<< std::endl;
     return(10000);    
   }
   
}







void
Hashtable::Entry_clear_name(const Name& name, uint32_t x, uint32_t y, uint32_t IoD)
{

  
    auto prefixLen=name.size();
    HashValue h = computeHash(name, prefixLen);  
                 
   size_t bucket = this->computeBucketIndex(h);

   int bool2=0;
		
   for (Node* node = m_buckets[bucket]; node != nullptr; node = node->next) 
    {
			 
      Name  name1=node->entry.getName();
      uint32_t input = node->entry.getInput();
      uint32_t output = node->entry.getOutput();	
      uint32_t counter=node->entry.getCounter(); 
      uint32_t IoD1=node->entry.getIoD();  
     
     
     if (node->hash == h  && name.compare(name1)==0 && input==x && output==y && IoD1==IoD && counter!=0)
      {
         bool2=1;
         this->detach(bucket, node); delete node;
         --m_size;
         
          if (m_size < m_shrinkThreshold) 
          {
             size_t newNBuckets = std::max(m_options.minSize,
             static_cast<size_t>(m_options.shrinkFactor * this->getNBuckets()));
              this->resize(newNBuckets);
          }
        return;
      }		     
    	
  }	  
		     
  if(bool2==0)  std::cout<< "Hashtable::Entry_clear_name WRONG CASE"<< std::endl;
  

   
}

void
Hashtable::Entry_clear_name1(const Name& name, uint32_t x, uint32_t IoD)
{

  
    auto prefixLen=name.size();
    HashValue h = computeHash(name, prefixLen);  
                 
   size_t bucket = this->computeBucketIndex(h);

   int bool2=0;
		
   for (Node* node = m_buckets[bucket]; node != nullptr; node = node->next) 
    {
			 
      Name  name1=node->entry.getName();
      uint32_t input = node->entry.getInput();
      uint32_t counter=node->entry.getCounter(); 
      uint32_t IoD1=node->entry.getIoD();  
     
     
     if (node->hash == h  && name.compare(name1)==0 && input==x && IoD1==IoD && counter==1)
      {
         bool2=1;
         this->detach(bucket, node); delete node;
         --m_size;
         
          if (m_size < m_shrinkThreshold) 
          {
             size_t newNBuckets = std::max(m_options.minSize,
             static_cast<size_t>(m_options.shrinkFactor * this->getNBuckets()));
              this->resize(newNBuckets);
          }
        return;
      }		     
    	
  }	  
		     
  if(bool2==0)  std::cout<< "Hashtable::Entry_clear_name WRONG CASE"<< std::endl;
  

   
}

void
Hashtable::Entry_lock(const Name& name, uint32_t x, uint32_t y, uint32_t IoD)
{

  
    auto prefixLen=name.size();
    HashValue h = computeHash(name, prefixLen);  
                 
   size_t bucket = this->computeBucketIndex(h);

   int bool2=0;
		
   for (Node* node = m_buckets[bucket]; node != nullptr; node = node->next) 
    {
			 
      Name  name1=node->entry.getName();
      uint32_t input = node->entry.getInput();
      uint32_t output = node->entry.getOutput();	
      uint32_t counter=node->entry.getCounter(); 
      uint32_t IoD1=node->entry.getIoD();  
 
 
     if (node->hash == h  && name.compare(name1)==0 && input==x && output==y && IoD1==IoD && counter!=0)
      {
        bool2=1;
        node->entry.setOutput(y);	  
      }		     
    	
  }	  
		     
  if(bool2==0)  std::cout<< "Hashtable::Entry_lock WRONG CASE"<< std::endl;
  

   
}


/********* BF **********/





/********* DMIF **********/
 uint32_t
Hashtable::findOrInsert_dmif(const Name& name, size_t prefixLen, uint32_t forwarderId, bool allowInsert, bool interest_or_data, uint32_t iOrd)
{
               
        
       if(m_DMIF_bool1==0) 
         {
           m_DMIF_bool1=1;
           this->DMIF_PeriodicCheck_Fib_Entries();
         }
   
       
      
       auto prefixLen1=name.size();
       HashValue h = computeHash(name, prefixLen1);  
                 
     size_t bucket = this->computeBucketIndex(h);
       
   //    std::cout<<"name=  "<< name<<std::endl; 
    //   std::cout<<"fi=  "<< forwarderId<<std::endl;
    //   std::cout<<"Ord=  "<< iOrd<<std::endl;
    //   std::cout<<"h=  "<< h<<std::endl;     
                 
  


     for (Node* node = m_buckets[bucket]; node != nullptr; node = node->next) 
         {
			
        
        
        	   
                    auto  name1=node->entry.getName();
		    auto fi1 = node->entry.getForwarderId();
		    auto iOrd1 = node->entry.getInterestOrData();
		    auto h1=node->hash;
		    
			 
          //     std::cout<<"name1=  "<< name1<<std::endl; 
          //     std::cout<<"fi1=  "<< fi1<<std::endl;
          //     std::cout<<"Ord1=  "<< iOrd1<<std::endl;
         //      std::cout<<"h1=  "<< h1<<std::endl;
                
             if(allowInsert && interest_or_data && iOrd==2) 
		     {      
		          if (h1==h && name.compare(name1)==0 && forwarderId==fi1  && iOrd1==2) { std::cout<<"true true  222222222222222222222222"<< std::endl; 
                              return (fi1);}  
                         }
			 	
	    if(allowInsert && !interest_or_data && iOrd==2)  
		       {
		          if (h1==h && name.compare(name1)==0 && iOrd1==2)  return (fi1);    	      
		       }
		 		 
		 
	    if(!allowInsert && interest_or_data && iOrd==2) //search for data forwarder
                         {
                             
                              if (h1==h && name.compare(name1)==0 && forwarderId==fi1   && iOrd1==2)  { //std::cout<<"here inside  "<< std::endl; 
                              return (fi1);}    	 
                         } 


	     if(!allowInsert && !interest_or_data && iOrd==2) //search for data forwarder
                         {
                              if (h1==h && name.compare(name1)==0 && forwarderId==fi1 && iOrd1==2)   { //std::cout<<"false false  ord-222222222222222222222222"<< std::endl; 
                              return (fi1);}  
                         }
                         
	
	
	
	     if(allowInsert && interest_or_data && iOrd==1) //search for data forwarder
                         {
                              
                           //       std::cout<<"name1=  "<< name1<<std::endl; 
                           //       std::cout<<"fi1=  "<< fi1<<std::endl;
                           //       std::cout<<"Ord1=  "<< iOrd1<<std::endl;
                           //        std::cout<<"h1=  "<< h1<<"    h="<<h<< std::endl;
                              
                               if (h1==h && name.compare(name1)==0 && forwarderId==fi1  && iOrd1==1)   { //std::cout<<"True True  1111111111111111111111111111111111  "<< std::endl; 
                              return (fi1);}   	 
                         }
                         

	   if(allowInsert && !interest_or_data && iOrd==1) //search for data forwarder
                         {
                              if (h1==h && name.compare(name1)==0 && forwarderId==fi1   && iOrd1==1)    return (fi1);    	 
                         }	
                         	
	   
	   if(!allowInsert && interest_or_data && iOrd==1) //search for data forwarder
                         {
                              if (h1==h && name.compare(name1)==0 && forwarderId==fi1 && iOrd1==1)    { //std::cout<<"false true 1111111111111111111111111111 "<< std::endl; 
                              return (fi1);}  
                         }
		   
	   if(!allowInsert && !interest_or_data && iOrd==1) //search for data forwarder
                         {
                                  
                              if (h1==h && name.compare(name1)==0  && iOrd1==1)   { //std::cout<<"false false 11111111111111111111111111111111111111111111111 "<< std::endl; 
                              return (fi1);}  	 
                         }
                         		 		
   }
      
 
  
  if (allowInsert && !interest_or_data && iOrd==2)  return (40000);  
 
  if (!allowInsert && interest_or_data && iOrd==2)  return (40000);  

  if (!allowInsert && !interest_or_data && iOrd==2)  return (40000);   
  
  
  if (allowInsert && !interest_or_data && iOrd==1)  return (40000); 
  
  if(!allowInsert && interest_or_data && iOrd==1)  return (40000);
  
  if (!allowInsert && !interest_or_data && iOrd==1)  return (40000);
        
    
    
    
       Node* node = new Node(h, name.getPrefix(prefixLen));    
	
	this->attach(bucket, node);

    
  
         node->entry.setName(name);
         node->entry.setForwarderId(forwarderId);
         
         ns3::Time now = ns3::Simulator::Now ();
         
         node->entry.setDMIFTime(now);
        	
        
        node->entry.setInterestOrData(iOrd); 
        
          

	++m_size;
	
	
	if (m_size > m_expandThreshold) 
	{
	
		this->resize(static_cast<size_t>(m_options.expandFactor * this->getNBuckets()));
	}

         
	return {forwarderId};
 

}
/********* DMIF **********/







const Node*
Hashtable::find(const Name& name, size_t prefixLen) const
{
  HashValue h = computeHash(name, prefixLen);
  return const_cast<Hashtable*>(this)->findOrInsert(name, prefixLen, h, false).first;
}




const Node*
Hashtable::find(const Name& name, size_t prefixLen, const HashSequence& hashes) const
{
  BOOST_ASSERT(hashes.at(prefixLen) == computeHash(name, prefixLen));
  return const_cast<Hashtable*>(this)->findOrInsert(name, prefixLen, hashes[prefixLen], false).first;
}

std::pair<const Node*, bool>
Hashtable::insert(const Name& name, size_t prefixLen, const HashSequence& hashes)
{
  BOOST_ASSERT(hashes.at(prefixLen) == computeHash(name, prefixLen));
  return this->findOrInsert(name, prefixLen, hashes[prefixLen], true);
}
 


/********* DMIF **********/
 uint32_t
Hashtable::find_dmif(const Name& name, size_t prefixLen, uint32_t forwarderId, bool allowInsert, bool interest_or_data, uint32_t iOrd )  
   {
	
		HashValue h = computeHash(name, prefixLen);
		uint32_t fi= this->findOrInsert_dmif(name, prefixLen, forwarderId, allowInsert, interest_or_data, iOrd);
		return (fi);
		
	
  }
/********* DMIF **********/



 uint32_t
Hashtable::insert_dmif(const Name& name, size_t prefixLen,  uint32_t forwarderId, bool allowInsert, bool interest_or_data, uint32_t iOrd)
{

// BOOST_ASSERT(hashes.at(prefixLen) == computeHash(name, prefixLen));
   HashValue h = computeHash(name, prefixLen);
  
  uint32_t fi=this->findOrInsert_dmif(name, prefixLen, forwarderId, allowInsert, interest_or_data, iOrd);
  return(fi);
 
}  
  



void
Hashtable::erase(Node* node)
{
  BOOST_ASSERT(node != nullptr);
  BOOST_ASSERT(node->entry.getParent() == nullptr);

  size_t bucket = this->computeBucketIndex(node->hash);
  NFD_LOG_TRACE("erase " << node->entry.getName() << " hash=" << node->hash << " bucket=" << bucket);

  this->detach(bucket, node);
  delete node;
  --m_size;

  if (m_size < m_shrinkThreshold) {
    size_t newNBuckets = std::max(m_options.minSize,
      static_cast<size_t>(m_options.shrinkFactor * this->getNBuckets()));
    this->resize(newNBuckets);
  }
}

void
Hashtable::computeThresholds()
{
  m_expandThreshold = static_cast<size_t>(m_options.expandLoadFactor * this->getNBuckets());
  m_shrinkThreshold = static_cast<size_t>(m_options.shrinkLoadFactor * this->getNBuckets());
  NFD_LOG_TRACE("thresholds expand=" << m_expandThreshold << " shrink=" << m_shrinkThreshold);
}

void
Hashtable::resize(size_t newNBuckets)
{
  if (this->getNBuckets() == newNBuckets) {
    return;
  }
  NFD_LOG_DEBUG("resize from=" << this->getNBuckets() << " to=" << newNBuckets);

  std::vector<Node*> oldBuckets;
  oldBuckets.swap(m_buckets);
  m_buckets.resize(newNBuckets);

  for (Node* head : oldBuckets) {
    foreachNode(head, [this] (Node* node) {
      size_t bucket = this->computeBucketIndex(node->hash);
      this->attach(bucket, node);
    });
  }

  this->computeThresholds();
}








} // namespace name_tree
} // namespace nfd

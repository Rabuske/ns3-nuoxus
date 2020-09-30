#ifndef NUOXUS_CACHE_MANAGER_H
#define NUOXUS_CACHE_MANAGER_H

#include "ns3/uinteger.h"
#include "ns3/object.h"
#include "ns3/simple-ref-count.h"
#include <map>
#include <exception>

namespace ns3
{

typedef std::map <uint32_t, uint32_t> stored_content_t;

class ContentNotStoredException : public std::exception
{
    virtual const char* what() const throw()
    {
        return "Requested Content is Not on Disk";
    }
};


class DiskFullException : public std::exception
{
    virtual const char* what() const throw()
    {
        return "Disk is full";
    }
};

class NuoxusCacheManager : public SimpleRefCount<NuoxusCacheManager, Object>
{
public:

    NuoxusCacheManager();
    NuoxusCacheManager(uint32_t blockSize, uint64_t spaceInDiskInKilobytes);
    virtual ~NuoxusCacheManager();
    
    // 50 GB
    const static uint64_t g_space_in_disk_in_kilobytes = 50000000;
  /**
   * \brief Set the size of the block used by this component
   * \param blockSize is the size in KB of a content block
   */          
    void SetBlockSize(uint32_t blockSize);

  /**
   * \brief Get the size of the block used by this component
   */          
   uint32_t GetBlockSize();    

  /**
   * \brief Retrieve the content from the cache (for the simulation, it returns nothing). 
   * It throws ContentNotStoredException in case the content is not stored
   * \param contentId the unique identifier for a content
   * \param blockNumber is the number of the requested Block
   */          
    void Retrieve(uint32_t contentId, uint32_t blockNumber);    
    
  /**
   * \brief Store the content from the cache (for the simulation, it stores nothing but the packet number). 
   * It throws DiskFullException in case there is no space available for the packet
   * \param contentId the unique identifier for a content
   * \param blockNumber is the number of the requested Block
   */              
    void Store(uint32_t contentId, uint32_t blockNumber);    
    
  /**
   * \brief Delete the entire content from the cache
   * \param contentId the unique identifier for a content
   */             
    void Delete(uint32_t contentId, uint64_t size);   
    
  /**
   * \brief Return the remaining space availabe in disk in KB
   * \param contentId the unique identifier for a content
   */                 
    uint64_t GetAvailableSpace();        

    void ReserveSpace(uint64_t space);
    void SetDiskSize(uint64_t diskSizeInKb);

private:
    uint32_t m_block_size;                              //!< The block size in KB
    uint64_t m_used_space;                              //!< Space currently being used in disk
    uint64_t m_space_in_disk_in_kilobytes;              //!< Total space in disk
    stored_content_t m_stored_content;                  //!< The content stored in disk
};

}
#endif

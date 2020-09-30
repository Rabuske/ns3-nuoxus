#include "nuoxus-cache-manager.h"
#include "ns3/log.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE ("NuoxusCacheManager");
NS_OBJECT_ENSURE_REGISTERED (NuoxusCacheManager);

NuoxusCacheManager::NuoxusCacheManager()
{
    NS_LOG_FUNCTION (this);
    m_block_size = 0;
    m_space_in_disk_in_kilobytes = g_space_in_disk_in_kilobytes;
}

NuoxusCacheManager::~NuoxusCacheManager ()
{
    NS_LOG_FUNCTION (this);
}

NuoxusCacheManager::NuoxusCacheManager(uint32_t blockSize, uint64_t spaceInDiskInKilobytes)
{
    NS_LOG_FUNCTION (this);
    m_block_size = blockSize;
    m_space_in_disk_in_kilobytes = spaceInDiskInKilobytes;
}

void NuoxusCacheManager::SetBlockSize(uint32_t blockSize)
{
    NS_LOG_FUNCTION (this);
    m_block_size = blockSize;
}

uint32_t NuoxusCacheManager::GetBlockSize()
{
    NS_LOG_FUNCTION (this);
    return m_block_size;
}

void NuoxusCacheManager::Retrieve(uint32_t contentId, uint32_t blockNumber)
{
    NS_LOG_FUNCTION (this);
    if(m_stored_content[contentId] < blockNumber) throw ContentNotStoredException();
}

void NuoxusCacheManager::Store(uint32_t contentId, uint32_t blockNumber)
{   
    NS_LOG_FUNCTION (this);
    if((m_used_space + m_block_size) > GetAvailableSpace()) throw DiskFullException();
    if(m_stored_content[contentId] < blockNumber) m_stored_content[contentId]++;
}

void NuoxusCacheManager::Delete(uint32_t contentId, uint64_t size)
{
    NS_LOG_FUNCTION (this);
    m_used_space -= size;
    m_stored_content[contentId] = 0;    
}

uint64_t NuoxusCacheManager::GetAvailableSpace()
{
    NS_LOG_FUNCTION (this); 
   return m_space_in_disk_in_kilobytes - m_used_space;
}

void NuoxusCacheManager::ReserveSpace(uint64_t space)
{
    m_used_space += space;
}

void NuoxusCacheManager::SetDiskSize(uint64_t diskSizeInKb)
{
    m_space_in_disk_in_kilobytes = diskSizeInKb;
}

}

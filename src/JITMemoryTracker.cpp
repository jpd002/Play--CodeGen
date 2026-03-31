#include "JITMemoryTracker.h"
#include <sys/mman.h>

namespace CodeGen
{

	void JITMemoryTracker::RegisterJITRegion(void* ptr, size_t size)
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		m_regions[ptr] = {ptr, size, 0};
	}

	void JITMemoryTracker::UnregisterJITRegion(void* ptr)
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		m_regions.erase(ptr);
	}

	JITMemoryTracker::JITRegion* JITMemoryTracker::FindRegion(void* ptr)
	{
		for(auto& pair : m_regions)
		{
			JITRegion& region = pair.second;
			if(ptr >= region.start &&
			   ptr < static_cast<char*>(region.start) + region.size)
			{
				return &region;
			}
		}
		return nullptr;
	}

	void JITMemoryTracker::JITRegionWriteEnableExecuteDisable(void* ptr)
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		JITRegion* region = FindRegion(ptr);
		if(region)
		{
			if(region->nesting_counter == 0)
			{
				mprotect(region->start, region->size, PROT_READ | PROT_WRITE);
			}
			region->nesting_counter++;
		}
	}

	void JITMemoryTracker::JITRegionWriteDisableExecuteEnable(void* ptr)
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		JITRegion* region = FindRegion(ptr);
		if(region)
		{
			region->nesting_counter--;
			if(region->nesting_counter == 0)
			{
				mprotect(region->start, region->size, PROT_READ | PROT_EXEC);
			}
		}
	}

} // namespace CodeGen

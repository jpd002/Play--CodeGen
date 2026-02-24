#pragma once

#include <cstddef>
#include <map>
#include <mutex>

namespace CodeGen
{

	class JITMemoryTracker
	{
	public:
		void RegisterJITRegion(void* ptr, size_t size);
		void UnregisterJITRegion(void* ptr);
		void JITRegionWriteEnableExecuteDisable(void* ptr);
		void JITRegionWriteDisableExecuteEnable(void* ptr);

	private:
		struct JITRegion
		{
			void* start;
			size_t size;
			int nesting_counter; // Support appels imbriqués
		};

		std::map<void*, JITRegion> m_regions;
		std::mutex m_mutex;

		JITRegion* FindRegion(void* ptr);
	};

} // namespace CodeGen

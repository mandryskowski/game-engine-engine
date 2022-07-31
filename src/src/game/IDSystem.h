#pragma once
#include <atomic>
#include <utility/Utility.h>

namespace GEE
{
	typedef uint64_t GEEID;
	template <typename T>
	class IDSystem
	{
	public:
		IDSystem() = delete;
		static GEEID GenerateID()
		{
			static std::atomic_uint64_t idCounter = 1;
			return static_cast<GEEID>(idCounter++);
		}
	private:
	};
}
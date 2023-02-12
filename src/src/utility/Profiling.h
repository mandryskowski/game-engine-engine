#pragma once
#include <deque>
#include <string>
#include <functional>
#include <utility/Utility.h>

namespace GEE
{

	class Profiling
	{
	public:
		Profiling();
	
		void StartProfiling(Time currentTime);
		bool HasBeenStarted() const;
		void AddTime(Time frameTime);
		Time GetStartTime() const { return StartTime; }

		String GetProfilingTitle() const { if (!GetTitleFunc) return nullptr;  return GetTitleFunc(); }
		void SetProfilingTitleFunc(const String& profilingTitle) { GetTitleFunc = [=]() { return profilingTitle; }; }
		void SetProfilingTitleFunc(std::function<String()> titleFunc) { GetTitleFunc = titleFunc; }

		void SetCurrentUsageOfGPU(float usage);
		void StopAndSaveToFile(Time currentTime, const String& filenme = "profiling.txt");
	private:
		std::deque<std::pair<Time, Time>> KeyFrames;
		Time StartTime;
		Time SmoothedAverage;
		float CurrentUsageOfGPU;
		std::function<std::string()> GetTitleFunc;
	};
}
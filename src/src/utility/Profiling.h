#pragma once
#include <deque>
#include <string>
#include <functional>

namespace GEE
{
	typedef float Time;

	class Profiling
	{
	public:
		Profiling();
	
		void StartProfiling(Time currentTime);
		bool HasBeenStarted() const;
		void AddTime(Time frameTime);
		Time GetStartTime() const { return StartTime; }

		std::string GetProfilingTitle() const { if (!GetTitleFunc) return nullptr;  return GetTitleFunc(); }
		void SetProfilingTitleFunc(const std::string& profilingTitle) { GetTitleFunc = [=]() { return profilingTitle; }; }
		void SetProfilingTitleFunc(std::function<std::string()> titleFunc) { GetTitleFunc = titleFunc; }

		void SetCurrentUsageOfGPU(float usage);
		void StopAndSaveToFile(Time currentTime, const std::string& filenme = "profiling.txt");
	private:
		std::deque<std::pair<Time, Time>> KeyFrames;
		Time StartTime;
		Time SmoothedAverage;
		float CurrentUsageOfGPU;
		std::function<std::string()> GetTitleFunc;
	};
}
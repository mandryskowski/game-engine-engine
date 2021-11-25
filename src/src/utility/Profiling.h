#pragma once
#include <deque>
#include <string>

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
		void StopAndSaveToFile(Time currentTime, const std::string& filenme = "profiling.txt");
	private:
		std::deque<Time> FrameTimes;
		Time StartTime;
		Time SmoothedAverage;
	};
}
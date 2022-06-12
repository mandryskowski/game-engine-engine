#include <utility/Profiling.h>
#include <utility/Log.h>
#include <fstream>
#include <chrono>
#include <ctime>

namespace GEE
{
	Profiling::Profiling() :
		StartTime(-1.0f),
		CurrentUsageOfGPU(0.0f)
	{
	}

	void Profiling::StartProfiling(Time currentTime)
	{
		StartTime = currentTime;
		SmoothedAverage = 0.0f;
		KeyFrames.clear();
	}

	bool Profiling::HasBeenStarted() const
	{
		return StartTime >= 0.0f;
	}

	void Profiling::AddTime(Time frameTime)
	{
		//if (!KeyFrames.empty())
			//SmoothedAverage = frameTime * 0.9f + KeyFrames.back() * 0.1f;

		//std::cout << SmoothedAverage << '\n';
		KeyFrames.push_back(std::pair<Time, Time>(frameTime, CurrentUsageOfGPU));
	}

	void Profiling::SetCurrentUsageOfGPU(float usage)
	{
		CurrentUsageOfGPU = usage;
	}

	void Profiling::StopAndSaveToFile(Time currentTime, const std::string& filename)
	{
		std::fstream file(filename, std::ios::app);

		file << "=============\n";
		
		if (GetTitleFunc)
		file << GetTitleFunc() << '\n';

		std::time_t systemTime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

		// Change delimiter of ctime (\n) to end of const char delimiter (\0)
		std::string systemTimeStr = std::ctime(&systemTime);

		file << systemTimeStr;
		
		for (auto keyframe : KeyFrames)
			file << keyframe.first << ' ' << keyframe.second << '\n';

		file.close();

		StartTime = -1.0f;
		KeyFrames.clear();
	}
}
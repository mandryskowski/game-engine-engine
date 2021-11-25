#include <utility/Profiling.h>
#include <utility/Log.h>
#include <fstream>

namespace GEE
{
	Profiling::Profiling() :
		StartTime(-1.0f)
	{
	}

	void Profiling::StartProfiling(Time currentTime)
	{
		StartTime = currentTime;
		SmoothedAverage = 0.0f;
		FrameTimes.clear();
	}

	bool Profiling::HasBeenStarted() const
	{
		return StartTime >= 0.0f;
	}

	void Profiling::AddTime(Time frameTime)
	{
		if (!FrameTimes.empty())
			SmoothedAverage = frameTime * 0.9f + FrameTimes.back() * 0.1f;

		std::cout << SmoothedAverage << '\n';
		FrameTimes.push_back(SmoothedAverage);
	}

	void Profiling::StopAndSaveToFile(Time currentTime, const std::string& filename)
	{
		std::fstream file(filename, std::ios::app);
		
		for (auto time : FrameTimes)
			file << time << '\n';

		file.close();

		StartTime = -1.0f;
		FrameTimes.clear();
	}
}
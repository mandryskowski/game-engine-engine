#pragma once

namespace GEE
{
	class FPSCounter
	{
	public:
		FPSCounter() : PrevFPSUpdateTime(0.0f), FrameCount(0), LastFPSReading(0.0f) {}
		void CountFrame() { FrameCount++; }
		void Reset(float currTime)
		{
			LastFPSReading = (currTime != PrevFPSUpdateTime) ? (static_cast<float>(FrameCount) / currTime - PrevFPSUpdateTime) : (0.0f);
			FrameCount = 0;
			PrevFPSUpdateTime = currTime;
		}

		float GetLastFPSReading() const { return LastFPSReading; }

	private:
		float PrevFPSUpdateTime, LastFPSReading;
		unsigned int FrameCount;
	};
}
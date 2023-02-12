#pragma once
#include <iostream>
#include <chrono>
#include <ctime>

template <typename Arg,typename...	Args>
void GEE_LOG(Arg&& arg, Args&&... args)
{
	const std::time_t currentTime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

	constexpr unsigned int bufferSize = 80;
	tm newTime;
	char buffer[bufferSize];

	localtime_s(&newTime, &currentTime);

	// Change delimiter of timeStr (\n) to end of const char delimiter (\0)
	std::string timeStr(buffer, std::strftime(buffer, bufferSize, "%Y-%m-%d-%H:%M:%S", &newTime));
	timeStr.pop_back();

	std::cout << timeStr << "> ";
	std::cout << std::forward<Arg>(arg);
	((std::cout << ' ' << std::forward<Args>(args)), ...);
	std::cout << '\n';
}
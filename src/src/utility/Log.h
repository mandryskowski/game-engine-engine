#pragma once
#include <iostream>
#include <chrono>
#include <ctime>

template <typename Arg,typename...	Args>
void GEE_LOG(Arg&& arg, Args&&... args)
{
	std::time_t currentTime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

	std::cout << std::ctime(&currentTime) << ">";
	std::cout << std::forward<Arg>(arg);
	((std::cout << ' ' << std::forward<Args>(args)), ...);
	std::cout << '\n';
}
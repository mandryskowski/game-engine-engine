#pragma once
#include "Event.h"

class InputDevicesStateRetriever
{
public:
	InputDevicesStateRetriever(GLFWwindow&);
	bool IsKeyPressed(const Key&) const;

	friend class Game;
private:
	GLFWwindow& WindowRef;
};
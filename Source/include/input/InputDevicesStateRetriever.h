#pragma once
#include "Event.h"

class InputDevicesStateRetriever
{
public:
	InputDevicesStateRetriever(GLFWwindow&);
	glm::dvec2 GetMousePosition() const;
	glm::dvec2 GetMousePositionNDC() const;
	bool IsKeyPressed(const Key&) const;

	friend class Game;
private:
	GLFWwindow& WindowRef;
};
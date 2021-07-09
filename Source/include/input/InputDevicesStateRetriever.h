#pragma once
#include "Event.h"

namespace GEE
{
	class InputDevicesStateRetriever
	{
	public:
		InputDevicesStateRetriever(GLFWwindow&);
		Vec2d GetMousePosition() const;
		Vec2d GetMousePositionNDC() const;
		bool IsKeyPressed(const Key&) const;

		friend class Game;
	private:
		GLFWwindow& WindowRef;
	};
}
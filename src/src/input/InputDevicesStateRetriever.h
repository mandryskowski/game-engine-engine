#pragma once
#include "Event.h"

namespace GEE
{
	class InputDevicesStateRetriever
	{
	public:
		InputDevicesStateRetriever(SystemWindow&);
		bool IsKeyPressed(const Key) const;
		bool IsMouseButtonPressed(MouseButton) const;

		friend class Game;
	private:
		SystemWindow& WindowRef;
	};
}
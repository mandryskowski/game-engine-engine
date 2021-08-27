#pragma once
#include "Event.h"

namespace GEE
{
	class InputDevicesStateRetriever
	{
	public:
		InputDevicesStateRetriever(SystemWindow&);
		Vec2d GetMousePositionPx() const;
		Vec2d GetMousePositionNDC() const;
		bool IsKeyPressed(const Key&) const;

		friend class Game;
	private:
		Vec2i GetWindowSize() const;
		SystemWindow& WindowRef;
	};

	namespace pxConversion
	{
		Vec2d PxToNDC(Vec2d pos, Vec2i windowSize);
	}
}
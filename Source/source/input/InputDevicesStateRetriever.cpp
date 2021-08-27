#include <input/InputDevicesStateRetriever.h>

namespace GEE
{
	InputDevicesStateRetriever::InputDevicesStateRetriever(SystemWindow& window) :
		WindowRef(window)
	{
	}

	Vec2d InputDevicesStateRetriever::GetMousePositionPx() const
	{
		Vec2d mousePos;
		glfwGetCursorPos(&WindowRef, &mousePos.x, &mousePos.y);
		mousePos.y = GetWindowSize().y - mousePos.y;

		return mousePos;
	}

	Vec2d InputDevicesStateRetriever::GetMousePositionNDC() const
	{
		return pxConversion::PxToNDC(GetMousePositionPx(), GetWindowSize());
	}

	bool InputDevicesStateRetriever::IsKeyPressed(const Key& k) const
	{
		return glfwGetKey(&WindowRef, static_cast<int>(k)) == GLFW_PRESS;
	}

	Vec2i InputDevicesStateRetriever::GetWindowSize() const
	{
		Vec2i windowSize;
		glfwGetWindowSize(&WindowRef, &windowSize.x, &windowSize.y);
		return windowSize;
	}

	Vec2d pxConversion::PxToNDC(Vec2d pos, Vec2i windowSize)
	{
		Vec2d mousePos = pos / static_cast<Vec2d>(windowSize);
		return mousePos * 2.0 - 1.0;
	}


}
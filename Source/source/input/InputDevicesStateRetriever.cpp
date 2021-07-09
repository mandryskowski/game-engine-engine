#include <input/InputDevicesStateRetriever.h>

namespace GEE
{
	InputDevicesStateRetriever::InputDevicesStateRetriever(GLFWwindow& window) :
		WindowRef(window)
	{
	}

	Vec2d InputDevicesStateRetriever::GetMousePosition() const
	{
		Vec2d mousePos;
		glfwGetCursorPos(&WindowRef, &mousePos.x, &mousePos.y);

		return mousePos;
	}

	Vec2d InputDevicesStateRetriever::GetMousePositionNDC() const
	{
		glm::ivec2 windowSize;
		glfwGetWindowSize(&WindowRef, &windowSize.x, &windowSize.y);

		Vec2d mousePosition = GetMousePosition() / static_cast<Vec2d>(windowSize);
		mousePosition.y = 1.0 - mousePosition.y;

		return mousePosition * 2.0 - 1.0;
	}

	bool InputDevicesStateRetriever::IsKeyPressed(const Key& k) const
	{
		return glfwGetKey(&WindowRef, static_cast<int>(k)) == GLFW_PRESS;
	}

}
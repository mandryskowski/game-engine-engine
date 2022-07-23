#include <input/InputDevicesStateRetriever.h>

namespace GEE
{
	InputDevicesStateRetriever::InputDevicesStateRetriever(SystemWindow& window) :
		WindowRef(window)
	{
	}

	bool InputDevicesStateRetriever::IsMouseButtonPressed(MouseButton button) const
	{
		return glfwGetMouseButton(&WindowRef, static_cast<int>(button));
	}

	bool InputDevicesStateRetriever::IsKeyPressed(const Key k) const
	{
		return glfwGetKey(&WindowRef, static_cast<int>(k)) == GLFW_PRESS;
	}
}
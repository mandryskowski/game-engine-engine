#include <input/InputDevicesStateRetriever.h>

namespace GEE
{
	InputDevicesStateRetriever::InputDevicesStateRetriever(SystemWindow& window) :
		WindowRef(window)
	{
	}

	bool InputDevicesStateRetriever::IsKeyPressed(const Key& k) const
	{
		return glfwGetKey(&WindowRef, static_cast<int>(k)) == GLFW_PRESS;
	}
}
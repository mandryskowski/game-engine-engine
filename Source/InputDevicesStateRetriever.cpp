#include "InputDevicesStateRetriever.h"
InputDevicesStateRetriever::InputDevicesStateRetriever(GLFWwindow& window) :
	WindowRef(window)
{
}

bool InputDevicesStateRetriever::IsKeyPressed(const Key& k) const
{
	return glfwGetKey(&WindowRef, static_cast<int>(k)) == GLFW_PRESS;
}

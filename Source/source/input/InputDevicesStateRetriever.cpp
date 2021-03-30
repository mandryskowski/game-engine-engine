#include <input/InputDevicesStateRetriever.h>
InputDevicesStateRetriever::InputDevicesStateRetriever(GLFWwindow& window) :
	WindowRef(window)
{
}

glm::dvec2 InputDevicesStateRetriever::GetMousePosition() const
{
	glm::dvec2 mousePos;
	glfwGetCursorPos(&WindowRef, &mousePos.x, &mousePos.y);

	return mousePos;
}

glm::dvec2 InputDevicesStateRetriever::GetMousePositionNDC() const
{
	glm::ivec2 windowSize;
	glfwGetWindowSize(&WindowRef, &windowSize.x, &windowSize.y);

	glm::dvec2 mousePosition = GetMousePosition() / static_cast<glm::dvec2>(windowSize);
	mousePosition.y = 1.0 - mousePosition.y;

	return mousePosition * 2.0 - 1.0;
}

bool InputDevicesStateRetriever::IsKeyPressed(const Key& k) const
{
	return glfwGetKey(&WindowRef, static_cast<int>(k)) == GLFW_PRESS;
}

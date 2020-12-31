#include "Viewport.h"
#include <glad/glad.h>
#include "Utility.h"

Viewport::Viewport(const glm::uvec2& pos, const glm::uvec2& size) :
	Position(pos), Size(size)
{

}
const glm::uvec2& Viewport::GetSize() const
{
	return Size;
}
void Viewport::SetOpenGLState() const
{
	glViewport(Position.x, Position.y, Size.x, Size.y);
}
NDCViewport Viewport::ToNDCViewport(const glm::uvec2& res) const
{
	return NDCViewport(static_cast<glm::vec2>(Position) / static_cast<glm::vec2>(res), static_cast<glm::vec2>(Size) / static_cast<glm::vec2>(res));
}



NDCViewport::NDCViewport(const glm::vec2& pos, const glm::vec2& size) :
	Position(pos), Size(size)
{

}
const glm::vec2& NDCViewport::GetSize() const
{
	return Size;
}
void NDCViewport::SetOpenGLState(const glm::uvec2& res) const
{
	ToPxViewport(res).SetOpenGLState();
}
Viewport NDCViewport::ToPxViewport(const glm::uvec2& res) const
{
	return Viewport((Position + glm::vec2(1.0f)) * static_cast<glm::vec2>(res) / 2.0f, Size * static_cast<glm::vec2>(res));
}
#include <rendering/Viewport.h>
#include <glad/glad.h>
#include <utility/Utility.h>

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


NDCViewport::NDCViewport(glm::vec2 pos, glm::vec2 size):
	Boxf<Vec2f>(pos, size)
{
	if (Position != glm::vec2(-1.0f) && !NDCViewport(glm::vec2(-1.0f), glm::vec2(2.0f)).Contains(Position))
	{
		std::cout << "ERROR! NDCViewport at (" << Position.x << ", " << Position.y << ") is out of window bounds (1.0)\n";
		Position = glm::vec2(0.0f);
		Size = glm::vec2(0.0f);
	}
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

bool NDCViewport::Contains(const glm::vec2& point) const
{
	return Boxf<Vec2f>(Position + Size, Size).Contains(point);
}

bool NDCViewport::Contains(const Boxf<Vec2f>& box) const
{
	return Boxf<Vec2f>(Position + Size, Size / 2.0f).Contains(box);
}

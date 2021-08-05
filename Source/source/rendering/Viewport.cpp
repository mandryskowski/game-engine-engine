#include <rendering/Viewport.h>
#include <glad/glad.h>

namespace GEE
{
	Viewport::Viewport(const Vec2u& pos, const Vec2u& size) :
		Position(pos), Size(size)
	{

	}
	Viewport::Viewport(const Vec2u& size):
		Viewport(Vec2u(0), size)
	{
	}
	const Vec2u& Viewport::GetSize() const
	{
		return Size;
	}
	void Viewport::SetOpenGLState() const
	{
		glViewport(Position.x, Position.y, Size.x, Size.y);
	}
	void Viewport::SetScissor() const
	{
		glScissor(Position.x, Position.y, Size.x, Size.y);
	}
	NDCViewport Viewport::ToNDCViewport(const Vec2u& res) const
	{
		return NDCViewport(static_cast<Vec2f>(Position) / static_cast<Vec2f>(res), static_cast<Vec2f>(Size) / static_cast<Vec2f>(res));
	}


	NDCViewport::NDCViewport(Vec2f pos, Vec2f size) :
		Boxf<Vec2f>(pos, size)
	{
		if (Position != Vec2f(-1.0f) && !NDCViewport(Vec2f(-1.0f), Vec2f(2.0f)).Contains(Position))
		{
			std::cout << "ERROR! NDCViewport at (" << Position.x << ", " << Position.y << ") is out of window bounds (1.0)\n";
			Position = Vec2f(0.0f);
			Size = Vec2f(0.0f);
		}
	}

	NDCViewport::NDCViewport(Vec2f size):
		NDCViewport(Vec2u(0), size)
	{
	}

	const Vec2f& NDCViewport::GetPosition() const
	{
		return Position;
	}

	const Vec2f& NDCViewport::GetSize() const
	{
		return Size;
	}
	Mat4f NDCViewport::GetOrthoProjectionMatrix() const
	{
		return glm::ortho(Position.x - Size.x, Position.x + Size.x, Position.y - Size.y, Position.y + Size.y, 1.0f, -2550.0f);
	}
	void NDCViewport::SetOpenGLState(const Vec2u& res) const
	{
		ToPxViewport(res).SetOpenGLState();
	}
	Viewport NDCViewport::ToPxViewport(const Vec2u& res) const
	{
		return Viewport((Position + Vec2f(1.0f)) * static_cast<Vec2f>(res) / 2.0f, Size * static_cast<Vec2f>(res));
	}

	bool NDCViewport::Contains(const Vec2f& point) const
	{
		return Boxf(Position + Size, Size).Contains(point);
	}

	bool NDCViewport::Contains(const Boxf<Vec2f>& box) const
	{
		return Boxf(Position + Size, Size / 2.0f).Contains(box);
	}

}
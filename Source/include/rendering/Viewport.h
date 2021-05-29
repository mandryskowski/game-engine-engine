#pragma once
#include <math/Box.h>

namespace GEE
{
	class NDCViewport;

	class Viewport
	{
	public:
		Viewport(const glm::uvec2& pos, const glm::uvec2& size);
		const glm::uvec2& GetSize() const;
		void SetOpenGLState() const;
		NDCViewport ToNDCViewport(const glm::uvec2& res) const;


	private:
		glm::uvec2 Position;
		glm::uvec2 Size;
	};

	class NDCViewport : protected Boxf<Vec2f>
	{
	public:
		NDCViewport(glm::vec2 pos, glm::vec2 size);

		const glm::vec2& GetPosition() const;
		const glm::vec2& GetSize() const;

		void SetOpenGLState(const glm::uvec2& res) const;
		Viewport ToPxViewport(const glm::uvec2& res) const;

		bool Contains(const glm::vec2&) const;
		bool Contains(const Boxf<Vec2f>&) const;
	};
}
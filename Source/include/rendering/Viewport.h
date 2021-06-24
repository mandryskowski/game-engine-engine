#pragma once
#include <math/Box.h>

namespace GEE
{
	class NDCViewport;

	class Viewport
	{
	public:
		Viewport(const Vec2u& pos, const Vec2u& size);
		Viewport(const Vec2u& size);
		const Vec2u& GetSize() const;
		void SetOpenGLState() const;
		NDCViewport ToNDCViewport(const Vec2u& res) const;


	private:
		Vec2u Position;
		Vec2u Size;
	};

	class NDCViewport : protected Boxf<Vec2f>
	{
	public:
		NDCViewport(Vec2f pos, Vec2f size);
		NDCViewport(Vec2f size);

		const Vec2f& GetPosition() const;
		const Vec2f& GetSize() const;

		void SetOpenGLState(const Vec2u& res) const;
		Viewport ToPxViewport(const Vec2u& res) const;

		bool Contains(const Vec2f&) const;
		bool Contains(const Boxf<Vec2f>&) const;
	};
}
#include <math/Vec.h>

namespace GEE
{
	Vec2f Math::GetRatioOfComponents(const Vec2f& vec)
	{
		return glm::min(Vec2f(vec.y / vec.x, vec.x / vec.y), Vec2f(1.0f));
	}
}

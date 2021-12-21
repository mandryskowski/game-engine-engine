#include <math/Box.h>
#include <math/Transform.h>


namespace GEE
{
	template <typename VecType> Boxf<VecType>::Boxf(const VecType& pos, const VecType& size) :
		Position(pos), Size(size)
	{
	}

	template <typename VecType> Boxf<VecType>::Boxf(const Transform& t) :
		Position(t.GetPos()),
		Size(t.GetScale())
	{
	}

	template <typename VecType> float Boxf<VecType>::GetRight() const
	{
		return Position.x + Size.x;
	}

	template <typename VecType> float Boxf<VecType>::GetLeft() const
	{
		return Position.x - Size.x;
	}

	template <typename VecType> float Boxf<VecType>::GetTop() const
	{
		return Position.y + Size.y;
	}

	template <typename VecType> float Boxf<VecType>::GetBottom() const
	{
		return Position.y - Size.y;
	}

	template <typename VecType> bool Boxf<VecType>::Contains(const VecType& point) const
	{
		VecType thisRightUp = Position + Size;
		VecType thisLeftDown = Position - Size;

		return GeomTests::Contains(point, thisLeftDown, thisRightUp);
	}

	template <typename VecType> bool Boxf<VecType>::Contains(const Boxf<VecType>& rhs) const
	{
		VecType thisRightUp = Position + Size;
		VecType thisLeftDown = Position - Size;

		VecType rhsRightUp = rhs.Position + rhs.Size;
		VecType rhsLeftDown = rhs.Position - rhs.Size;

		return GeomTests::Contains(thisLeftDown, thisRightUp, rhsLeftDown, rhsRightUp);
	}

	template<typename VecType>
	Boxf<VecType> Boxf<VecType>::FromMinMaxCorners(const VecType& minCorner, const VecType& maxCorner)
	{
		return Boxf<VecType>((minCorner + maxCorner) / 2.0f, (maxCorner - minCorner) / 2.0f);
	}

	template class Boxf<Vec2f>;
	template class Boxf<Vec3f>;
}
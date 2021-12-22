#pragma once

namespace GEE
{
	enum class Alignment
	{
		Left,
		Center,
		Right,
		Bottom = Left,
		Top = Right
	};

	class Alignment2D
	{
	public:
		Alignment2D(Alignment horizontal, Alignment vertical = Alignment::Center) : Horizontal(horizontal), Vertical(vertical) {}

		Alignment GetHorizontal() const { return Horizontal; }
		Alignment GetVertical() const { return Vertical; }

		void SetHorizontal(Alignment alignment) { Horizontal = alignment; }
		void SetVertical(Alignment alignment) { Vertical = alignment; }

		static Alignment2D LeftTop() { return Alignment2D(Alignment::Left, Alignment::Top); }
		static Alignment2D Top() { return Alignment2D(Alignment::Center, Alignment::Top); }
		static Alignment2D RightTop() { return Alignment2D(Alignment::Right, Alignment::Top); }
			   
		static Alignment2D LeftCenter() { return Alignment2D(Alignment::Left, Alignment::Center); }
		static Alignment2D Center() { return Alignment2D(Alignment::Center, Alignment::Center); }
		static Alignment2D RightCenter() { return Alignment2D(Alignment::Right, Alignment::Center); }

		static Alignment2D LeftBottom() { return Alignment2D(Alignment::Left, Alignment::Bottom); }
		static Alignment2D Bottom() { return Alignment2D(Alignment::Center, Alignment::Bottom); }
		static Alignment2D RightBottom() { return Alignment2D(Alignment::Right, Alignment::Bottom); }

	private:
		Alignment Horizontal, Vertical;
	};
}
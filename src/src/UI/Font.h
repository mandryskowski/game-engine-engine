#pragma once
#include <math/Vec.h>
#include <vector>
#include <rendering/Texture.h>

namespace GEE
{

	struct Character
	{
		unsigned int ID;
		Vec2f Size;
		Vec2f Bearing;
		float Advance;
	};

	class Font
	{
		Texture BitmapsArray;
		float BaselineHeight;
		std::vector<Character> Characters;

		std::string Path;

	public:
		Font(const std::string& path);
		Texture GetBitmapsArray() const;
		float GetBaselineHeight() const;
		const std::string& GetPath() const;
		const Character& GetCharacter(unsigned int index) const;
		void SetBaselineHeight(float height);
		void AddCharacter(const Character&);
	};
}
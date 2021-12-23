#pragma once
#include <math/Vec.h>
#include <vector>
#include <rendering/Texture.h>
#include <map>
#include <utility/Utility.h>
#include <game/GameManager.h>

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
	public:
		class Variation
		{
			Texture BitmapsArray;
			float BaselineHeight;
			std::vector<Character> Characters;

			std::string Path;
		public:
			Variation(const std::string& path);
			Texture GetBitmapsArray() const;
			float GetBaselineHeight() const;
			const std::string& GetPath() const;
			const Character& GetCharacter(unsigned int index) const;
			void SetBaselineHeight(float height);
			void AddCharacter(const Character&);
		};

		Font(const std::string& regularPath, const std::string& boldPath = "", const std::string& italicPath = "", const std::string& boldItalicPath = "");
		Variation* GetVariation(FontStyle);
		const Variation* GetVariation(FontStyle) const;

		operator Variation&() const { return *Variations.at(FontStyle::Regular); }
	private:
		std::map<FontStyle, SharedPtr<Variation>> Variations;
	};
}
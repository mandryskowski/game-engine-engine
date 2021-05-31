#include <UI/Font.h>
#include <iostream>

namespace GEE
{
	Font::Font(const std::string& path) :
		Path(path)
	{
		BitmapsArray = *reserveTexture(glm::uvec3(64, 64, 128), GL_RED, GL_UNSIGNED_BYTE, GL_LINEAR, GL_LINEAR_MIPMAP_LINEAR, GL_TEXTURE_2D_ARRAY, 0, "undefinedFontBitmap", GL_RED);
		std::cout << BitmapsArray.GetID() << ".\n";
	}

	Texture Font::GetBitmapsArray() const
	{
		return BitmapsArray;
	}

	float Font::GetBaselineHeight() const
	{
		return BaselineHeight;
	}

	const std::string& Font::GetPath() const
	{
		return Path;
	}

	const Character& Font::GetCharacter(unsigned int index) const
	{
		return Characters[index];
	}

	void Font::SetBaselineHeight(float height)
	{
		BaselineHeight = height;
	}

	void Font::AddCharacter(const Character& character)
	{
		Characters.push_back(character);
	}

}
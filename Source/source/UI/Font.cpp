#include <UI/Font.h>
#include <iostream>

namespace GEE
{
	Font::Font(const std::string& path) :
		Path(path)
	{
		BitmapsArray = Texture::Loader::ReserveEmpty2DArray(Vec3u(64, 64, 128), GL_RED);
		BitmapsArray.SetMinFilter(Texture::MinTextureFilter::Trilinear());
		BitmapsArray.SetMagFilter(Texture::MagTextureFilter::Bilinear());
		BitmapsArray.SetWrap(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, 0, true);

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
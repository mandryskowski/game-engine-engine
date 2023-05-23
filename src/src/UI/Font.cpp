#include <UI/Font.h>
#include <iostream>
#include <utility/Asserts.h>

namespace GEE
{
	Font::Variation::Variation(const std::string& path):
		BaselineHeight(0.0f),
		Path(path)
	{
		BitmapsArray = Texture::Loader<>::ReserveEmpty2DArray(Vec3u(64, 64, 128), Texture::Format::Red());
		BitmapsArray.SetMinFilter(Texture::MinFilter::Trilinear());
		BitmapsArray.SetMagFilter(Texture::MagFilter::Bilinear());
		BitmapsArray.SetWrap(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, 0, true);

		std::cout << BitmapsArray.GetID() << ".\n";
	}

	Texture Font::Variation::GetBitmapsArray() const
	{
		return BitmapsArray;
	}

	float Font::Variation::GetBaselineHeight() const
	{
		return BaselineHeight;
	}

	const std::string& Font::Variation::GetPath() const
	{
		return Path;
	}

	const Character& Font::Variation::GetCharacter(unsigned int index) const
	{
		GEE_CORE_ASSERT(index < Characters.size());
		return Characters[index];
	}

	void Font::Variation::SetBaselineHeight(float height)
	{
		BaselineHeight = height;
	}

	void Font::Variation::AddCharacter(const Character& character)
	{
		Characters.push_back(character);
	}

	Font::Font(const std::string& regularPath, const std::string& boldPath, const std::string& italicPath, const std::string& boldItalicPath)
	{
		Variations[FontStyle::Regular] = MakeShared<Variation>(regularPath);
		Variations[FontStyle::Bold] = MakeShared<Variation>(boldPath);
		Variations[FontStyle::Italic] = MakeShared<Variation>(italicPath);
		Variations[FontStyle::BoldItalic] = MakeShared<Variation>(boldItalicPath);
	}

	Font::Variation* Font::GetVariation(FontStyle type)
	{
		return Variations[type].get();
	}

	const Font::Variation* Font::GetVariation(FontStyle type) const
	{
		return Variations.at(type).get();
	}
}
#include <rendering/RenderInfo.h>
#include <rendering/Material.h>

namespace GEE
{
	void MatrixInfoExt::DefaultBools()
	{
		bUseMaterials = true;
		bOnlyShadowCasters = false;
		bMainPass = false;
		bAllowBlending = true;
	}
}
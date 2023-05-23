#include <rendering/RenderInfo.h>
#include <rendering/material/Material.h>

namespace GEE
{
	void MatrixInfoExt::DefaultBools()
	{
		bUseMaterials = true;
		bOnlyShadowCasters = false;
		bMainPass = false;
		bAllowBlending = true;
	}

	void MatrixInfoExt::SetRequiredShaderInfo(ShaderInfo info)
	{
		RequiredShaderInfo = info;
	}

	void MatrixInfoExt::StopRequiringShaderInfo()
	{
		SetRequiredShaderInfo
			(ShaderInfo(MaterialShaderHint::None));
	}
}

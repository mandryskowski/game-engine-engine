#include "ShaderInfo.h"
#include <cereal/archives/json.hpp>
#include <rendering/RenderToolbox.h>

#include "game/GameManager.h"
#include "rendering/RenderEngineManager.h"
#include "rendering/Shader.h"

using namespace GEE;


Shader* ShaderInfo::RetrieveShaderForRendering(RenderToolboxCollection& tbCol)
{
	if (CustomShader)
		return CustomShader;

	return tbCol.GetShaderFromHint(GetShaderHint());
}

template <typename Archive>
void ShaderInfo::Save(Archive& archive) const
{
	archive(CEREAL_NVP(ShaderHint), cereal::make_nvp("CustomShaderName",
		(CustomShader)
		? (CustomShader->GetName())
		: (std::string())));
}

template <typename Archive>
void ShaderInfo::Load(Archive& archive)
{
	std::string customShaderName;
	archive(CEREAL_NVP(ShaderHint), customShaderName);

	/////TODO: This dirty hack should be refactored
	if (!customShaderName.empty())
		CustomShader = GameManager::Get().GetRenderEngineHandle()->FindShader(customShaderName);
}

template <typename Archive>
void ShaderInfo::load_and_construct(Archive& archive, cereal::construct<ShaderInfo>& construct)
{
	construct(MaterialShaderHint::None);
	construct->Load(archive);
}

template void ShaderInfo::Save<cereal::JSONOutputArchive>(cereal::JSONOutputArchive&) const;
template void ShaderInfo::Load<cereal::JSONInputArchive>(cereal::JSONInputArchive&);
template void ShaderInfo::load_and_construct<cereal::JSONInputArchive>(cereal::JSONInputArchive&, cereal::construct<ShaderInfo>&);

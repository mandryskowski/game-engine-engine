#include "GEditorSettings.h"
#include <cereal/archives/json.hpp>

namespace GEE
{
	namespace Editor
	{
		GEditorSettings::GEditorSettings() :
			bRenderGrid(true), bDebugRenderComponents(true), bDebugRenderPhysicsMeshes(true) {}

		template<typename Archive>
		void GEditorSettings::Serialize(Archive& archive)
		{
			archive(CEREAL_NVP(bRenderGrid), CEREAL_NVP(bDebugRenderComponents), CEREAL_NVP(bDebugRenderPhysicsMeshes), cereal::make_nvp("GameSettings", cereal::base_class<GameSettings>(this)));
		}

		template void GEditorSettings::Serialize<cereal::JSONOutputArchive>(cereal::JSONOutputArchive&);
		template void GEditorSettings::Serialize<cereal::JSONInputArchive>(cereal::JSONInputArchive&);
	}
}
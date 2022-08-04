#pragma once
#include <game/GameSettings.h>

namespace GEE
{
	namespace Editor
	{
		struct GEditorSettings : public GameSettings
		{
			bool bRenderGrid, bDebugRenderComponents, bDebugRenderPhysicsMeshes;
			bool bViewportMaximized;

			GEditorSettings();

			template <typename Archive> void Serialize(Archive& archive);
		};
	}
}

GEE_REGISTER_TYPE(GEE::Editor::GEditorSettings);
GEE_REGISTER_POLYMORPHIC_RELATION(GEE::GameSettings, GEE::Editor::GEditorSettings);
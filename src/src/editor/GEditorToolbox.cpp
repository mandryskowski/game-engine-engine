#include "GEditorToolbox.h"
#include <rendering/Shader.h>

namespace GEE
{
	namespace Editor
	{
		GEditorToolbox::GEditorToolbox(ShaderFromHintGetter& getter, const GameSettings::VideoSettings& settings) :
			RenderToolbox(getter),
			GridShader(nullptr)
		{
			Setup(settings);
		}

		void GEditorToolbox::Setup(const GameSettings::VideoSettings& settings)
		{
			GridShader = AddShader(ShaderLoader::LoadShaders("GEditor_Grid", "Shaders/UI/grid.vs", "Shaders/UI/grid.fs"));
		}
	}
}
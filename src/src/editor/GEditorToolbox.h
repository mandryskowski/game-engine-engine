#pragma once
#include <rendering/RenderToolbox.h>
namespace GEE
{
	namespace Editor
	{
		class GEditorToolbox : public RenderToolbox
		{
		public:
			GEditorToolbox(ShaderFromHintGetter&, const GameSettings::VideoSettings& settings);
			void Setup(const GameSettings::VideoSettings&);
		private:
			Shader* GridShader;
			friend class Editor::EditorRenderer;
		};

		class GEditorRenderToolboxCollection : public RenderToolboxCollection
		{
		public:
			using RenderToolboxCollection::RenderToolboxCollection;
			virtual void AddTbsRequiredBySettings() override
			{
				RenderToolboxCollection::AddTbsRequiredBySettings();
				AddTb<GEditorToolbox>();
			}
		};
	}
}

#pragma once
#include <rendering/Renderer.h>
#include <editor/EditorManager.h>

namespace GEE
{
	namespace Editor
	{
		class GEditorRenderToolboxCollection;
		class GEditorSettings;

		class EditorRenderer : public GameRenderer
		{
		public:
			EditorRenderer(EditorManager&, GEditorSettings& editorSettings, RenderToolboxCollection& mainSceneCol, GEditorRenderToolboxCollection& uiCol, SystemWindow& mainWindow);
			virtual void RenderPass(GameScene* mainScene, GameScene* editorScene, GameScene* menuScene, GameScene* meshPreviewScene) override;

			void RenderMainScene(GameScene*);
			void RenderEditorScene(GameScene*);
			void RenderMainMenu(GameScene*);
			void RenderMeshPreview(GameScene*, RenderToolboxCollection&);
			void RenderPopups();
			void RenderGrid(const MatrixInfo&);

		private:
			GEditorSettings& EditorSettings;
			SystemWindow* MainWindow;
			Editor::EditorManager& EditorHandle;
			RenderToolboxCollection* MainSceneCollection;
			GEditorRenderToolboxCollection* UICollection;
		};
	}
}
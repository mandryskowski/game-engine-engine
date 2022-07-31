#pragma once
#include <rendering/Renderer.h>
#include <editor/EditorManager.h>

namespace GEE
{
	namespace Editor
	{
		class EditorRenderer : public GameRenderer
		{
		public:
			EditorRenderer(Editor::EditorManager&, RenderToolboxCollection& mainSceneCol, RenderToolboxCollection& uiCol, SystemWindow& mainWindow);
			virtual void RenderPass(GameScene* mainScene, GameScene* editorScene, GameScene* menuScene, GameScene* meshPreviewScene) override;

			void RenderMainScene(GameScene*);
			void RenderEditorScene(GameScene*);
			void RenderMainMenu(GameScene*);
			void RenderMeshPreview(GameScene*, RenderToolboxCollection&);
			void RenderPopups();
			void RenderGrid(const MatrixInfo&);

		private:
			SystemWindow* MainWindow;
			Editor::EditorManager& EditorHandle;
			RenderToolboxCollection* MainSceneCollection, *UICollection;
		};
	}
}
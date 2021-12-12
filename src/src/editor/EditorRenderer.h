#pragma once
#include <rendering/Renderer.h>
#include <editor/EditorManager.h>

namespace GEE
{
	class EditorRenderer : public Renderer
	{
	public:
		EditorRenderer(Editor::EditorManager&, RenderToolboxCollection& mainSceneCol, RenderToolboxCollection& uiCol, SystemWindow& mainWindow);
		void FullRender(GameScene* mainScene, GameScene* editorScene, GameScene* menuScene);

		void RenderMainMenu(RenderToolboxCollection&);
		void RenderEditorScene(RenderToolboxCollection&);

	private:
		SystemWindow* MainWindow;
		Editor::EditorManager& EditorHandle;
		RenderToolboxCollection *MainSceneCollection, *UICollection;
	};
}
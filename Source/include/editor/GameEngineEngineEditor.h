#pragma once
#include <game/Game.h>
#include <UI/UIListActor.h>

namespace GEE
{
	struct EditorEventProcessor
	{
		static void FileDropCallback(GLFWwindow* window, int count, const char** paths);
		static void Resize(GLFWwindow* window, int width, int height);

		static EditorManager* EditorHandle;
	};

	class GameEngineEngineEditor : public Game, public EditorManager
	{
	public:
		GameEngineEngineEditor(GLFWwindow* window, const GameSettings& settings);
		virtual GameManager* GetGameHandle() override;
		virtual Component* GetSelectedComponent() override;
		virtual GameScene* GetSelectedScene() override;
		virtual std::vector<GameScene*> GetScenes() override;
		GameSettings* GetEditorSettings() override;
		virtual void Init(GLFWwindow* window) override;

		virtual void UpdateGameSettings() override;
		virtual void UpdateEditorSettings() override;

		void SetupEditorScene();
		void SetupMainMenu();
		virtual void HandleEvents() override;

		virtual void SelectComponent(Component* comp, GameScene& editorScene) override;

		virtual void SelectActor(Actor* actor, GameScene& editorScene) override;
		virtual void SelectScene(GameScene* selectedScene, GameScene& editorScene) override;
		virtual void PreviewHierarchyTree(HierarchyTemplate::HierarchyTreeT& tree) override;
		template <typename T> void AddActorToList(GameScene& editorScene, T& obj, UIAutomaticListActor& listParent, UICanvas& canvas);
		virtual void Render() override;
		void LoadProject(const std::string& filepath);
		void SaveProject();

	private:
		void UpdateRecentProjects();

		RenderToolboxCollection* ViewportRenderCollection, * HUDRenderCollection;
		GameScene* EditorScene;


		GameSettings EditorSettings;
		bool bDebugRenderComponents;


		std::string ProjectName, ProjectFilepath;
		std::string ExecutableFolder;

		//	UICanvasActor* CanvasContext;
		std::vector<UICanvasActor*> Canvases;

		Component* SelectedComp;
		Actor* SelectedActor;
		GameScene* SelectedScene;
	};

	template<typename T>
	inline void GameEngineEngineEditor::AddActorToList(GameScene& editorScene, T& obj, UIAutomaticListActor& listParent, UICanvas& canvas)
	{
		if (obj.IsBeingKilled())
			return;

		UIButtonActor& element = listParent.CreateChild<UIButtonActor>(obj.GetName() + "'s Button", [this, &obj, &editorScene]() {this->Select(&obj, editorScene); });//*this, obj, &EditorManager::Select<T>));
		element.SetTransform(Transform(Vec2f(1.5f, 0.0f), Vec2f(3.0f, 1.0f)));
		TextConstantSizeComponent& elementText = element.CreateComponent<TextConstantSizeComponent>(obj.GetName() + "'s Text", Transform(Vec2f(0.0f), Vec2f(0.4f / 3.0f, 0.4f)), "", "", std::pair<TextAlignment, TextAlignment>(TextAlignment::CENTER, TextAlignment::CENTER));
		elementText.SetMaxSize(Vec2f(0.9f));
		elementText.SetContent(obj.GetName());

		std::vector<T*> children = obj.GetChildren();
		for (auto& child : children)
		{
			//AddActorToList(editorScene, *child, listParent.CreateChild(UIListActor(editorScene, "NextLevel")), canvas);
			UIAutomaticListActor& nestedList = listParent.CreateChild<UIAutomaticListActor>(child->GetName() + "'s nestedlist");
			AddActorToList(editorScene, *child, nestedList, canvas);
		}
	}
}
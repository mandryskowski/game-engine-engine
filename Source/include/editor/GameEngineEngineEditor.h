#pragma once
#include <game/Game.h>
#include <editor/EditorManager.h>
#include <UI/UIListActor.h>

struct EditorEventProcessor
{
	static void FileDropCallback(GLFWwindow* window, int count, const char** paths)
	{
		if (count > 0)
		{
			std::cout << "Dropped " << paths[0] << '\n';
			EditorHandle->PreviewHierarchyTree(*EngineDataLoader::LoadHierarchyTree(*EditorHandle->GetGameHandle()->GetMainScene(), paths[0]));
		}
	}
	static void Resize(GLFWwindow* window, int width, int height)
	{
		EditorHandle->GetGameHandle()->GetGameSettings()->WindowSize = glm::uvec2(width, height);
		EditorHandle->UpdateSettings();
	}

	static EditorManager* EditorHandle;
};

class GameEngineEngineEditor : public Game, public EditorManager
{
public:
	GameEngineEngineEditor(GLFWwindow* window, const GameSettings& settings);
	virtual GameManager* GetGameHandle() override;
	virtual GameScene* GetSelectedScene() override;
	virtual std::vector<GameScene*> GetScenes() override;
	virtual void Init(GLFWwindow* window) override;
	virtual void UpdateSettings() override;
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
	RenderToolboxCollection* ViewportRenderCollection, * HUDRenderCollection;
	GameScene* EditorScene;
	GameSettings EditorSettings;

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
	element.SetTransform(Transform(glm::vec2(1.5f, 0.0f), glm::vec2(3.0f, 1.0f)));
	TextConstantSizeComponent& elementText = element.CreateComponent<TextConstantSizeComponent>(obj.GetName() + "'s Text", Transform(glm::vec2(0.0f), glm::vec2(0.4f / 3.0f, 0.4f)), "", "", std::pair<TextAlignment, TextAlignment>(TextAlignment::CENTER, TextAlignment::CENTER));
	elementText.SetMaxSize(Vec2f(0.9f));
	elementText.SetContent(obj.GetName());

	std::vector<T*> children = obj.GetChildren();
	std::cout << "liczba dzieciakow of " << obj.GetName() << ": " << children.size() << '\n';
	for (auto& child : children)
	{
		//AddActorToList(editorScene, *child, listParent.CreateChild(UIListActor(editorScene, "NextLevel")), canvas);
		UIAutomaticListActor& nestedList = listParent.CreateChild<UIAutomaticListActor>(child->GetName() + "'s nestedlist");
		AddActorToList(editorScene, *child, nestedList, canvas);
	}
}
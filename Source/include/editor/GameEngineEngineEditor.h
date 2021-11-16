#pragma once
#include <game/Game.h>
#include <editor/EditorManager.h>
#include <UI/UIListActor.h>
#include <ctpl/ctpl_stl.h>
#include <editor/EditorMessageLogger.h>
#include <unordered_map>
#include <condition_variable>

namespace GEE
{
	class PopupDescription;
	namespace Editor
	{
		struct EditorEventProcessor
		{
			static void FileDropCallback(SystemWindow* window, int count, const char** paths);
			static void Resize(SystemWindow* window, int width, int height);

			static EditorManager* EditorHandle;
		};

		class GameEngineEngineEditor : public Game, public EditorManager
		{
		public:
			GameEngineEngineEditor(SystemWindow* window, const GameSettings& settings);
			virtual GameManager* GetGameHandle() override;
			virtual Component* GetSelectedComponent() override;
			virtual GameScene* GetSelectedScene() override;
			virtual std::vector<GameScene*> GetScenes() override;
			GameSettings* GetEditorSettings() override;
			virtual AtlasMaterial* GetDefaultEditorMaterial(EditorDefaultMaterial) override;
			virtual bool CheckEEForceForwardShading() override
			{
				return EEForceForwardShading;
			}
			virtual void Init(SystemWindow* window) override;

			virtual PopupDescription CreatePopupMenu(const Vec2f& posWindowSpace, SystemWindow& relativeWindow) override;

			virtual void UpdateGameSettings() override;
			virtual void UpdateEditorSettings() override;

			void SetupEditorScene();
			void SetupMainMenu();
			virtual void Update(float deltaTime) override;
			virtual void HandleEvents() override;

			virtual void SelectComponent(Component* comp, GameScene& editorScene) override;

			virtual void SelectActor(Actor* actor, GameScene& editorScene) override;
			virtual void SelectScene(GameScene* selectedScene, GameScene& editorScene) override;
			virtual void PreviewHierarchyTree(HierarchyTemplate::HierarchyTreeT& tree) override;
			template <typename T> void AddActorToList(GameScene& editorScene, T& obj, UIAutomaticListActor& listParent, UICanvas& canvas);

			virtual void PassMouseControl(Controller* controller) override;

			virtual void Render() override;
			void LoadProject(const std::string& filepath);
			void SaveProject();

		private:
			void UpdateRecentProjects();

			RenderToolboxCollection* ViewportRenderCollection, * HUDRenderCollection;
			GameScene* EditorScene;

			std::unordered_map<GameScene*, EditorMessageLogger> Logs;

			struct Popup
			{
				Popup(unsigned int id, SystemWindow& window, GameScene& scene) : Window(window), Scene(scene), ID(id) {}
				std::reference_wrapper<SystemWindow> Window;
				std::reference_wrapper<GameScene> Scene;
				unsigned int ID;
			};
			std::vector<Popup> OpenPopups;
			std::deque<std::thread> PopupRenderThreads;
			std::mutex ShouldRenderMutex;
			std::condition_variable ShouldRenderVariable;
			bool bRenderingPopups;

			ctpl::thread_pool ThreadPool;

			GameSettings EditorSettings;
			bool bDebugRenderComponents;


			std::string ProjectName, ProjectFilepath;
			std::string ExecutableFolder;

			//	UICanvasActor* CanvasContext;
			std::vector<UICanvasActor*> Canvases;

			Component* SelectedComp;
			Actor* SelectedActor;
			GameScene* SelectedScene;

			bool EEForceForwardShading;

			// Which Controller should be set after we switch from editor to game control
			Controller* GameController;
		};

		template<typename T>
		inline void GameEngineEngineEditor::AddActorToList(GameScene& editorScene, T& obj, UIAutomaticListActor& listParent, UICanvas& canvas)
		{
			if (obj.IsBeingKilled())
				return;

			UIButtonActor& element = listParent.CreateChild<UIButtonActor>(obj.GetName() + "'s Button", [this, &obj, &editorScene]() {this->Select(&obj, editorScene); });//*this, obj, &EditorManager::Select<T>));
			element.SetTransform(Transform(Vec2f(1.5f, 0.0f), Vec2f(3.0f, 1.0f)));
			TextConstantSizeComponent& elementText = element.CreateComponent<TextConstantSizeComponent>(obj.GetName() + "'s Text", Transform(Vec2f(0.0f), Vec2f(1.0f, 1.0f)), "", "", std::pair<TextAlignment, TextAlignment>(TextAlignment::CENTER, TextAlignment::CENTER));
			elementText.SetMaxSize(Vec2f(0.9f));
			elementText.SetContent(obj.GetName());
			elementText.Unstretch();
			elementText.UpdateSize();

			std::vector<T*> children = obj.GetChildren();
			for (auto& child : children)
			{
				//AddActorToList(editorScene, *child, listParent.CreateChild(UIListActor(editorScene, "NextLevel")), canvas);
				UIAutomaticListActor& nestedList = listParent.CreateChild<UIAutomaticListActor>(child->GetName() + "'s nestedlist");
				AddActorToList(editorScene, *child, nestedList, canvas);
			}
		}
	}
}
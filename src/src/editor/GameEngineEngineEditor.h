#pragma once
#include <game/Game.h>
#include <editor/EditorActions.h>
#include <editor/EditorManager.h>
#include <UI/UIListActor.h>
#include <ctpl/ctpl_stl.h>
#include <editor/EditorMessageLogger.h>
#include <unordered_map>
#include <condition_variable>
#include <utility/Profiling.h>

namespace GEE
{
	class PopupDescription;
	namespace Editor
	{
		class EditorActions;

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

			virtual bool GameLoopIteration(float timeStep, float deltaTime) override;

			virtual GameManager* GetGameHandle() override;
			virtual std::vector<GameScene*> GetScenes() override;
			GameSettings* GetEditorSettings() override;
			virtual AtlasMaterial* GetDefaultEditorMaterial(EditorDefaultMaterial) override;
			virtual EditorActions& GetActions() override;

			virtual std::string ToRelativePath(const std::string&) override;
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

			virtual void PreviewHierarchyTree(HierarchyTemplate::HierarchyTreeT& tree) override;
			template <typename T> void AddActorToList(GameScene& editorScene, T& obj, UIAutomaticListActor& listParent, UICanvas& canvas);

			virtual void PassMouseControl(Controller* controller) override;

			virtual void Render() override;
			void LoadProject(const std::string& filepath);
			void SaveProject();

	private:
		void UpdateRecentProjects();
		void MaximizeViewport();
		EditorMessageLogger& GetEditorLogger(GameScene& scene);

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

		bool bViewportMaximzized;


			std::string ProjectName, ProjectFilepath;
			std::string ExecutableFolder;

			Profiling Profiler;

			//	UICanvasActor* CanvasContext;
			std::vector<UICanvasActor*> Canvases;

			UniquePtr<EditorActions> Actions;

			bool EEForceForwardShading;

			// Which Controller should be set after we switch from editor to game control
			Controller* GameController;
		};

		template<typename T>
		inline void GameEngineEngineEditor::AddActorToList(GameScene& editorScene, T& obj, UIAutomaticListActor& listParent, UICanvas& canvas)
		{
			if (obj.IsBeingKilled())
				return;

			UIButtonActor& element = listParent.CreateChild<UIActivableButtonActor>(obj.GetName() + "'s Button", [this, &obj, &editorScene]() {Actions->Select(&obj, editorScene); });//*this, obj, &EditorManager::Select<T>));
			element.SetTransform(Transform(Vec2f(1.5f, 0.0f), Vec2f(3.0f, 1.0f)));
			TextConstantSizeComponent& elementText = element.CreateComponent<TextConstantSizeComponent>(obj.GetName() + "'s Text", Transform(Vec2f(0.0f), Vec2f(1.0f, 1.0f)), "", "", std::pair<TextAlignment, TextAlignment>(TextAlignment::CENTER, TextAlignment::CENTER));
			elementText.SetMaxSize(Vec2f(0.9f));
			elementText.SetContent(obj.GetName());
			elementText.Unstretch();
			//elementText.UpdateSize();

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
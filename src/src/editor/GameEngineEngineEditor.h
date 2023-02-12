#pragma once
#include <game/Game.h>
#include <editor/EditorActions.h>
#include <editor/EditorManager.h>
#include <UI/UIListActor.h>
#include <editor/EditorMessageLogger.h>
#include <unordered_map>
#include <condition_variable>
#include <utility/Profiling.h>
#include <editor/GEditorSettings.h>

namespace GEE
{
	class PopupDescription;
	namespace Editor
	{
		class EditorActions;
		class GEditorRenderToolboxCollection;

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

			bool GameLoopIteration(Time timeStep, Time deltaTime) override;

			GameManager* GetGameHandle() override;
			std::vector<GameScene*> GetScenes() override;
			GameScene* GetEditorScene() { return EditorScene; }

			GEditorSettings* GetEditorSettings() override { return &EditorSettings; }
			AtlasMaterial* GetDefaultEditorMaterial(EditorDefaultMaterial) override;
			EditorActions& GetActions() override;
			EditorMessageLogger& GetEditorLogger(GameScene& scene) override;
			bool GetViewportMaximized() const override { return bViewportMaximized; }
			Profiling& GetProfilerRef() { return Profiler; }
			DefaultEditorController* GetEditorController() { return EditorController; }
			void GenerateActorList(PopupDescription, std::function<void(Actor&)>) override;

			void RequestPopupMenu(const Vec2f& posWindowSpace, SystemWindow& relativeWindow, std::function<void(PopupDescription)> popupCreation) override;

			std::string ToRelativePath(const std::string&) override;
			void Init(SystemWindow* window, const Vec2u& windowSize) override;

			PopupDescription CreatePopupMenu(const Vec2f& posWindowSpace, SystemWindow& relativeWindow) override;

			void UpdateGameSettings() override;
			void UpdateEditorSettings() override;

			virtual void SetupEditorScene();
			void SetupMainMenu();
			void Update(Time deltaTime) override;
			void HandleEvents() override;

			//virtual void PreviewHierarchyTree(Hierarchy::Tree& tree) override;

			void PassMouseControl(Controller* controller) override;

			void Render() override;
			void NewProject(const std::string& filepath);
			void LoadProject(const std::string& filepath);
			void SaveProject(std::string optionalFilepath = std::string());

	protected:
		virtual void StartProfiler();
		virtual void StopProfiler();
		std::string GetProjectFilepath() { return ProjectFilepath; }
		void SetProjectFilepath(const std::string& filepath);
	private:
		const std::deque<EditorPopup>& GetPopupsForRendering() override;
		void UpdateRecentProjects();
		void MaximizeViewport();
		void UpdateControllerCameraInfo();

		GameScene* EditorScene;

		// Which Controller should be set after we switch from editor to game control
		Controller* GameController;

		// Which Controller should be set after we switch from game to editor control
		DefaultEditorController* EditorController;

		std::unordered_map<GameScene*, EditorMessageLogger> Logs;

		std::deque<EditorPopup> OpenPopups;
		std::function<void()> LastRenderPopupRequest;


		GEditorSettings EditorSettings;
		bool bViewportMaximized;

		Vec2f TestTranslateLastPos;

		std::string ProjectName, ProjectFilepath;
		std::string ExecutableFolder;

		Profiling Profiler;

		//	UICanvasActor* CanvasContext;
		std::vector<UICanvasActor*> Canvases;

		UniquePtr<EditorActions> Actions;

	protected:
		RenderToolboxCollection* ViewportRenderCollection;
		GEditorRenderToolboxCollection* HUDRenderCollection;
		//std::vector<RenderToolboxCollection*>
		};
	}
}
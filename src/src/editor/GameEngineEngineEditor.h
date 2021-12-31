#pragma once
#include <game/Game.h>
#include <editor/EditorActions.h>
#include <editor/EditorManager.h>
#include <UI/UIListActor.h>
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

		class GameEngineEngineEditorSettings : public GameSettings
		{
		public:

		};

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
			virtual EditorMessageLogger& GetEditorLogger(GameScene& scene) override;
			virtual bool GetViewportMaximized() const override { return bViewportMaximized; }
			virtual bool GetDebugRenderComponents() const override { return bDebugRenderComponents; }
			virtual bool GetDebugRenderPhysicsMeshes() const override { return bDebugRenderPhysicsMeshes; }

			virtual void RequestPopupMenu(const Vec2f& posWindowSpace, SystemWindow& relativeWindow, std::function<void(PopupDescription)> popupCreation) override;

			virtual std::string ToRelativePath(const std::string&) override;
			virtual bool CheckEEForceForwardShading() override
			{
				return EEForceForwardShading;
			}
			virtual void Init(SystemWindow* window, const Vec2u& windowSize) override;

			virtual PopupDescription CreatePopupMenu(const Vec2f& posWindowSpace, SystemWindow& relativeWindow) override;

			virtual void UpdateGameSettings() override;
			virtual void UpdateEditorSettings() override;

			void SetupEditorScene();
			void SetupMainMenu();
			virtual void Update(float deltaTime) override;
			virtual void HandleEvents() override;

			//virtual void PreviewHierarchyTree(HierarchyTemplate::HierarchyTreeT& tree) override;
			template <typename T> void AddActorToList(GameScene& editorScene, T& obj, UIAutomaticListActor& listParent, UICanvas& canvas);

			virtual void PassMouseControl(Controller* controller) override;

			virtual void Render() override;
			void NewProject(const std::string& filepath);
			void LoadProject(const std::string& filepath);
			void SaveProject();

		virtual void SetDebugRenderComponents(bool) override;
	private:
		virtual const std::vector<EditorPopup>& GetPopupsForRendering() override;
		void UpdateRecentProjects();
		void MaximizeViewport();
		void UpdateControllerCameraInfo();

		RenderToolboxCollection* ViewportRenderCollection, * HUDRenderCollection;
		GameScene* EditorScene;

		std::unordered_map<GameScene*, EditorMessageLogger> Logs;

		std::vector<EditorPopup> OpenPopups;
		std::function<void()> LastRenderPopupRequest;


		GameSettings EditorSettings;
		bool bDebugRenderComponents, bDebugRenderPhysicsMeshes;
		bool bViewportMaximized;

		Vec2f TestTranslateLastPos;

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
	}
}
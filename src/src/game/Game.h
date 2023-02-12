#pragma once
#include <physics/PhysicsEngine.h>
#include <rendering/RenderEngine.h>
#include <audio/AudioEngine.h>
#include "GameSettings.h"
#include "GameScene.h"
#include <input/Event.h>

namespace GEE
{
	/*
		todo: comment explaining how to use the Game class
		1. Call Init with the window that you created
		2. Call LoadSceneFromFile and load at least the Main Scene, otherwise there will be no scenes and the program will just shut down
		3. Call PreGameLoop
		4. Call GameLoopIteration on every iteration
	*/

	class Game : public GameManager
	{
	public:
		Game(const ShadingAlgorithm&, const GameSettings&);
		virtual void Init(SystemWindow* window, const Vec2u& windowSize);

		bool LoadSceneFromFile(const String& path, const String& name = String());

		GameScene& CreateScene(String name, bool disallowChangingNameIfTaken = true) override;
		GameScene& CreateUIScene(String name, SystemWindow& associateWindow, bool disallowChangingNameIfTaken = true) override;
		void AddScene(UniquePtr <GameScene> scene);
		
		/**
		 * @brief Add an Interpolation to be Updated each frame. This Interpolation will be removed if its UpdateT function returns true.
		 * @param: the Interpolation object.
		*/
		void AddInterpolation(const Interpolation&) override;
		void BindAudioListenerTransformPtr(Transform*) override;

		/**
		 * @brief Checks if the passed transform is bound and unbinds it if that's the case.
		 * @param a pointer to the Transform object to be checked (and potentially unbound)
		*/
		void UnbindAudioListenerTransformPtr(Transform* transform) override;

		void PassMouseControl(Controller* controller) override; //pass nullptr to unbind the current controller.
		const Controller* GetCurrentMouseController() const override;
		SharedPtr<Font> GetDefaultFont() override;
		bool HasStarted() const override;
		/**
		 * @return The default input retriever (for the default game window)
		*/
		InputDevicesStateRetriever GetDefInputRetriever() override;
		/**
		 * @return Amount of time (in seconds) which passed since the start of the program.
		*/
		Time GetProgramRuntime() const override;

		Physics::PhysicsEngineManager* GetPhysicsHandle() override;
		RenderEngineManager* GetRenderEngineHandle() override;
		Audio::AudioEngineManager* GetAudioEngineHandle() override;

		GameSettings* GetGameSettings() override;
		GameScene* GetScene(const std::string& name) override;
		GameScene* GetMainScene() override;
		Actor* GetRootActor(GameScene* scene = nullptr) override;

		unsigned long long GetTotalTickCount() const override;
		unsigned long long GetTotalFrameCount() const override;

		GameScene* GetActiveScene() override;
		void SetActiveScene(GameScene* scene) override;

		void SetCursorIcon(DefaultCursorIcon) override;

		Hierarchy::Tree* FindHierarchyTree(const std::string& name, Hierarchy::Tree* treeToIgnore = nullptr) override;
		SharedPtr<Font> FindFont(const std::string& path) override;

		virtual void PreGameLoop();
		virtual bool GameLoopIteration(Time timeStep, Time deltaTime);
		virtual void HandleEvents();

		virtual void Update(Time deltaTime);

		virtual void Render() = 0;

	protected:
		void TerminateGame();
		void SetMainScene(GameScene*);
		EventPusher GetEventPusher(GameScene&) override;
		void DeleteScene(GameScene&) override;
		std::vector<GameSceneRenderData*> GetSceneRenderDatas();

		// If bGameTerminated is true, no more GameLoopIterations will occur.
		bool bGameTerminated;

		SystemWindow* GameWindow;

		std::vector <UniquePtr <GameScene>> Scenes;	//The first scene (Scenes[0]) is referred to as the Main Scene. If you only use 1 scene in your application, don't bother with passing arround GameScene pointers - the Main Scene will be chosen automatically.
		std::deque <UniquePtr<Interpolation>> Interpolations;
		GameScene* MainScene;
		GameScene* ActiveScene;	//which scene currently handles events
		EventHolder EventHolderObj;
		Controller* MouseController;

		UniquePtr<GameSettings> Settings;

		std::vector<SharedPtr<Font>> Fonts;
		SharedPtr<Font> DefaultFont;

		RenderEngine RenderEng;
		Physics::PhysicsEngine PhysicsEng;
		Audio::AudioEngine AudioEng;

		const ShadingAlgorithm Shading;
		bool GameStarted;
		bool DebugMode;
		Time LoopBeginTime;
		Time TimeAccumulator;

		unsigned long long TotalTickCount, TotalFrameCount;
	};

	struct WindowEventProcessor
	{
		static void CursorPosCallback(SystemWindow*, double, double);
		static void MouseButtonCallback(SystemWindow*, int button, int action, int mods);
		static void KeyPressedCallback(SystemWindow*, int key, int scancode, int action, int mods);
		static void CharEnteredCallback(SystemWindow*, unsigned int codepoint);
		static void ScrollCallback(SystemWindow*, double xoffset, double yoffset);
		static void FileDropCallback(SystemWindow*, int count, const char** paths);
		static void CursorLeaveEnterCallback(SystemWindow*, int enter);

		static GameScene* GetGameSceneFromWindow(SystemWindow&);

		static EventHolder* TargetHolder;
		static std::unordered_map<SystemWindow*, Vec2i> PrevCursorPositions;
	};

	namespace DebugCallbacks
	{
		void APIENTRY OpenGLDebug(GLenum source, GLenum type, unsigned int id, GLenum severity, GLsizei length, const char* message, const void* userParam);
	}
}
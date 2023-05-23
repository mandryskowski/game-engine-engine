#pragma once
#include <game/GameSettings.h>
#include <math/Vec.h>
#include <string>
#include <vector>


typedef unsigned int GLenum;

namespace GEE
{
	namespace Physics
	{
		class PhysicsEngineManager;
	}

	namespace Audio
	{
		class AudioEngineManager;
	}

	class Controller;
	class EventPusher;
	class Font;
	class InputDevicesStateRetriever;
	class Interpolation;
	class RenderEngineManager;
	class Transform;

	namespace GEE_FB
	{
		class Framebuffer;
		class FramebufferAttachment;
	}

	namespace Hierarchy
	{
		class NodeBase;
		template <typename CompType> class Node;
		class Tree;
	}


	// These values correspond to GLFW macros. Check official GLFW documentation.
	enum class DefaultCursorIcon
	{
		Regular = 0x00036001,
		TextInput = 0x00036002,
		Crosshair = 0x00036003,
		Hand = 0x00036004,
		HorizontalResize = 0x00036005,
		VerticalResize = 0x00036006
	};

	enum class FontStyle
	{
		Regular,
		Bold,
		Italic,
		BoldItalic
	};

	class GameManager
	{
	public:
		static GameScene* DefaultScene;
		static GameManager& Get();
	public:
		virtual void AddInterpolation(const Interpolation&) = 0;
		virtual GameScene& CreateScene(String name, bool disallowChangingNameIfTaken = true) = 0;
		virtual GameScene& CreateUIScene(String name, SystemWindow& associateWindow, bool disallowChangingNameIfTaken = true) = 0;

		virtual void BindAudioListenerTransformPtr(Transform*) = 0;
		virtual void UnbindAudioListenerTransformPtr(Transform* transform) = 0;
		virtual void PassMouseControl(Controller* controller) = 0;
		virtual const Controller* GetCurrentMouseController() const = 0;
		virtual Time GetProgramRuntime() const = 0;
		virtual InputDevicesStateRetriever GetDefInputRetriever() = 0;
		virtual SharedPtr<Font> GetDefaultFont() = 0;

		virtual bool HasStarted() const = 0;

		virtual Actor* GetRootActor(GameScene* = nullptr) = 0;
		virtual GameScene* GetScene(const String& name) = 0;
		virtual GameScene* GetMainScene() = 0;
		virtual std::vector<GameScene*> GetScenes() = 0;
		
		virtual unsigned long long GetTotalTickCount() const = 0;
		virtual unsigned long long GetTotalFrameCount() const = 0;

		virtual Physics::PhysicsEngineManager* GetPhysicsHandle() = 0;
		virtual RenderEngineManager* GetRenderEngineHandle() = 0;
		virtual Audio::AudioEngineManager* GetAudioEngineHandle() = 0;

		virtual GameSettings* GetGameSettings() = 0;

		virtual GameScene* GetActiveScene() = 0;
		virtual void SetActiveScene(GameScene* scene) = 0;

		virtual void SetCursorIcon(DefaultCursorIcon) = 0;

		virtual Hierarchy::Tree* FindHierarchyTree(const String& name, Hierarchy::Tree* treeToIgnore = nullptr) = 0;
		virtual SharedPtr<Font> FindFont(const std::string& path) = 0;

		virtual ~GameManager() = default;
	protected:
		virtual EventPusher GetEventPusher(GameScene&) = 0;
		virtual void DeleteScene(GameScene&) = 0;
	private:
		static GameManager* GamePtr;
		friend class GameScene;
		friend class Game;
	};

	struct PrimitiveDebugger
	{
		static bool bDebugMeshTrees;
		static bool bDebugProbeLoading;
		static bool bDebugCubemapFromTex;
		static bool bDebugFramebuffers;
		static bool bDebugHierarchy;
	};

	class HTreeObjectLoc
	{
		const Hierarchy::Tree* TreePtr;
	protected:
		HTreeObjectLoc() : TreePtr(nullptr) {}
	public:
		HTreeObjectLoc(const Hierarchy::Tree& tree) : TreePtr(&tree) {}
		[[nodiscard]] bool IsValidTreeElement() const;
		[[nodiscard]] String GetTreeName() const;	//get path
	};
}
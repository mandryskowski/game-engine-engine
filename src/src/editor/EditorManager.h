#pragma once
#include <math/Vec.h>
#include <utility/Utility.h>
#include <deque>

namespace GEE
{
	class Component;
	class GameScene;
	class Actor;
	class GameManager;
	struct GameSettings;
	class AtlasMaterial;
	
	namespace Hierarchy
	{
		class Tree;
	}

	enum class EditorDefaultMaterial
	{
		Close,
		Refresh,
		Add,
		Remove,
		More,

		X_Axis,
		Y_Axis,
		Z_Axis,
		W_Axis
	};

	enum ButtonStateFlags
	{
		Hover = 1,
		BeingClicked = 2,
		Active = 4
	};

	enum class ButtonMaterialType
	{
		Idle,
		Hover,
		BeingClicked,
		Active
	};

	class UICanvasActor;

	class PopupDescription;
	namespace Editor
	{
		class EditorActions;
		class EditorMessageLogger;
		struct EditorPopup;

		class EditorManager
		{
		public:
			virtual PopupDescription CreatePopupMenu(const Vec2f& posWindowSpace, SystemWindow& relativeWindow) = 0;
			virtual EditorActions& GetActions() = 0;

			virtual EditorMessageLogger& GetEditorLogger(GameScene& scene) = 0;
			virtual bool GetViewportMaximized() const = 0;
			virtual bool GetDebugRenderComponents() const = 0;
			virtual bool GetDebugRenderPhysicsMeshes() const = 0;

			virtual void RequestPopupMenu(const Vec2f&, SystemWindow&, std::function<void(PopupDescription)>) = 0;

			virtual void UpdateGameSettings() = 0;
			virtual void UpdateEditorSettings() = 0;

			virtual GameManager* GetGameHandle() = 0;
			virtual GameSettings* GetEditorSettings() = 0;

			virtual AtlasMaterial* GetDefaultEditorMaterial(EditorDefaultMaterial) = 0;

			virtual std::string ToRelativePath(const std::string&) = 0;

			//virtual void PreviewHierarchyTree(Hierarchy::Tree& tree) = 0;

			virtual ~EditorManager() = default;

			virtual void SetDebugRenderComponents(bool) = 0;

		private:
			virtual const std::deque<EditorPopup>& GetPopupsForRendering() = 0;
			friend class EditorRenderer;
		};
	}
}
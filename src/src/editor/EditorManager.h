#pragma once
#include <math/Vec.h>
#include <utility/Utility.h>

namespace GEE
{
	class Component;
	class GameScene;
	class Actor;
	class GameManager;
	struct GameSettings;
	class AtlasMaterial;
	
	namespace HierarchyTemplate
	{
		class HierarchyTreeT;
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

	enum class EditorIconState
	{
		IDLE,
		HOVER,
		BEING_CLICKED_INSIDE,
		BEING_CLICKED_OUTSIDE,
		ACTIVATED
	};

	class UICanvasActor;

	class PopupDescription;
	namespace Editor
	{
		class EditorActions;

		class EditorManager
		{
		public:
			virtual PopupDescription CreatePopupMenu(const Vec2f& posWindowSpace, SystemWindow& relativeWindow) = 0;
			virtual EditorActions& GetActions() = 0;

			virtual void UpdateGameSettings() = 0;
			virtual void UpdateEditorSettings() = 0;

			virtual GameManager* GetGameHandle() = 0;
			virtual GameSettings* GetEditorSettings() = 0;

			virtual AtlasMaterial* GetDefaultEditorMaterial(EditorDefaultMaterial) = 0;

			virtual std::string ToRelativePath(const std::string&) = 0;

			virtual void PreviewHierarchyTree(HierarchyTemplate::HierarchyTreeT& tree) = 0;

			virtual ~EditorManager() = default;
		};
	}
}
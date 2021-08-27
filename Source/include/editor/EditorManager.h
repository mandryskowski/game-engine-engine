#pragma once

namespace GEE
{
	class Component;
	class GameScene;
	class Actor;
	class GameManager;
	class GameSettings;
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

	class EditorManager
	{
	public:
		virtual void SelectComponent(Component* comp, GameScene& editorScene) = 0;
		virtual void SelectActor(Actor* actor, GameScene& editorScene) = 0;
		virtual void SelectScene(GameScene* scene, GameScene& editorScene) = 0;
		template <typename T> void Select(T*, GameScene& editorScene);

		virtual void UpdateGameSettings() = 0;
		virtual void UpdateEditorSettings() = 0;

		virtual GameManager* GetGameHandle() = 0;
		virtual Component* GetSelectedComponent() = 0;
		virtual GameScene* GetSelectedScene() = 0;
		virtual GameSettings* GetEditorSettings() = 0;

		virtual AtlasMaterial* GetDefaultEditorMaterial(EditorDefaultMaterial) = 0;

		virtual void PreviewHierarchyTree(HierarchyTemplate::HierarchyTreeT& tree) = 0;

		virtual ~EditorManager() = default;
	};
}
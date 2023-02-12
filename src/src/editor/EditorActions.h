#pragma once
#include <game/GameManager.h>
#include <math/Transform.h>
#include <editor/EditorManager.h>
#include <UI/UIListActor.h>
#include <scene/hierarchy/HierarchyNode.h>

namespace GEE
{
	class UIButtonActor;
	class UIInputBoxActor;
	class UIActivableButtonActor;
	class UIAutomaticListActor;

	namespace Editor
	{
		struct EditorPopup;
	}

	class IconData
	{
	public:
		/**
		 * @brief Construct an invalid IconData object.
		*/
		IconData() : IconMaterial(nullptr), IconAtlasID(-1.0f) {}

		IconData(RenderEngineManager& renderHandle, const std::string& texFilepath, Vec2i iconAtlasSize = Vec2i(0), float iconAtlasID = -1.0f);
		bool IsValid() const;
		SharedPtr<MaterialInstance> GetMatInstance() const;
	private:
		SharedPtr<Material> IconMaterial;
		float IconAtlasID;
	};

	class PopupDescription
	{
	public:
		PopupDescription(Editor::EditorPopup& popup, UIAutomaticListActor*, UICanvasActor* canvas);
		UIButtonActor& AddOption(const std::string&, std::function<void()>, const IconData& = IconData());
		UIInputBoxActor& AddInputBox(std::function<void(std::string)>, const std::string& defaultContent = "0");
		PopupDescription AddSubmenu(const std::string&, std::function<void(PopupDescription)> prepareSubmenu, const IconData& = IconData());
		/**
		 * @brief Should be called after adding all the options and submenus.
		*/
		void RefreshPopup();
		 
	private:
		Editor::EditorPopup& Popup;
		UIAutomaticListActor* DescriptionRoot;
		UICanvasActor* PopupCanvas;
		Vec2i PopupOptionSizePx;
	};

	namespace Editor
	{
		struct EditorPopup
		{
			EditorPopup(unsigned int id, SystemWindow& window, GameScene& scene, RenderToolboxCollection& tbCol, SharedPtr<GameSettings::VideoSettings> settings) : Window(window), Scene(scene), ID(id), TbCol(tbCol), Settings(std::move(settings)) { std::cout << "tb collll adress: " << &TbCol.get() << '\n'; }
			EditorPopup(EditorPopup&& rhs) :
				Window(rhs.Window),
				Scene(rhs.Scene),
				TbCol(rhs.TbCol),
				Settings(rhs.Settings),
				ID(rhs.ID)
			{}
			EditorPopup& operator=(EditorPopup&& rhs)
			{
				Window = rhs.Window;
				Scene = rhs.Scene;
				TbCol = rhs.TbCol;
				Settings = rhs.Settings;
				ID = rhs.ID;

				return *this;
			}
			std::reference_wrapper<SystemWindow> Window;
			std::reference_wrapper<GameScene> Scene;
			std::reference_wrapper<RenderToolboxCollection> TbCol;
			SharedPtr<GameSettings::VideoSettings> Settings;
			unsigned int ID;
		};

		class EditorActions
		{
		public:
			EditorActions(EditorManager&);

			Component* GetSelectedComponent();
			Actor* GetSelectedActor();
			GameScene* GetSelectedScene();

			void SelectComponent(Component* comp, GameScene& editorScene);
			void SelectActor(Actor* actor, GameScene& editorScene);
			void SelectScene(GameScene* selectedScene, GameScene& editorScene);
			template <typename T> void Select(T*, GameScene& editorScene);

			void PreviewHierarchyTree(Hierarchy::Tree& tree);
			void PreviewHierarchyNode(Hierarchy::NodeBase& node, GameScene& editorScene);

//		private:
			Component* GetContextComp();
			Actor* GetContextActor();
			void CreateComponentWindow(Actor&);
			friend class GameEngineEngineEditor;
		private:

			EditorManager& EditorHandle;

			Component* SelectedComp;
			Actor* SelectedActor;
			GameScene* SelectedScene;
		};

		struct FunctorHierarchyNodeCreator
		{
			FunctorHierarchyNodeCreator(Hierarchy::NodeBase& parent) : ParentNode(parent) {}
			template <typename T>
			Hierarchy::Node<T>& Create(const std::string& name)
			{
				return ParentNode.CreateChild<T>(name);
			}
		private:
			Hierarchy::NodeBase& ParentNode;
		};

		class ContextMenusFactory
		{
		public:
			ContextMenusFactory(RenderEngineManager&);
			void AddComponent(PopupDescription, Component& contextComp, std::function<void(Component&)> onCreation);
			template <typename TFunctor>
			void AddComponent(PopupDescription, TFunctor&&, std::function<void()> onCreation);
		private:
			RenderEngineManager& RenderHandle;
		};
	}
}
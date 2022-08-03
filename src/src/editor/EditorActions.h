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

			template <typename T> void AddActorToList(GameScene& editorScene, T& obj, UIAutomaticListActor& listParent, UICanvas& canvas, std::function<void(PopupDescription, T&)> createPopupFunc = nullptr, unsigned int elementDepth = 0);

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

		template<typename T>
		inline void EditorActions::AddActorToList(GameScene& editorScene, T& obj, UIAutomaticListActor& listParent, UICanvas& canvas, std::function<void(PopupDescription, T&)> createPopupFunc, unsigned int elementDepth)
		{
			if (obj.IsBeingKilled())	
				return;

			auto& element = listParent.CreateChild<UIActivableButtonActor>(obj.GetName(), [this, &obj, &editorScene]() {this->Select(&obj, editorScene); });//*this, obj, &EditorManager::Select<CompType>));
			SharedPtr<Material> colorMat = MakeShared<Material>("ColorMat", Vec4f(hsvToRgb(Vec3f(288.0f, 0.82f, 0.49f + static_cast<float>(elementDepth) * 0.03f)), 1.0f));
			element.SetMatIdle(MaterialInstance(colorMat));
			element.SetActivateOnClicking(false);
			element.SetDeactivateOnClickingAnywhere(false);
			element.SetDeactivateOnClickingAgain(false);
			element.SetTransform(Transform(Vec2f(0.0f, 0.0f), Vec2f(3.0f, 1.0f)));
			TextComponent& elementText = element.CreateComponent<TextComponent>("ButtonText", Transform(Vec2f(-0.8f + static_cast<float>(elementDepth) * 0.05f, 0.0f), Vec2f(1.0f, 1.0f)), "", "", Alignment2D::Center());
			elementText.SetFontStyle(FontStyle::Italic);
			elementText.SetMaxSize(Vec2f(0.7f));
			elementText.SetContent(obj.GetName());
			elementText.SetHorizontalAlignment(Alignment::Left);
			//elementText.Unstretch();
			if (createPopupFunc)
				element.SetPopupCreationFunc([&obj, createPopupFunc](PopupDescription desc) { createPopupFunc(desc, obj); });
			//elementText.UpdateSize();

			std::vector<T*> children = obj.GetChildren();
			for (auto& child : children)
			{
				//AddActorToList(editorScene, *child, listParent.CreateChild(UIListActor(editorScene, "NextLevel")), canvas);
				UIAutomaticListActor& nestedList = listParent.CreateChild<UIAutomaticListActor>(child->GetName() + "'s nestedlist");
				AddActorToList(editorScene, *child, nestedList, canvas, createPopupFunc, elementDepth + 1);
			}
		}
	}
}
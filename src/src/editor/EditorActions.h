#pragma once
#include <game/GameManager.h>
#include <math/Transform.h>
#include <editor/EditorManager.h>
#include <UI/UIListActor.h>
#include <scene/hierarchy/HierarchyNode.h>
#include <scene/UIInputBoxActor.h>
#include <UI/UICanvasActor.h>
#include <scene/TextComponent.h>
#include <utility/RenamableDetection.h>
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
			enum class CanvasList
			{
				Left,
				Bottom,
				Right
			};

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

			Pair<UICanvasActor&, CanvasList> AddToCanvasList(const String& name, bool copyPreviousView = true);
			void PolishCanvasListElement(UICanvasActor& canvas, CanvasList canvasList, const String& titleStr, unsigned int fieldLength, bool autoClamp)
			{
				PolishCanvasListElement(canvas, canvasList,
					nullptr,
					[titleStr]() { return titleStr; }, fieldLength,
					autoClamp);
			}
			template <typename Renamable, typename T = typename std::enable_if_t<is_renamable<Renamable>>>
			void PolishCanvasListElement(UICanvasActor& canvas, CanvasList canvasList, T& titleRenamable, unsigned int fieldLength, bool autoClamp)
			{
				PolishCanvasListElement(canvas, canvasList,
				                        [&titleRenamable](const String& str) { titleRenamable.SetName(str); },
				                        [&titleRenamable]() { return titleRenamable.GetName(); }, fieldLength,
				                        autoClamp);
			}
			template <typename SetNameFunc, typename GetNameFunc>
			void PolishCanvasListElement(UICanvasActor& canvas, CanvasList canvasList, SetNameFunc setFunc, GetNameFunc getFunc, unsigned int fieldLength, bool autoClamp)
			{
				std::cout << "$*$ First canvas view " << canvas.GetViewT().GetPos2D() << '\n';
				auto canvasListActor = GetCanvasListActor(canvasList);
				GEE_CORE_ASSERT(canvasListActor);

				bool viewWasSet = !canvas.GetViewT().IsEmpty();

				canvas.GetTransform()->SetScale(Vec2f(1.0f, fieldLength));
				canvas.SetCanvasView(Transform(canvas.GetViewT().GetPos(), Vec2f(canvas.GetViewT().GetScale2D().x, fieldLength)), false);
				auto& titleActor = addTitleAndPositionCanvas(canvas, setFunc, getFunc);
				std::cout << "$*$ Mid canvas view " << canvas.GetViewT().GetPos2D() << '\n';

				canvas.RefreshFieldsList(false);
				canvasListActor->AddElement(UIListElement(&canvas,
					[&canvas, &titleActor]() -> Vec3f { return Vec3f(0.0f, -canvas.GetBoundingBox(false).Size.y * (2.0f + titleActor.GetTransform()->GetScale2D().y * 2.0f), 0.0f); },
					[&canvas, &titleActor]() -> Vec3f { return Vec3f(0.0f, -canvas.GetBoundingBox(false).Size.y * (1.0f + titleActor.GetTransform()->GetScale2D().y * 2.0f), 0.0f); }));
				canvasListActor->Refresh();

				if (autoClamp)
					canvas.AutoClampView();

				canvas.ClampViewToElements();

				std::cout << "$*$ Final canvas view " << canvas.GetViewT().GetPos2D() << '\n';
			}
			void RefreshCanvasLists()
			{
				InitCanvasListPointers();

				LeftCanvasList->Refresh();
				BottomCanvasList->Refresh();
				RightCanvasList->Refresh();
			}

			friend class GameEngineEngineEditor;
		private:
			void InitCanvasListPointers();
			UIListActor* GetCanvasListActor(CanvasList canvasList)
			{
				switch (canvasList)
				{
				case CanvasList::Left: return LeftCanvasList;
				case CanvasList::Bottom: return BottomCanvasList;
				case CanvasList::Right: return RightCanvasList;
				}
			}
			template <typename SetNameFunc, typename GetNameFunc>
			UIInputBoxActor& addTitleAndPositionCanvas(UICanvasActor& canvas, SetNameFunc setFunc, GetNameFunc getFunc, float titleScaleWorldY = 0.05f)
			{
				const auto canvasScaleY = canvas.GetTransform()->GetWorldTransform().GetScale2D().y;
				auto titleButtonScaleY = titleScaleWorldY / canvasScaleY;

				auto& titleButton = canvas.CreateChild<UIInputBoxActor>(canvas.GetName() + "_Title", setFunc, getFunc, Transform(Vec2f(0.0f, 1.0f + titleButtonScaleY), Vec2f(1.0f, titleButtonScaleY)));
				titleButton.DetachFromCanvas();
				titleButton.GetContentTextComp()->GetTransform().SetScale(Vec2f(0.3f, 0.3f));
				titleButton.GetContentTextComp()->SetFontStyle(FontStyle::Bold);
				titleButton.GetContentTextComp()->Unstretch();
				titleButton.SetMatIdle(std::make_shared<Material>("SceneTitleMat", Vec4f(hsvToRgb(Vec3f(288.0f, 0.82f, 0.3f)), 1.0f)));
				titleButton.SetPopupCreationFunc([](PopupDescription desc) { desc.AddOption("Rename", nullptr); });

				return titleButton;
			}

			template <typename Renamable>
			UIInputBoxActor& addTitleAndPositionCanvas(UICanvasActor& canvas, Renamable& renamable, float titleScaleWorldY = 0.05f)
			{
				return addTitleAndPositionCanvas(canvas, [&renamable](const std::string& str) { renamable.SetName(str); }, [&renamable]() { return renamable.GetName(); }, titleScaleWorldY);
			}


			EditorManager& EditorHandle;

			Component* SelectedComp;
			Actor* SelectedActor;
			GameScene* SelectedScene;

			UIListActor *LeftCanvasList, *BottomCanvasList, *RightCanvasList;
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
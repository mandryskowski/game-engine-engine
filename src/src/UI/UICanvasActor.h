#pragma once
#include <scene/Actor.h>
#include <game/GameScene.h>
#include <scene/RenderableComponent.h>
#include <UI/UICanvas.h>
#include <UI/UIListActor.h>

namespace GEE
{
	namespace Editor
	{
		class EditorManager;
	}

	class ModelComponent;
	class UIListActor;
	class UICanvasField;
	class UIButtonActor;
	class UICanvasFieldCategory;
	class UIActorDefault;
	class UIElementTemplates;

	class PopupDescription;

	class UICanvasActor : public UIActorDefault, public UICanvas
	{
	public:
		UICanvasActor(GameScene&, Actor* parentActor, UICanvasActor* canvasParent, const std::string& name, const Transform & = Transform());
		UICanvasActor(GameScene&, Actor* parentActor, const std::string& name, const Transform & = Transform());
		UICanvasActor(UICanvasActor&&);

		void OnStart() override;

		Vec3f GetFieldsListOffset() const
		{
			return FieldsList->GetListOffset();
		}
		std::function<void(PopupDescription)> GetPopupCreationFunc()
		{
			return PopupCreationFunc;
		}

		UIActorDefault* GetScaleActor();
		Mat4f GetViewMatrix() const override;
		NDCViewport GetViewport() const override;
		const Transform* GetCanvasT() const override;
		Boxf<Vec2f> GetBoundingBox(bool world = true) const override;

		void Update(Time dt) override
		{
			Actor::Update(dt);
			if (CanvasParent && CanvasParent->IsBeingKilled())
				CanvasParent = nullptr;
		}

		void SetPopupCreationFunc(std::function<void(PopupDescription)>);
		void SetCanvasView(const Transform&, bool clampView = false) override;
		/**
		 * @return The ModelComponent that is the canvas background. Can be nullptr.
		*/
		ModelComponent* GetCanvasBackground();

		Vec2f FromCanvasSpace(const Vec2f& canvasSpacePos) const override;
		Vec2f ScaleFromCanvasSpace(const Vec2f& canvasSpaceScale) const override;
		Mat4f FromCanvasSpace(const Mat4f& canvasSpaceMat) const override;
		Transform FromCanvasSpace(const Transform& canvasSpaceTransform) const override;
		Vec2f ToCanvasSpace(const Vec2f& worldPos) const override;
		Mat4f ToCanvasSpace(const Mat4f& worldMat) const override;
		Transform ToCanvasSpace(const Transform& worldTransform) const override;

		void RefreshFieldsList(bool clampview = true);

		void KillScrollBars();
		void KillResizeBars();
		void ClampViewToElements() override;

		/**
		 * @brief Create a quad model to distinguish between the canvas and other UI elements.
		 * @param color: the colour of the background. Leave it at Vec3f(-1.0f) to use the default colour.
		*/
		void CreateCanvasBackgroundModel(const Vec3f& color = Vec3f(-1.0f));
		template <typename ChildClass, typename... Args> ChildClass& CreateChildCanvas(Args&&...);

		Actor& AddChild(UniquePtr<Actor>) override;
		UICanvasFieldCategory& AddCategory(const std::string& name) override;
		UICanvasField& AddField(const std::string& name, std::function<Vec3f()> getElementOffset = nullptr) override;

		void HandleEvent(const Event& ev) override;
		void HandleEventAll(const Event& ev) override;

		SceneMatrixInfo BindForRender(const SceneMatrixInfo&) override;
		void UnbindForRender() override;

		~UICanvasActor();
	protected:
		void GetExternalButtons(std::vector<UIButtonActor*>&) const override;
		std::string GetFullCanvasName() const;
		void PushCanvasViewChangeEvent() override;
	private:

		void CreateScrollBars();
		template <VecAxis barAxis> void UpdateScrollBarT();

		UIActorDefault* ScaleActor;
		UIListActor* FieldsList;
		Vec3f FieldSize;
		UIScrollBarActor* ScrollBarX, *ScrollBarY, *BothScrollBarsButton;
		UIScrollBarActor* ResizeBarX, *ResizeBarY;
		ModelComponent* CanvasBackground;

		UICanvasActor* CanvasParent;
		std::function<void(PopupDescription)> PopupCreationFunc;
	};

	struct BuilderStyle
	{
		Vec2f NormalButtonSize, LongButtonSize, VecButtonSize;

		BuilderStyle() :
			NormalButtonSize(1.0f), LongButtonSize(3.0f, 1.0f), VecButtonSize(1.0f, 0.6f) {}
		
		// Not cached
		Transform GetNormalButtonT() const { return Transform(Vec2f(NormalButtonSize.x - 1.0f, 0.0f), NormalButtonSize); }
		Transform GetLongButtonT() const { return Transform(Vec2f(LongButtonSize.x - 1.0f, 0.0f), LongButtonSize); }
		Transform GetVecButtonT() const { return Transform(Vec2f(VecButtonSize.x - 1.0f, 0.0f), VecButtonSize); }
	};

	class EditorDescriptionBuilder
	{
	public:
		EditorDescriptionBuilder(Editor::EditorManager& editorHandle, UICanvasFieldCategory&);
		EditorDescriptionBuilder(Editor::EditorManager&, UIActorDefault&);
		EditorDescriptionBuilder(Editor::EditorManager&, Actor&, UICanvas&);
		GameScene& GetEditorScene();
		UICanvas& GetCanvas();
		Actor& GetDescriptionParent();
		GameManager& GetGameHandle();
		Editor::EditorManager& GetEditorHandle();
		BuilderStyle GetStyle() const { return Style; }

		UICanvasField& AddField(const std::string& name, std::function<Vec3f()> getFieldOffsetFunc = nullptr);	//equivalent to GetCanvasActor().AddField(...). I put it here for easier access.
		UICanvasFieldCategory& AddCategory(const std::string& name);

		template <typename ChildClass, typename... Args> ChildClass& CreateActor(Args&&...);

		void SetDeleteFunction(std::function<void()> deleteFunction) { DeleteFunction = deleteFunction; }
		void CallDeleteFunction() { if (DeleteFunction) DeleteFunction(); }

		void SelectComponent(Component*);
		void SelectActor(Actor*);
		void RefreshComponent();
		void RefreshActor();
		void RefreshScene();

		void DeleteDescription();
	private:
		Editor::EditorManager& EditorHandle;
		GameScene& EditorScene;
		Actor& DescriptionParent;
		UICanvas& CanvasRef;

		UICanvasFieldCategory* OptionalCategory;

		std::function<void()> DeleteFunction;

		BuilderStyle Style;
	};
		

	class ComponentDescriptionBuilder : public EditorDescriptionBuilder
	{
	public:
		ComponentDescriptionBuilder(Editor::EditorManager& editorHandle, UICanvasFieldCategory&);
		ComponentDescriptionBuilder(Editor::EditorManager&, UIActorDefault&);
		ComponentDescriptionBuilder(Editor::EditorManager&, Actor&, UICanvas&);
		
		void Refresh();

		bool IsNodeBeingBuilt() const;
		void SetNodeBeingBuilt(Hierarchy::NodeBase*);
		Hierarchy::NodeBase* GetNodeBeingBuilt();
	private:
		Hierarchy::NodeBase* OptionalBuiltNode;
	};

	template <typename ChildClass, typename... Args>
	inline ChildClass& UICanvasActor::CreateChildCanvas(Args&&... args)
	{
		return Scene.CreateActorAtRoot<ChildClass>(this, std::forward<Args>(args)...);
	}

	template <typename ChildClass, typename... Args>
	inline ChildClass& EditorDescriptionBuilder::CreateActor(Args&&... args)
	{
		return DescriptionParent.CreateChild<ChildClass>(std::forward<Args>(args)...);
	}



	UICanvasField& AddFieldToCanvas(const std::string& name, UICanvasElement& element, std::function<Vec3f()> getFieldOffsetFunc = nullptr);
}
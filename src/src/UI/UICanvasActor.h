#pragma once
#include <scene/Actor.h>
#include <game/GameScene.h>
#include <scene/RenderableComponent.h>
#include <UI/UICanvas.h>
#include <UI/UIListActor.h>

namespace GEE
{
	class UIListActor;
	class UICanvasField;
	class UIButtonActor;
	class UICanvasFieldCategory;
	class UIActorDefault;
	class UIElementTemplates;

	class PopupDescription;

	class UICanvasActor : public Actor, public UICanvas
	{
	public:
		UICanvasActor(GameScene&, Actor* parentActor, UICanvasActor* canvasParent, const std::string& name, const Transform & = Transform());
		UICanvasActor(GameScene&, Actor* parentActor, const std::string& name, const Transform & = Transform());
		UICanvasActor(UICanvasActor&&);

		virtual void OnStart() override;

		UIActorDefault* GetScaleActor();
		virtual Mat4f GetViewMatrix() const override;
		virtual NDCViewport GetViewport() const override;
		virtual const Transform* GetCanvasT() const override;

		virtual void Update(float deltaTime) override
		{
			Actor::Update(deltaTime);
			if (CanvasParent && CanvasParent->IsBeingKilled())
				CanvasParent = nullptr;
		}

		void SetPopupCreationFunc(std::function<void(PopupDescription)>);
		virtual void SetCanvasView(const Transform&, bool clampView = true) override;
		/**
		 * @return The ModelComponent that is the canvas background. Can be nullptr.
		*/
		ModelComponent* GetCanvasBackground();

		virtual Vec2f FromCanvasSpace(const Vec2f& canvasSpacePos) const override;
		virtual Vec2f ScaleFromCanvasSpace(const Vec2f& canvasSpaceScale) const override;
		virtual Mat4f FromCanvasSpace(const Mat4f& canvasSpaceMat) const override;
		virtual Transform FromCanvasSpace(const Transform& canvasSpaceTransform) const override;
		virtual Vec2f ToCanvasSpace(const Vec2f& worldPos) const override;
		virtual Mat4f ToCanvasSpace(const Mat4f& worldMat) const override;
		virtual Transform ToCanvasSpace(const Transform& worldTransform) const override;

		void RefreshFieldsList();

		void KillScrollBars();
		void KillResizeBars();
		virtual void ClampViewToElements() override;

		/**
		 * @brief Create a quad model to distinguish between the canvas and other UI elements.
		 * @param color: the colour of the background. Leave it at Vec3f(-1.0f) to use the default colour.
		*/
		void CreateCanvasBackgroundModel(const Vec3f& color = Vec3f(-1.0f));
		template <typename ChildClass, typename... Args> ChildClass& CreateChildCanvas(Args&&...);

		virtual Actor& AddChild(UniquePtr<Actor>) override;
		virtual UICanvasFieldCategory& AddCategory(const std::string& name) override;
		virtual UICanvasField& AddField(const std::string& name, std::function<Vec3f()> getElementOffset = nullptr) override;

		virtual void HandleEvent(const Event& ev) override;
		virtual void HandleEventAll(const Event& ev) override;

		virtual SceneMatrixInfo BindForRender(const SceneMatrixInfo&) override;
		virtual void UnbindForRender() override;

		~UICanvasActor();
	protected:
		virtual void GetExternalButtons(std::vector<UIButtonActor*>&) const override;
		std::string GetFullCanvasName() const;
	private:

		void CreateScrollBars();
		template <VecAxis barAxis> void UpdateScrollBarT();
	public:
		UIActorDefault* ScaleActor;
		UIListActor* FieldsList;
		Vec3f FieldSize;
		UIScrollBarActor* ScrollBarX, *ScrollBarY, *BothScrollBarsButton;
		UIScrollBarActor* ResizeBarX, *ResizeBarY;
		ModelComponent* CanvasBackground;

		UICanvasActor* CanvasParent;
		std::function<void(PopupDescription)> PopupCreationFunc;
	};

	class UICanvasFieldCategory : public UIAutomaticListActor
	{
	public:
		UICanvasFieldCategory(GameScene& scene, Actor* parentActor, const std::string& name);

		virtual void OnStart() override;
		UIButtonActor* GetExpandButton();

		UIElementTemplates GetTemplates();

		UICanvasField& AddField(const std::string& name, std::function<Vec3f()> getElementOffset = nullptr);
		UICanvasFieldCategory& AddCategory(const std::string& name);

		virtual Vec3f GetListOffset() override;
		void SetOnExpansionFunc(std::function<void()> onExpansionFunc);

		virtual void HandleEventAll(const Event& ev) override;

		virtual void Refresh() override;

	private:
		std::function<void()> OnExpansionFunc;
		UIButtonActor* ExpandButton;
		ModelComponent* CategoryBackgroundQuad;
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
		GameManager& GetGameHandle() { return *EditorHandle.GetGameHandle(); }
		Editor::EditorManager& GetEditorHandle();

		UICanvasField& AddField(const std::string& name, std::function<Vec3f()> getFieldOffsetFunc = nullptr);	//equivalent to GetCanvasActor().AddField(...). I put it here for easier access.
		UICanvasFieldCategory& AddCategory(const std::string& name);

		template <typename ChildClass, typename... Args> ChildClass& CreateActor(Args&&...);

		void SetDeleteFunction(std::function<void()> deleteFunction) { DeleteFunction = deleteFunction; }
		void CallDeleteFunction() { if (DeleteFunction) DeleteFunction(); }

		void SelectComponent(Component*);
		void SelectActor(Actor*);
		void RefreshComponent();
		void RefreshScene();

		void DeleteDescription();
	private:
		Editor::EditorManager& EditorHandle;
		GameScene& EditorScene;
		Actor& DescriptionParent;
		UICanvas& CanvasRef;

		UICanvasFieldCategory* OptionalCategory;

		std::function<void()> DeleteFunction;
	};

	class ComponentDescriptionBuilder : public EditorDescriptionBuilder
	{
	public:
		ComponentDescriptionBuilder(Editor::EditorManager& editorHandle, UICanvasFieldCategory&);
		ComponentDescriptionBuilder(Editor::EditorManager&, UIActorDefault&);
		ComponentDescriptionBuilder(Editor::EditorManager&, Actor&, UICanvas&);
		
		void Refresh();

		bool IsNodeBeingBuilt() const;
		void SetNodeBeingBuilt(HierarchyTemplate::HierarchyNodeBase*);
		HierarchyTemplate::HierarchyNodeBase* GetNodeBeingBuilt();
	private:
		HierarchyTemplate::HierarchyNodeBase* OptionalBuiltNode;
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
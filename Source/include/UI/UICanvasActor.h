#pragma once
#include <scene/Actor.h>
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

	class UICanvasActor : public Actor, public UICanvas
	{
	public:
		UICanvasActor(GameScene&, Actor* parentActor, UICanvasActor* canvasParent, const std::string& name, const Transform & = Transform());
		UICanvasActor(GameScene&, Actor* parentActor, const std::string& name, const Transform & = Transform());
		UICanvasActor(UICanvasActor&&);

		virtual void OnStart() override;

		UIActorDefault* GetScaleActor();
		virtual glm::mat4 GetViewMatrix() const override;
		virtual NDCViewport GetViewport() const override;
		virtual const Transform* GetCanvasT() const override;

		virtual void SetCanvasView(const Transform&) override;

		virtual Transform ToCanvasSpace(const Transform& worldTransform) const override;

		void RefreshFieldsList();

		void HideScrollBars();
		virtual void ClampViewToElements() override;

		template <typename ChildClass, typename... Args> ChildClass& CreateChildCanvas(Args&&...);

		virtual Actor& AddChild(std::unique_ptr<Actor>) override;
		virtual UICanvasFieldCategory& AddCategory(const std::string& name) override;
		virtual UICanvasField& AddField(const std::string& name, std::function<glm::vec3()> getElementOffset = nullptr) override;

		virtual void HandleEvent(const Event& ev) override;
		virtual void HandleEventAll(const Event& ev) override;

		virtual RenderInfo BindForRender(const RenderInfo&, const glm::uvec2& res) override;
		virtual void UnbindForRender(const glm::uvec2& res) override;

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
		glm::vec3 FieldSize;
		UIScrollBarActor* ScrollBarX, * ScrollBarY, * BothScrollBarsButton;
		UIScrollBarActor* ResizeBarX, * ResizeBarY;

		UICanvasActor* CanvasParent;
	};

	class UICanvasFieldCategory : public UIAutomaticListActor
	{
	public:
		UICanvasFieldCategory(GameScene& scene, Actor* parentActor, const std::string& name);

		virtual void OnStart() override;
		UIButtonActor* GetExpandButton();

		UIElementTemplates GetTemplates();

		UICanvasField& AddField(const std::string& name, std::function<glm::vec3()> getElementOffset = nullptr);

		virtual glm::vec3 GetListOffset() override;
		void SetOnExpansionFunc(std::function<void()> onExpansionFunc);

		virtual void HandleEventAll(const Event& ev) override;
	private:
		std::function<void()> OnExpansionFunc;
		UIButtonActor* ExpandButton;
	};

	class EditorDescriptionBuilder
	{
	public:
		EditorDescriptionBuilder(EditorManager&, UIActorDefault&);
		EditorDescriptionBuilder(EditorManager&, Actor&, UICanvas&);
		GameScene& GetEditorScene();
		UICanvas& GetCanvas();
		Actor& GetDescriptionParent();
		EditorManager& GetEditorHandle();

		UICanvasField& AddField(const std::string& name, std::function<glm::vec3()> getFieldOffsetFunc = nullptr);	//equivalent to GetCanvasActor().AddField(...). I put it here for easier access.
		UICanvasFieldCategory& AddCategory(const std::string& name);

		template <typename ChildClass, typename... Args> ChildClass& CreateActor(Args&&...);

		void SelectComponent(Component*);
		void SelectActor(Actor*);
		void RefreshScene();

		void DeleteDescription();
	private:
		EditorManager& EditorHandle;
		GameScene& EditorScene;
		Actor& DescriptionParent;
		UICanvas& CanvasRef;
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



	UICanvasField& AddFieldToCanvas(const std::string& name, UICanvasElement& element, std::function<glm::vec3()> getFieldOffsetFunc = nullptr);
}
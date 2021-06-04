#pragma once
#include <scene/Actor.h>
#include <UI/UIActor.h>
#include <editor/EditorManager.h>
#include <functional>
namespace GEE
{
	class UIButtonActor : public UIActorDefault
	{
	public:
		UIButtonActor(GameScene&, Actor* parentActor, const std::string& name, std::function<void()> onClickFunc = nullptr, std::function<void()> whileBeingClickedFunc = nullptr, const Transform & = Transform());
		UIButtonActor(GameScene&, Actor* parentActor, const std::string& name, const std::string& buttonTextContent, std::function<void()> onClickFunc = nullptr, std::function<void()> whileBeingClickedFunc = nullptr, const Transform & = Transform());

		ModelComponent* GetButtonModel();
		virtual Boxf<Vec2f> GetBoundingBox(bool world = true) override;

		void SetMatIdle(MaterialInstance&&);
		void SetMatHover(MaterialInstance&&);
		void SetMatClick(MaterialInstance&&);
		void SetMatDisabled(MaterialInstance&&);

		void SetDisableInput(bool disable);

		void SetOnClickFunc(std::function<void()> onClickFunc);
		void SetWhileBeingClickedFunc(std::function<void()> whileBeingClickedFunc);

		void DeleteButtonModel();

		virtual void HandleEvent(const Event& ev) override;

		virtual void OnHover();
		virtual void OnUnhover();

		virtual void OnClick();
		virtual void OnBeingClicked();
		virtual void WhileBeingClicked();

		EditorIconState GetState();


		bool ContainsMouse(Vec2f cursorNDC);
	protected:
		virtual void DeduceMaterial();

		std::function<void()> OnClickFunc, WhileBeingClickedFunc;

		ModelComponent* ButtonModel;
		std::shared_ptr<MaterialInstance> MatIdle, MatHover, MatClick, MatDisabled;
		MaterialInstance* PrevDeducedMaterial;

		EditorIconState State;

		bool bInputDisabled;
	};

	class UIActivableButtonActor : public UIButtonActor
	{
	public:
		UIActivableButtonActor(GameScene& scene, Actor* parentActor, const std::string& name, std::function<void()> onClickFunc = nullptr, std::function<void()> onDeactivationFunc = nullptr);

		void SetMatActive(MaterialInstance&&);
		void SetOnDeactivationFunc(std::function<void()> onDeactivationFunc);

		virtual void HandleEvent(const Event& ev) override;
		virtual void OnClick() override;	//On activation
		virtual void OnDeactivation();
	protected:
		virtual void DeduceMaterial() override;

		std::function<void()> OnDeactivationFunc;
		std::shared_ptr<MaterialInstance> MatActive;
	};

	class UIScrollBarActor : public UIButtonActor
	{
	public:
		UIScrollBarActor(GameScene&, Actor* parentActor, const std::string& name, std::function<void()> onClickFunc = nullptr, std::function<void()> whileBeingClickedFunc = nullptr);
		virtual void OnBeingClicked() override;
		virtual void WhileBeingClicked() override;
		const Vec2f& GetClickPosNDC();
		void SetClickPosNDC(const Vec2f&);
	private:
		Vec2f ClickPosNDC;
	};

	struct CollisionTests
	{
	public:
		static bool AlignedRectContainsPoint(const Transform& rect, const Vec2f& point); //treats rect as a rectangle at position rect.PositionRef and size rect.ScaleRef
	};
}
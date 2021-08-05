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
		SharedPtr<MaterialInstance> MatIdle, MatHover, MatClick, MatDisabled;
		MaterialInstance* PrevDeducedMaterial;

		EditorIconState State;

		bool bInputDisabled;
	};

	namespace uiButtonActorUtil
	{
		/**
		 * @brief Set a UIButtonActor's material instances from an atlas from TextureIDs.
		 * @param button: The button which materials will be set.
		 * @param atlasMat: The atlas material to use and take textures from.
		 * @param idleID: ID of the idle texture
		 * @param hoverID: ID of the hover texture. Leave at -1.0f if you don't want to change the hover mat instance.
		 * @param clickID: ID of the click texture. Leave at -1.0f if you don't want to change the click mat instance.
		 * @param disabledID: ID of the disabled texture. Leave at -1.0f if you don't want to change the disabled mat instance.
		*/
		void ButtonMatsFromAtlas(UIButtonActor& button, AtlasMaterial& atlasMat, float idleID, float hoverID = -1.0f, float clickID = -1.0f, float disabledID = -1.0f);
	}

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
		SharedPtr<MaterialInstance> MatActive;
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
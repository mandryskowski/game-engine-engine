#pragma once
#include <scene/UIButtonActor.h>

namespace GEE
{
	class TextComponent;

	class UIInputBoxActor : public UIActivableButtonActor
	{
	public:
		UIInputBoxActor(GameScene&, Actor* parentActor, const std::string& name, const Transform& transform = Transform());
		UIInputBoxActor(GameScene&, Actor* parentActor, const std::string& name, std::function<void(const std::string&)> inputFunc, std::function<std::string()> valueGetter, const Transform& transform = Transform());
		UIInputBoxActor(GameScene&, Actor* parentActor, const std::string& name, std::function<void(float)> inputFunc, std::function<float()> valueGetter, bool fixNumberStr = true, const Transform& transform = Transform());
		UIInputBoxActor(UIInputBoxActor&&);

		virtual void OnStart() override;

		ScrollingTextComponent* GetContentTextComp();
		std::string GetContent();
		void PutString(const std::string&);

		void SetOnInputFunc(std::function<void(const std::string&)> inputFunc, std::function<std::string()> valueGetter);
		void SetOnInputFunc(std::function<void(float)> inputFunc, std::function<float()> valueGetter, bool fixNumberStr = true);
		void SetRetrieveContentEachFrame(bool);
		virtual void SetActive(bool active) override;
		void SetCaretWidthPx(unsigned int pixelWidth);

		void UpdateValue();

		virtual void Update(float deltaTime) override;

		virtual void HandleEvent(const Event& ev) override;
		virtual void OnClick() override;	//On activation
		virtual void OnHover() override;
		virtual void OnUnhover() override;
		virtual void OnDeactivation() override;
	protected:
		virtual void CreateButtonText(const std::string& content) override;
	private:
		void UpdateCaretModel(bool refreshAnim = true);
		void SetCaretPosAndUpdateModel(unsigned int changeCaretPos);
		bool IsCaretInsideInputBox();
		void NormalizeCaretPosition();

		std::function<void()> ValueGetter;
		ScrollingTextComponent* ContentTextComp;
		bool RetrieveContentEachFrame;

		ModelComponent* CaretComponent;	//SetHide() every x seconds
		unsigned int CaretPosition, CaretDirtyFlag;
		float CaretNDCWidth;
		Interpolation CaretAnim;

		Vec2i TextSelectionRange;	//-1, -1 is no selection
	};
}
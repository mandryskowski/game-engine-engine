#pragma once
#include <scene/UIButtonActor.h>

class TextComponent;

class UIInputBoxActor : public UIActivableButtonActor
{
public:
	UIInputBoxActor(GameScene&, const std::string& name);
	//UIInputBoxActor(UIInputBoxActor&&);

	virtual void OnStart() override;

	void PutString(const std::string&);

	void SetOnInputFunc(std::function<void(const std::string&)> inputFunc, std::function<std::string()> valueGetter);
	void SetOnInputFunc(std::function<void(float)> inputFunc, std::function<float()> valueGetter, bool fixNumberStr = true);

	virtual void HandleEvent(const Event& ev) override;
	virtual void OnClick() override;	//On activation
	virtual void OnDeactivation() override;
private:

	std::function<void()> ValueGetter;
	TextComponent* ContentTextComp;
};
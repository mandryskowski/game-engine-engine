#pragma once
#include <scene/UIButtonActor.h>

class TextComponent;
class TextConstantSizeComponent;

class UIInputBoxActor : public UIActivableButtonActor
{
public:
	UIInputBoxActor(GameScene&, Actor* parentActor, const std::string& name);
	UIInputBoxActor(GameScene&, Actor* parentActor, const std::string& name, std::function<void(const std::string&)> inputFunc, std::function<std::string()> valueGetter);
	UIInputBoxActor(GameScene&, Actor* parentActor, const std::string& name, std::function<void(float)> inputFunc, std::function<float()> valueGetter, bool fixNumberStr = true);
	UIInputBoxActor(UIInputBoxActor&&);

	virtual void OnStart() override;

	std::string GetContent();
	void PutString(const std::string&);

	void SetOnInputFunc(std::function<void(const std::string&)> inputFunc, std::function<std::string()> valueGetter);
	void SetOnInputFunc(std::function<void(float)> inputFunc, std::function<float()> valueGetter, bool fixNumberStr = true);
	void SetRetrieveContentEachFrame(bool);

	virtual void Update(float deltaTime) override;

	virtual void HandleEvent(const Event& ev) override;
	virtual void OnClick() override;	//On activation
	virtual void OnDeactivation() override;
private:
	std::function<void()> ValueGetter;
	TextConstantSizeComponent* ContentTextComp;
	bool RetrieveContentEachFrame;
};
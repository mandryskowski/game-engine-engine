#pragma once
#include <scene/Actor.h>
#include <UI/UIActor.h>
#include <editor/EditorManager.h>
#include <functional>

class UIButtonActor : public UIActor
{
public:
	UIButtonActor(GameScene&, const std::string& name, std::function<void()> onClickFunc = nullptr, std::function<void()> whileBeingClickedFunc = nullptr);
	UIButtonActor(GameScene&, const std::string& name, const std::string& buttonTextContent, std::function<void()> onClickFunc = nullptr, std::function<void()> whileBeingClickedFunc = nullptr);

	ModelComponent* GetButtonModel();
	virtual Box2f GetBoundingBox(bool world = true) override;

	void SetMatIdle(MaterialInstance&&);
	void SetMatHover(MaterialInstance&&);
	void SetMatClick(MaterialInstance&&);

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


protected:
	bool ContainsMouse(glm::vec2 cursorNDC);
	virtual void DeduceMaterial();

	std::function<void()> OnClickFunc, WhileBeingClickedFunc;
	
	ModelComponent* ButtonModel;
	std::shared_ptr<MaterialInstance> MatIdle, MatHover, MatClick;
	MaterialInstance* PrevDeducedMaterial;

	EditorIconState State;
};

class UIActivableButtonActor : public UIButtonActor
{
public:
	UIActivableButtonActor(GameScene& scene, const std::string& name, std::function<void()> onClickFunc = nullptr, std::function<void()> onDeactivationFunc = nullptr);

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
	UIScrollBarActor(GameScene&, const std::string& name, std::function<void()> onClickFunc = nullptr, std::function<void()> beingClickedFunc = nullptr);
	virtual void OnBeingClicked() override;
	virtual void WhileBeingClicked() override;
	const glm::vec2& GetClickPosNDC();
	void SetClickPosNDC(const glm::vec2&);
private:
	glm::vec2 ClickPosNDC;
};

struct CollisionTests
{
public:
	static bool AlignedRectContainsPoint(const Transform& rect, const glm::vec2& point); //treats rect as a rectangle at position rect.PositionRef and size rect.ScaleRef
};
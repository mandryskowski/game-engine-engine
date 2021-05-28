#pragma once
#include <rendering/Viewport.h>
#include <math/Transform.h>
#include <functional>

class RenderInfo;
class UICanvasElement;
class Actor;
class UIActor;
class UIScrollBarActor;
class UICanvasField;
class UICanvasFieldCategory;
class UIButtonActor;

class UICanvas
{
public:
	UICanvas(unsigned int canvasDepth = 0): bContainsMouse(false), CanvasDepth(canvasDepth) {}
	UICanvas(const UICanvas&);
	UICanvas(UICanvas&&); 
	virtual NDCViewport GetViewport() const = 0;
	virtual glm::mat4 GetViewMatrix() const = 0;
	virtual const Transform& GetViewT() const;
	virtual const Transform* GetCanvasT() const = 0;
	virtual Transform ToCanvasSpace(const Transform& worldTransform) const = 0;

	virtual glm::mat4 GetProjection() const;

	Boxf<Vec2f> GetBoundingBox() const;	//Canvas space

	unsigned int GetCanvasDepth() const;

	virtual void SetCanvasView(const Transform&);

	virtual void ClampViewToElements();
	void AutoClampView(bool trueHorizontalFalseVertical = true);	//Set to false to set view scale to 100% of vertical size, or leave it as it is to set it to 100% of horizontal size

	virtual UICanvasFieldCategory& AddCategory(const std::string& name) = 0;
	virtual UICanvasField& AddField(const std::string& name, std::function<glm::vec3()> getElementOffset = nullptr) = 0;

	void AddUIElement(UICanvasElement&);
	void EraseUIElement(UICanvasElement&);

	void ScrollView(Vec2f offset);
	void SetViewScale(glm::vec2 scale);
	virtual RenderInfo BindForRender(const RenderInfo&, const glm::uvec2& res) = 0;
	virtual void UnbindForRender(const glm::uvec2& res) = 0;

	bool ContainsMouse() const; //Returns cached value. Guaranteed to be valid if called in HandleEvent() or HandleEventAll() of a child of UICanvasActor.

	/**
	 * @brief Some UIElements should not accept certain events when the cursor is inside a different canvas.
	 * E.g. when the cursor is over a window that is over another window, the buttons in the window below should not be clickable.
	 * Assumes that this UICanvas is the CurrentBlockingCanvas
	 * @see GameScene::GetCurrentBlockingCanvas()
	 * @param element: the element to be checked for the ability to accept blocked events
	 * @return a boolean that indicates whether blocked events should be accepted
	*/
	bool ShouldAcceptBlockedEvents(UICanvasElement& element) const;

	virtual ~UICanvas() {}
//protected:
	bool ContainsMouseCheck(const Vec2f& mouseNDC) const;
	virtual void GetExternalButtons(std::vector<UIButtonActor*>&) const = 0;
public:

	std::vector<std::reference_wrapper<UICanvasElement>> UIElements;
	Transform CanvasView;
	mutable bool bContainsMouse;	//Cache variable
	mutable bool bContainsMouseOutsideTrueCanvas;

	const unsigned int CanvasDepth;
};
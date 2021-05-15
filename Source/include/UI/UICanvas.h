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

class UICanvas
{
public:
	UICanvas(): bContainsMouse(false) {}
	UICanvas(const UICanvas&);
	UICanvas(UICanvas&&); 
	virtual NDCViewport GetViewport() const = 0;
	virtual glm::mat4 GetView() const = 0;
	virtual const Transform& GetViewT() const;
	virtual const Transform* GetCanvasT() const = 0;
	virtual Transform ToCanvasSpace(const Transform& worldTransform) const = 0;

	virtual glm::mat4 GetProjection() const;

	Boxf<Vec2f> GetBoundingBox() const;	//Canvas space

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
protected:
	bool ContainsMouseCheck(const Vec2f& mouseNDC) const;
public:

	std::vector<std::reference_wrapper<UICanvasElement>> UIElements;
	Transform CanvasView;
	mutable bool bContainsMouse;	//Cache variable
};
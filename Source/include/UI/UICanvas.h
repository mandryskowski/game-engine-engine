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
	UICanvas(){}
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
	void AddUIElement(Actor&);	//Add UIElements that are components of the actor
	void AddUIElement(UIActor&);	//Add the actor and its components
	void EraseUIElement(UICanvasElement&);
	void RemoveUIElement(UICanvasElement*);
	void ScrollView(Vec2f offset);
	void SetViewScale(glm::vec2 scale);
	virtual RenderInfo BindForRender(const RenderInfo&, const glm::uvec2& res) = 0;
	virtual void UnbindForRender(const glm::uvec2& res) = 0;

	std::vector<std::reference_wrapper<UICanvasElement>> UIElements;
	Transform CanvasView;
};
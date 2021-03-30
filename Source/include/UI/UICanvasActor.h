#pragma once
#include <scene/Actor.h>
#include <scene/RenderableComponent.h>
#include <UI/UICanvas.h>

class UIAutomaticListActor;
class UIManualListActor;
class UICanvasField;
class UIButtonActor;

class UICanvasActor : public Actor, public UICanvas
{
public:
	UICanvasActor(GameScene&, const std::string& name);
	UICanvasActor(UICanvasActor&&);

	virtual void OnStart() override;

	UIActor* GetScaleActor();
	virtual glm::mat4 GetView() const override;
	virtual NDCViewport GetViewport() const override;
	virtual const Transform* GetCanvasT() const override;
	virtual Transform ToCanvasSpace(const Transform& worldTransform) const override;

	void HideScrollBars();

	virtual UICanvasField& AddField(const std::string& name, std::function<glm::vec3()> getElementOffset = nullptr) override;

	virtual void HandleEvent(const Event& ev) override;

	virtual RenderInfo BindForRender(const RenderInfo&, const glm::uvec2& res) override;
	virtual void UnbindForRender(const glm::uvec2& res) override;

private:
	void CreateScrollBars();
	template <VecAxis barAxis> void UpdateScrollBarT();
public:
	UIActor* ScaleActor;
	UIManualListActor* FieldsList;
	glm::vec3 FieldSize;
	UIScrollBarActor* ScrollBarX, *ScrollBarY, *BothScrollBarsButton;
	UIScrollBarActor *ResizeBarX, *ResizeBarY;
};

UICanvasField& AddFieldToCanvas(const std::string& name, UICanvasElement& element);
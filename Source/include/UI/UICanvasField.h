#pragma once
#include <UI/UIActor.h>
#include <UI/UIElement.h>
#include <functional>

class UIElementTemplates;

/*
class UIListActor : public UIActor
{
public:
	UIListActor(GameScene& scene, const std::string& name, glm::vec3 elementOffset = glm::vec3(0.0, -2.0f, 0.0f));
	UIListActor(UIListActor&&);
	virtual Actor& AddChild(std::unique_ptr<Actor> actor) override;
	virtual void Refresh();
	glm::vec3 GetListOffset();

	virtual glm::vec3 MoveElement(Actor& element, glm::vec3 nextElementBegin, float level);
	virtual	glm::vec3 MoveElements(std::vector<Actor*> currentLevelElements, unsigned int level);

protected:
	glm::vec3 ElementOffset;
};

class UIMultipleListActor : public UIListActor
{
public:
	using UIListActor::UIListActor;

	void NestList(UIListActor&);

	//virtual glm::vec3 MoveElement(Actor& element, glm::vec3 nextElementBegin, float level);
	virtual	glm::vec3 MoveElements(std::vector<Actor*> currentLevelElements, unsigned int level) override;

private:
	std::vector<UIListActor*> NestedLists;
};*/

class UICanvasField : public UIActor
{
public:
	UICanvasField(GameScene&, const std::string& name);
	virtual void OnStart() override;
	UIElementTemplates GetTemplates();
private:
};


class UIElementTemplates	//TODO: Wyjeb UICanvas* Canvas
{
public:
	UIElementTemplates(Actor&, UICanvas*);
	void TickBox(bool& modifiedBool);
	void TickBox(std::function<bool()> setFunc);

	template <int vecSize> void VecInput(std::function<void(float, float)> setFunc, std::function<float(float)> getFunc);	//encapsulated
	template <typename VecType> void VecInput(std::function<void(float, float)> setFunc, std::function<float(float)> getFunc);	//encapsulated
	template <typename VecType> void VecInput(VecType& modifedVec);	//not encapsulated

	template <typename ObjectBase, typename ObjectType> void ObjectInput(ObjectBase& hierarchyRoot, std::function<void(ObjectType*)> setFunc);	//encapsulated
	template <typename ObjectBase, typename ObjectType> void ObjectInput(ObjectBase& hierarchyRoot, ObjectType*& inputTo);	//not encapsulated
	template <typename CompType> void ComponentInput(Component& hierarchyRoot, std::function<void(CompType*)> setFunc);	//encapsulated
	template <typename CompType> void ComponentInput(Component& hierarchyRoot, CompType*& inputTo);	//not encapsulated

	void PathInput(std::function<void(const std::string&)> setFunc, std::function<std::string()> getFunc);	//encapsulated

private:
	Actor& TemplateParent;
	UICanvas* Canvas;
	GameScene& Scene;
	GameManager& GameHandle;
};

template <typename ObjectType> void GetAllObjects(Component& hierarchyRoot, std::vector<ObjectType*>&);
template <typename ObjectType> void GetAllObjects(Actor& hierarchyRoot, std::vector<ObjectType*>&);
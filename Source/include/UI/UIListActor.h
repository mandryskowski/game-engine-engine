#pragma once
#include <UI/UIActor.h>
#include <functional>

class UIListActor : public UIActor
{
public:
	UIListActor(GameScene& scene, const std::string& name);
	UIListActor(UIListActor&&);

	virtual Actor& AddChild(std::unique_ptr<Actor> actor) override;
	virtual void Refresh();

	virtual glm::vec3 GetListOffset() = 0;
	virtual glm::vec3 GetListBegin() = 0;

	void NestList(UIListActor&);

	virtual	glm::vec3 MoveElements(unsigned int level = 0) = 0;

protected:
	std::vector<UIListActor*> NestedLists;
};

class UIAutomaticListActor : public UIListActor
{
public:
	UIAutomaticListActor(GameScene& scene, const std::string& name, glm::vec3 elementOffset = glm::vec3(0.0, -2.0f, 0.0f));
	UIAutomaticListActor(UIAutomaticListActor&&);
	virtual glm::vec3 GetListOffset() override;
	virtual glm::vec3 GetListBegin() override;

	glm::vec3 MoveElement(Actor& element, glm::vec3 nextElementBegin, float level);
	virtual	glm::vec3 MoveElements(unsigned int level = 0) override;

private:
	glm::vec3 ElementOffset;
};

class UIListElement
{
public:
	UIListElement(Actor& actorRef, std::function<glm::vec3()> getElementOffset, std::function<glm::vec3()> getCenterOffset = nullptr);
	UIListElement(Actor& actorRef, const glm::vec3& constElementOffset);
	UIListElement(const UIListElement&);
	UIListElement(UIListElement&&);
	UIListElement& operator=(const UIListElement&);
	UIListElement& operator=(UIListElement&&);
	Actor& GetActorRef();
	const Actor& GetActorRef() const;
	glm::vec3 GetElementOffset() const;
	glm::vec3 GetCenterOffset() const;
	void SetGetElementOffsetFunc(std::function<glm::vec3()>);
	void SetGetCenterOffsetFunc(std::function<glm::vec3()>);

private:
	Actor& ActorRef;
	std::function<glm::vec3()> GetElementOffsetFunc, GetCenterOffsetFunc;
};

class UIManualListActor : public UIListActor
{
public:
	using UIListActor::UIListActor;
	virtual glm::vec3 GetListOffset() override;
	virtual glm::vec3 GetListBegin() override;

	glm::vec3 MoveElement(UIListElement& element, glm::vec3 nextElementBegin, float level);
	virtual	glm::vec3 MoveElements(unsigned int level = 0) override;

	void AddElement(const UIListElement&);
	
	void SetListElementOffset(int index, std::function<glm::vec3()> getElementOffset);
	void SetListCenterOffset(int index, std::function<glm::vec3()> getCenterOffset);
	int GetListElementCount();

private:
	std::vector<UIListElement> ListElements;
};

/*
class UIListElement
{
public:
	UIListElement(std::function<glm::vec3()> getElementOffset, std::function<void(const glm::vec3&)> setElementPos);
	glm::vec3 GetElementOffset();
};

class UIManualListActor : public UIListActor
{
public:

private:
	std::vector<UIListElement> ListElements;
};
*/
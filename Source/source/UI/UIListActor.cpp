#include <UI/UIListActor.h>
#include <scene/Component.h>
#include <math/Transform.h>

UIListActor::UIListActor(GameScene& scene, const std::string& name) :
	UIActor(scene, name)
{
}

UIListActor::UIListActor(UIListActor&& listActor) :
	UIActor(std::move(listActor))
{
}

Actor& UIListActor::AddChild(std::unique_ptr<Actor> actor)
{
	Actor& actorRef = Actor::AddChild(std::move(actor));
	return actorRef;
}

void UIListActor::Refresh()
{
	MoveElements(0);
}


void UIListActor::NestList(UIListActor& list)
{
	NestedLists.push_back(&list);
}

UIAutomaticListActor::UIAutomaticListActor(GameScene& scene, const std::string& name, glm::vec3 elementOffset):
	UIListActor(scene, name),
	ElementOffset(elementOffset)
{
}

UIAutomaticListActor::UIAutomaticListActor(UIAutomaticListActor&& listActor):
	UIListActor(std::move(listActor)),
	ElementOffset(listActor.ElementOffset)
{
}

glm::vec3 UIAutomaticListActor::GetListOffset()
{
	glm::vec3 offset = ElementOffset * static_cast<float>(Children.size());
	for (auto& it : Children)
		if (UIListActor* cast = dynamic_cast<UIListActor*>(it.get()))
			offset += cast->GetListOffset() - ElementOffset;	//subtract ElementOffset to avoid offseting twice

	std::cout << Name << " list offset: " << (glm::mat3(GetTransform()->GetMatrix()) * offset).y << ". Children: " << Children.size() << ": \n";
	for (auto& it : Children)
	{
		if (UIListActor* cast = dynamic_cast<UIListActor*>(it.get()))
			std::cout << "LIST";
		std::cout << it->GetName() << " " << ElementOffset.y << '\n';
	}
	return glm::mat3(GetTransform()->GetMatrix()) * offset;
}

glm::vec3 UIAutomaticListActor::GetListBegin()
{
	if (Children.empty())
		return glm::vec3(0.0f);

	return Children.front()->GetTransform()->PositionRef - ElementOffset / 2.0f;
}

glm::vec3 UIAutomaticListActor::MoveElement(Actor& element, glm::vec3 nextElementBegin, float level)
{
	element.GetTransform()->SetPosition(nextElementBegin + ElementOffset / 2.0f);

	return nextElementBegin + ElementOffset;
}

glm::vec3 UIAutomaticListActor::MoveElements(unsigned int level)
{
	glm::vec3 nextElementBegin = GetListBegin();
	for (auto& element : Children)
	{
		if (element->GetName() == "OGAR")
			continue;
		nextElementBegin = MoveElement(*element, nextElementBegin, level);

		if (!NestedLists.empty())
		{
			auto found = std::find(NestedLists.begin(), NestedLists.end(), element.get());
			if (found != NestedLists.end())
				nextElementBegin += (*found)->GetListOffset() - ElementOffset;	//subtract ElementOffset to avoid offseting twice (we already applied it in MoveElement)
		}
	}

	for (auto& list : NestedLists)
		nextElementBegin = list->MoveElements(level + 1);

	return nextElementBegin;
}

glm::vec3 UIManualListActor::GetListOffset()
{
	glm::vec3 offset(0.0f);
	for (auto& it : ListElements)
	{
		offset += it.GetElementOffset();
	}

	return glm::mat3(GetTransform()->GetMatrix()) * offset;
}

glm::vec3 UIManualListActor::GetListBegin()
{
	if (Children.empty())
		return glm::vec3(0.0f);

	return ListElements.front().GetActorRef().GetTransform()->PositionRef - ListElements.front().GetCenterOffset();
}

glm::vec3 UIManualListActor::MoveElement(UIListElement& element, glm::vec3 nextElementBegin, float level)
{
	const glm::vec3& elementOffset = element.GetElementOffset();
	element.GetActorRef().GetTransform()->SetPosition(nextElementBegin + element.GetCenterOffset());
	return glm::vec3(nextElementBegin + elementOffset);
}

glm::vec3 UIManualListActor::MoveElements(unsigned int level)
{
	glm::vec3 nextElementBegin = GetListBegin();

	for (auto& element : ListElements)
	{
		nextElementBegin = MoveElement(element, nextElementBegin, level);

		/*if (!NestedLists.empty())
		{
			auto found = std::find(NestedLists.begin(), NestedLists.end(), &element.GetActorRef());
			if (found != NestedLists.end())
				nextElementBegin += (*found)->GetListOffset() - element.GetElementOffset();	//subtract ElementOffset to avoid offseting twice (we already applied it in MoveElement)
		}*/
	}

	//for (auto& list : NestedLists)
		//nextElementBegin = list->MoveElements(level + 1);

	return nextElementBegin;
}

void UIManualListActor::AddElement(const UIListElement& element)
{
	ListElements.push_back(element);
}

void UIManualListActor::SetListElementOffset(int index, std::function<glm::vec3()> getElementOffset)
{
	if (index < 0 || index > ListElements.size() || !getElementOffset)
		return;

	ListElements[index].SetGetElementOffsetFunc(getElementOffset);
}

void UIManualListActor::SetListCenterOffset(int index, std::function<glm::vec3()> getCenterOffset)
{
	if (index < 0 || index > ListElements.size() || !getCenterOffset)
		return;

	ListElements[index].SetGetCenterOffsetFunc(getCenterOffset);
}

int UIManualListActor::GetListElementCount()
{
	return static_cast<int>(ListElements.size());
}

UIListElement::UIListElement(Actor& actorRef, std::function<glm::vec3()> getElementOffset, std::function<glm::vec3()> getCenterOffset):
	ActorRef(actorRef),
	GetElementOffsetFunc(getElementOffset),
	GetCenterOffsetFunc(getCenterOffset)
{
	if (!GetCenterOffsetFunc && GetElementOffsetFunc)
		GetCenterOffsetFunc = [this]() -> glm::vec3 { return GetElementOffsetFunc() / 2.0f; };
}

UIListElement::UIListElement(Actor& actorRef, const glm::vec3& constElementOffset):
	UIListElement(actorRef, [constElementOffset]() { return constElementOffset; }, [constElementOffset]() { return constElementOffset / 2.0f; })
{
}

Actor& UIListElement::GetActorRef()
{
	return ActorRef;
}

glm::vec3 UIListElement::GetElementOffset() const
{
	return (GetElementOffsetFunc) ? (GetElementOffsetFunc()) : (glm::vec3(0.0f));
}

glm::vec3 UIListElement::GetCenterOffset() const
{
	return (GetCenterOffsetFunc) ? (GetCenterOffsetFunc()) : (glm::vec3(0.0f));
}

void UIListElement::SetGetElementOffsetFunc(std::function<glm::vec3()> getElementOffset)
{
	GetElementOffsetFunc = getElementOffset;
}

void UIListElement::SetGetCenterOffsetFunc(std::function<glm::vec3()> getCenterOffset)
{
	GetCenterOffsetFunc = getCenterOffset;
}

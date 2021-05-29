#include <UI/UIListActor.h>
#include <scene/Component.h>
#include <math/Transform.h>

namespace GEE
{
	UIListActor::UIListActor(GameScene& scene, Actor* parentActor, const std::string& name) :
		UIActorDefault(scene, parentActor, name),
		bExpanded(true)
	{
	}

	UIListActor::UIListActor(UIListActor&& listActor) :
		UIActorDefault(std::move(listActor)),
		bExpanded(listActor.bExpanded)
	{
	}

	void UIListActor::Refresh()
	{
		MoveElements();
	}

	glm::vec3 UIListActor::GetListOffset()
	{
		glm::vec3 offset(0.0f);
		for (auto& it : ListElements)
			offset += it.GetElementOffset();

		return glm::mat3(GetTransform()->GetMatrix()) * offset;
	}

	glm::vec3 UIListActor::GetListBegin()
	{
		if (ListElements.empty())
			return glm::vec3(0.0f);

		return ListElements.front().GetActorRef().GetTransform()->PositionRef - ListElements.front().GetCenterOffset();
	}

	int UIListActor::GetListElementCount() const
	{
		return static_cast<int>(ListElements.size());
	}

	void UIListActor::SetListElementOffset(int index, std::function<glm::vec3()> getElementOffset)
	{
		if (index < 0 || index > ListElements.size() || !getElementOffset)
			return;

		ListElements[index].SetGetElementOffsetFunc(getElementOffset);
	}

	void UIListActor::SetListCenterOffset(int index, std::function<glm::vec3()> getCenterOffset)
	{
		if (index < 0 || index > ListElements.size() || !getCenterOffset)
			return;

		ListElements[index].SetGetCenterOffsetFunc(getCenterOffset);
	}

	void UIListActor::AddElement(const UIListElement& element)
	{
		ListElements.push_back(element);
	}

	glm::vec3 UIListActor::MoveElement(UIListElement& element, glm::vec3 nextElementBegin, float level)
	{
		const glm::vec3& elementOffset = element.GetElementOffset();
		element.GetActorRef().GetTransform()->SetPosition(nextElementBegin + element.GetCenterOffset());
		return glm::vec3(nextElementBegin + elementOffset);
	}

	glm::vec3 UIListActor::MoveElements(unsigned int level)
	{
		EraseListElement([](const UIListElement& element) { return element.IsBeingKilled(); });
		glm::vec3 nextElementBegin = GetListBegin();

		for (auto& element : ListElements)
		{
			if (element.GetActorRef().GetName() == "OGAR")
				continue;
			nextElementBegin = MoveElement(element, nextElementBegin, level);
		}

		return nextElementBegin;
	}

	void UIListActor::EraseListElement(std::function<bool(const UIListElement&)> removeFunc)
	{
		ListElements.erase(std::remove_if(ListElements.begin(), ListElements.end(), [&removeFunc](const UIListElement& element) { return removeFunc(element); }), ListElements.end());
	}

	void UIListActor::EraseListElement(Actor& actor)
	{
		EraseListElement([&actor](const UIListElement& listElement) { return &listElement.GetActorRef() == &actor; });
	}

	UIAutomaticListActor::UIAutomaticListActor(GameScene& scene, Actor* parentActor, const std::string& name, glm::vec3 elementOffset) :
		UIListActor(scene, parentActor, name),
		ElementOffset(elementOffset)
	{
	}

	UIAutomaticListActor::UIAutomaticListActor(UIAutomaticListActor&& listActor) :
		UIListActor(std::move(listActor)),
		ElementOffset(listActor.ElementOffset)
	{
	}

	Actor& UIAutomaticListActor::AddChild(std::unique_ptr<Actor> actor)
	{
		AddElement(UIListElement(*actor, ElementOffset));
		if (UIListActor* listCast = dynamic_cast<UIListActor*>(actor.get()))
			ListElements.back().SetGetElementOffsetFunc([listCast]() -> Vec3f { listCast->Refresh(); return listCast->GetListOffset(); });

		return UIListActor::AddChild(std::move(actor));
	}

	UIListElement::UIListElement(Actor& actorRef, std::function<glm::vec3()> getElementOffset, std::function<glm::vec3()> getCenterOffset) :
		ActorRef(actorRef),
		GetElementOffsetFunc(getElementOffset),
		GetCenterOffsetFunc(getCenterOffset)
	{
		if (!GetCenterOffsetFunc)
			GetCenterOffsetFunc = [this]() -> glm::vec3 { return (GetElementOffsetFunc) ? (GetElementOffsetFunc() / 2.0f) : (glm::vec3(0.0f)); };
	}

	UIListElement::UIListElement(Actor& actorRef, const glm::vec3& constElementOffset) :
		UIListElement(actorRef, [constElementOffset]() { return constElementOffset; }, [constElementOffset]() { return constElementOffset / 2.0f; })
	{
	}

	UIListElement::UIListElement(const UIListElement& element) :
		ActorRef(element.ActorRef), GetElementOffsetFunc(element.GetElementOffsetFunc), GetCenterOffsetFunc(element.GetCenterOffsetFunc)
	{
	}

	UIListElement::UIListElement(UIListElement&& element) :
		UIListElement(static_cast<const UIListElement&>(element))
	{
	}

	UIListElement& UIListElement::operator=(const UIListElement& element)
	{
		GetElementOffsetFunc = element.GetElementOffsetFunc;
		GetCenterOffsetFunc = element.GetCenterOffsetFunc;
		return *this;
	}

	UIListElement& UIListElement::operator=(UIListElement&& element)
	{
		return (*this = static_cast<const UIListElement&>(element));
	}

	bool UIListElement::IsBeingKilled() const
	{
		return ActorRef.IsBeingKilled();
	}

	Actor& UIListElement::GetActorRef()
	{
		return ActorRef;
	}

	const Actor& UIListElement::GetActorRef() const
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

}
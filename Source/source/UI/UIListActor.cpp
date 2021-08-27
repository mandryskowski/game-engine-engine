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

	Vec3f UIListActor::GetListOffset()
	{
		Vec3f offset(0.0f);
		for (auto& it : ListElements)
			offset += it.GetElementOffset();

		return Mat3f(GetTransform()->GetMatrix()) * offset;
	}

	Vec3f UIListActor::GetListBegin()
	{
		if (ListElements.empty())
			return Vec3f(0.0f);

		return ListElements.front().GetActorRef().GetTransform()->GetPos() - ListElements.front().GetCenterOffset();
	}

	const UIListElement UIListActor::GetListElement(unsigned int index) const
	{
		GEE_CORE_ASSERT(index < ListElements.size());
		return ListElements[index];
	}

	int UIListActor::GetListElementCount() const
	{
		return static_cast<int>(ListElements.size());
	}

	void UIListActor::SetListElementOffset(int index, std::function<Vec3f()> getElementOffset)
	{
		if (index < 0 || index > ListElements.size() || !getElementOffset)
			return;

		ListElements[index].SetGetElementOffsetFunc(getElementOffset);
	}

	void UIListActor::SetListCenterOffset(int index, std::function<Vec3f()> getCenterOffset)
	{
		if (index < 0 || index > ListElements.size() || !getCenterOffset)
			return;

		ListElements[index].SetGetCenterOffsetFunc(getCenterOffset);
	}

	void UIListActor::AddElement(const UIListElement& element)
	{
		ListElements.push_back(element);
	}

	Vec3f UIListActor::MoveElement(UIListElement& element, Vec3f nextElementBegin, float level)
	{
		const Vec3f& elementOffset = element.GetElementOffset();
		element.GetActorRef().GetTransform()->SetPosition(nextElementBegin + element.GetCenterOffset());
		return Vec3f(nextElementBegin + elementOffset);
	}

	Vec3f UIListActor::MoveElements(unsigned int level)
	{
		EraseListElement([](const UIListElement& element) { return element.IsBeingKilled(); });
		Vec3f nextElementBegin = GetListBegin();

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

	UIAutomaticListActor::UIAutomaticListActor(GameScene& scene, Actor* parentActor, const std::string& name, Vec3f elementOffset) :
		UIListActor(scene, parentActor, name),
		ElementOffset(elementOffset)
	{
	}

	UIAutomaticListActor::UIAutomaticListActor(UIAutomaticListActor&& listActor) noexcept :
		UIListActor(std::move(listActor)),
		ElementOffset(listActor.ElementOffset)
	{
	}

	Actor& UIAutomaticListActor::AddChild(UniquePtr<Actor> actor)
	{
		AddElement(UIListElement(*actor, ElementOffset));
		if (UIListActor* listCast = dynamic_cast<UIListActor*>(actor.get()))
			ListElements.back().SetGetElementOffsetFunc([listCast]() -> Vec3f { listCast->Refresh(); return listCast->GetListOffset(); });

		return UIListActor::AddChild(std::move(actor));
	}

	UIListElement::UIListElement(Actor& actorRef, std::function<Vec3f()> getElementOffset, std::function<Vec3f()> getCenterOffset) :
		ActorPtr(actorRef),
		GetElementOffsetFunc(getElementOffset),
		GetCenterOffsetFunc(getCenterOffset)
	{
		if (!GetCenterOffsetFunc)
			GetCenterOffsetFunc = [this]() -> Vec3f { return (GetElementOffsetFunc) ? (GetElementOffsetFunc() / 2.0f) : (Vec3f(0.0f)); };
	}

	UIListElement::UIListElement(Actor& actorRef, const Vec3f& constElementOffset) :
		UIListElement(actorRef, [constElementOffset]() { return constElementOffset; }, [constElementOffset]() { return constElementOffset / 2.0f; })
	{
	}

	UIListElement::UIListElement(const UIListElement& element) :
		ActorPtr(element.ActorPtr), GetElementOffsetFunc(element.GetElementOffsetFunc), GetCenterOffsetFunc(element.GetCenterOffsetFunc)
	{
	}

	UIListElement::UIListElement(UIListElement&& element) :
		UIListElement(static_cast<const UIListElement&>(element))
	{
	}

	UIListElement& UIListElement::operator=(const UIListElement& element)
	{
		ActorPtr = element.ActorPtr;
		GetElementOffsetFunc = element.GetElementOffsetFunc;
		GetCenterOffsetFunc = element.GetCenterOffsetFunc;
		return *this;
	}

	UIListElement& UIListElement::operator=(UIListElement&& element)
	{
		return (*this = element);
	}

	bool UIListElement::IsBeingKilled() const
	{
		return ActorPtr.Get().IsBeingKilled();
	}

	Actor& UIListElement::GetActorRef()
	{
		return ActorPtr.Get();
	}

	const Actor& UIListElement::GetActorRef() const
	{
		return ActorPtr.Get();
	}

	Vec3f UIListElement::GetElementOffset() const
	{
		return (GetElementOffsetFunc) ? (GetElementOffsetFunc()) : (Vec3f(0.0f));
	}

	Vec3f UIListElement::GetCenterOffset() const
	{
		return (GetCenterOffsetFunc) ? (GetCenterOffsetFunc()) : (Vec3f(0.0f));
	}

	void UIListElement::SetGetElementOffsetFunc(std::function<Vec3f()> getElementOffset)
	{
		GetElementOffsetFunc = getElementOffset;
	}

	void UIListElement::SetGetCenterOffsetFunc(std::function<Vec3f()> getCenterOffset)
	{
		GetCenterOffsetFunc = getCenterOffset;
	}

}
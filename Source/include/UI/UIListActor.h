#pragma once
#include <UI/UIActor.h>
#include <functional>

namespace GEE
{
	class UIListElement
	{
		template <typename T>
		class NonNullPtr
		{
		public:
			NonNullPtr(T& obj) : Ptr(&obj) {}
			NonNullPtr(const NonNullPtr<T>& ptr) : Ptr(ptr.Ptr) {}
			NonNullPtr(NonNullPtr<T>&& ptr) : Ptr(ptr.Ptr) {}
			T& Get() { return *Ptr; };
			const T& Get() const { return *Ptr; };
			NonNullPtr& operator=(const NonNullPtr<T>& ptr) { Ptr = ptr.Ptr; return *this; }
			NonNullPtr& operator=(NonNullPtr<T>&& ptr) { return *this = static_cast<const NonNullPtr<T>&>(ptr); }
			NonNullPtr& operator=(const T& obj) { Ptr = &obj; return *this; };
		private:
			T* Ptr;
		};
	public:
		UIListElement(Actor& actorRef, std::function<Vec3f()> getElementOffset, std::function<Vec3f()> getCenterOffset = nullptr);
		UIListElement(Actor& actorRef, const Vec3f& constElementOffset);
		UIListElement(const UIListElement&);
		UIListElement(UIListElement&&);
		UIListElement& operator=(const UIListElement&);
		UIListElement& operator=(UIListElement&&);
		bool IsBeingKilled() const;
		Actor& GetActorRef();
		const Actor& GetActorRef() const;
		Vec3f GetElementOffset() const;
		Vec3f GetCenterOffset() const;

		void SetGetElementOffsetFunc(std::function<Vec3f()>);
		void SetGetCenterOffsetFunc(std::function<Vec3f()>);

	private:
		NonNullPtr<Actor> ActorPtr;
		std::function<Vec3f()> GetElementOffsetFunc, GetCenterOffsetFunc;
	};

	class UIListActor : public UIActorDefault
	{
	public:
		UIListActor(GameScene& scene, Actor* parentActor, const std::string& name);
		UIListActor(UIListActor&&);

		void Refresh();

		virtual Vec3f GetListOffset();
		Vec3f GetListBegin();

		int GetListElementCount() const;

		void SetListElementOffset(int index, std::function<Vec3f()> getElementOffset);
		void SetListCenterOffset(int index, std::function<Vec3f()> getCenterOffset);

		void AddElement(const UIListElement&);

		Vec3f MoveElement(UIListElement& element, Vec3f nextElementBegin, float level);
		Vec3f MoveElements(unsigned int level = 0);

		void EraseListElement(std::function<bool(const UIListElement&)>);
		void EraseListElement(Actor&);


	protected:
		std::vector<UIListElement> ListElements;
		bool bExpanded;
	};

	class UIAutomaticListActor : public UIListActor
	{
	public:
		UIAutomaticListActor(GameScene& scene, Actor* parentActor, const std::string& name, Vec3f elementOffset = Vec3f(0.0, -2.0f, 0.0f));
		UIAutomaticListActor(UIAutomaticListActor&&);

		virtual Actor& AddChild(std::unique_ptr<Actor>) override;

	private:
		Vec3f ElementOffset;
	};
}
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
		UIListElement(Actor& actorRef, std::function<glm::vec3()> getElementOffset, std::function<glm::vec3()> getCenterOffset = nullptr);
		UIListElement(Actor& actorRef, const glm::vec3& constElementOffset);
		UIListElement(const UIListElement&);
		UIListElement(UIListElement&&);
		UIListElement& operator=(const UIListElement&);
		UIListElement& operator=(UIListElement&&);
		bool IsBeingKilled() const;
		Actor& GetActorRef();
		const Actor& GetActorRef() const;
		glm::vec3 GetElementOffset() const;
		glm::vec3 GetCenterOffset() const;

		void SetGetElementOffsetFunc(std::function<glm::vec3()>);
		void SetGetCenterOffsetFunc(std::function<glm::vec3()>);

	private:
		NonNullPtr<Actor> ActorPtr;
		std::function<glm::vec3()> GetElementOffsetFunc, GetCenterOffsetFunc;
	};

	class UIListActor : public UIActorDefault
	{
	public:
		UIListActor(GameScene& scene, Actor* parentActor, const std::string& name);
		UIListActor(UIListActor&&);

		void Refresh();

		virtual glm::vec3 GetListOffset();
		glm::vec3 GetListBegin();

		int GetListElementCount() const;

		void SetListElementOffset(int index, std::function<glm::vec3()> getElementOffset);
		void SetListCenterOffset(int index, std::function<glm::vec3()> getCenterOffset);

		void AddElement(const UIListElement&);

		glm::vec3 MoveElement(UIListElement& element, glm::vec3 nextElementBegin, float level);
		glm::vec3 MoveElements(unsigned int level = 0);

		void EraseListElement(std::function<bool(const UIListElement&)>);
		void EraseListElement(Actor&);


	protected:
		std::vector<UIListElement> ListElements;
		bool bExpanded;
	};

	class UIAutomaticListActor : public UIListActor
	{
	public:
		UIAutomaticListActor(GameScene& scene, Actor* parentActor, const std::string& name, glm::vec3 elementOffset = glm::vec3(0.0, -2.0f, 0.0f));
		UIAutomaticListActor(UIAutomaticListActor&&);

		virtual Actor& AddChild(std::unique_ptr<Actor>) override;

	private:
		glm::vec3 ElementOffset;
	};
}
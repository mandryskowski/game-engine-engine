#pragma once
#include <UI/UIActor.h>
#include <UI/UIElement.h>
#include <functional>

namespace GEE
{
	class UIElementTemplates;
	class UIAutomaticListActor;

	class UICanvasField : public Actor, public UIActor
	{
	public:
		UICanvasField(GameScene&, Actor* parentActor, const std::string& name);
		virtual void OnStart() override;
		UIElementTemplates GetTemplates();
	};


	class UIElementTemplates
	{
	public:
		UIElementTemplates(Actor&);
		void TickBox(bool& modifiedBool);
		void TickBox(std::function<bool()> setFunc);
		void TickBox(std::function<void(bool)> setFunc, std::function<bool()> getFunc);

		template <int vecSize> void VecInput(std::function<void(float, float)> setFunc, std::function<float(float)> getFunc);	//encapsulated
		template <typename VecType> void VecInput(std::function<void(float, float)> setFunc, std::function<float(float)> getFunc);	//encapsulated
		template <typename VecType> void VecInput(VecType& modifedVec);	//not encapsulated

		//
		template <typename ObjectBase, typename ObjectType> void ObjectInput(std::function<std::vector<ObjectBase*>()> getObjectsFunc, std::function<void(ObjectType*)> setFunc);	//encapsulated
		template <typename ObjectBase, typename ObjectType> void ObjectInput(ObjectBase& hierarchyRoot, std::function<void(ObjectType*)> setFunc);	//encapsulated
		template <typename ObjectBase, typename ObjectType> void ObjectInput(ObjectBase& hierarchyRoot, ObjectType*& inputTo);	//not encapsulated
		template <typename CompType> void ComponentInput(Component& hierarchyRoot, std::function<void(CompType*)> setFunc);	//encapsulated
		template <typename CompType> void ComponentInput(GameScene& scene, std::function<void(CompType*)> setFunc);	//encapsulated
		template <typename CompType> void ComponentInput(Component& hierarchyRoot, CompType*& inputTo);	//not encapsulated

		template <typename InputIt, typename T> void ListSelection(InputIt first, InputIt last, std::function<void(UIButtonActor&, T&)> buttonFunc)
		{
			ListSelection(first, last, [](UIAutomaticListActor& listActor, T& object) { buttonFunc(listActor.CreateChild<UIButtonActor>("ListElementActor"), object); })
		}
		template < typename T, typename InputIt> void ListSelection(InputIt first, InputIt last, std::function<void(UIAutomaticListActor&, T&)> buttonFunc)
		{
			UIAutomaticListActor& listActor = TemplateParent.CreateChild<UIAutomaticListActor>("ListSelectionActor");
			std::cout << "WYWOLALEM LIST SELECTION " << &(*first) << &(*last) << '\n';
			std::for_each(first, last, [&listActor, buttonFunc](T& object) { buttonFunc(listActor, object); std::cout << " TWORZE PRZYCISKA"; });
			listActor.Refresh();
		}

	private:
		void HierarchyTreeInput(UIAutomaticListActor& list, GameScene&, std::function<void(HierarchyTemplate::HierarchyTreeT&)> setFunc);
	public:
		void HierarchyTreeInput(GameScene&, std::function<void(HierarchyTemplate::HierarchyTreeT&)> setFunc);
		void HierarchyTreeInput(GameManager&, std::function<void(HierarchyTemplate::HierarchyTreeT&)> setFunc);

		void PathInput(std::function<void(const std::string&)> setFunc, std::function<std::string()> getFunc, std::vector<const char*> extensions);	//encapsulated
		void FolderInput(std::function<void(const std::string&)> setFunc, std::function<std::string()> getFunc);	//encapsulated

	private:
		Actor& TemplateParent;
		GameScene& Scene;
		GameManager& GameHandle;
	};

	template <typename ObjectType> void GetAllObjects(Component& hierarchyRoot, std::vector<ObjectType*>&);
	template <typename ObjectType> void GetAllObjects(Actor& hierarchyRoot, std::vector<ObjectType*>&);
}
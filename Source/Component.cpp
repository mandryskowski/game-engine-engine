#include "Component.h"
#include "FileLoader.h"

Component::Component(GameManager* gameHandle, std::string name, const Transform& t):
	Name(name), ComponentTransform(t), GameHandle(gameHandle)
{
}
Component::Component(GameManager* gameHandle, std::string name, glm::vec3 pos, glm::vec3 rot, glm::vec3 scale):
	Name(name), ComponentTransform(pos, rot, scale), GameHandle(gameHandle)
{
}

void Component::Setup(std::stringstream& filestr, Actor* myActor)
{
	///////////////////////////////1. Load the type and the name of the current component
	SearchEngine* searcher = GameHandle->GetSearchEngine();
	RenderEngineManager* renderEngHandle = GameHandle->GetRenderEngineHandle();

	Component* comp = nullptr;
	std::string type, name;
	filestr >> type;
	name = multipleWordInput(filestr);

	///////////////////////////////2. Load info about its position in hierarchy

	std::string familyWord, parentName;
	familyWord = lookupNextWord(filestr);
	if (familyWord == "child")
		filestr >> familyWord >> parentName;	///skip familyword and load parentname
	else if (familyWord == "root")
		filestr >> familyWord;	///skip familyword
	else
		familyWord = "attachToRoot";

	///////////////////////////////4. Load its tranformations

	if (isNextWordEqual(filestr, "transform"))
		EngineDataLoader::LoadTransform(filestr, comp->GetTransform());

	///////////////////////////////5.  After the component has been created, we can take care of its hierarchy stuff (it was loaded in 2.)

	/*if (familyWord == "child")
	{
		Component* parent = myActor->GetRoot()->SearchForComponent(parentName);
		if (parent)
			parent->AddComponent(comp);
		else
			std::cerr << "ERROR! Can't find target parent " << parentName << "!\n";
	}
	else if (familyWord == "root")
		myActor->ReplaceRoot(comp);
	else if (familyWord == "attachToRoot")
		myActor->AddComponent(comp);
		*/

	//////Check for an error

	std::string lastWord;
	filestr >> lastWord;
	if (lastWord != "end")
		std::cerr << "ERROR: There is no ''end'' after component's " << name << " definition! Detected word: " << lastWord << ".\n";
}

std::string Component::GetName() const
{
	return Name;
}
Transform& Component::GetTransform()
{
	return ComponentTransform;
}
const Transform& Component::GetTransform() const
{
	return ComponentTransform;
}
std::vector<Component*> Component::GetChildren()
{
	return Children;
}
void Component::SetName(std::string name)
{
	Name = name;
}
void Component::SetTransform(Transform transform)
{ 
	//don't change the ParentTransform pointer; it's not needed in this case
	ComponentTransform.SetPosition(transform.PositionRef);
	ComponentTransform.SetRotation(transform.RotationRef);
	ComponentTransform.SetScale(transform.ScaleRef);
	ComponentTransform.SetFront(transform.FrontRef);
	ComponentTransform.bConstrain = transform.bConstrain;
}
void Component::AddComponent(Component* component)
{
	component->GetTransform().SetParentTransform(&this->ComponentTransform);
	Children.push_back(component);
}
void Component::AddComponents(std::vector<Component*> components)
{
	for (unsigned int i = 0; i < components.size(); i++)
	{
		components[i]->GetTransform().SetParentTransform(&this->ComponentTransform);
		Children.push_back(components[i]);
	}
}
void Component::HandleInputsAll(GLFWwindow* window, float deltaTime)
{
	HandleInputs(window, deltaTime);
	for (unsigned int i = 0; i < Children.size(); i++)
		Children[i]->HandleInputsAll(window, deltaTime);
}
void Component::UpdateAll(float dt)
{
	Update(dt);
	for (unsigned int i = 0; i < Children.size(); i++)
		Children[i]->UpdateAll(dt);
}
Component* Component::SearchForComponent(std::string name)
{
	if (Name == name)
		return this;

	for (unsigned int i = 0; i < Children.size(); i++)
	{
		Component* childSearch = Children[i]->SearchForComponent(name);
		if (childSearch)
			return childSearch;
	}
	return nullptr;
}

Component::~Component()
{
	ComponentTransform.SetParentTransform(nullptr);
	for (unsigned int i = 0; i < Children.size(); i++)
	{
		Children[i]->GetTransform().SetParentTransform(nullptr);
	}
}
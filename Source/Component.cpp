#include "Component.h"
#include "FileLoader.h"
#include "BoneComponent.h"
#include "MeshSystem.h"
#include "RenderInfo.h"
#include "Mesh.h"
#include "Texture.h"

Component::Component(GameScene* scene, const std::string& name, const Transform& t):
	Name(name), ComponentTransform(t), Scene(scene), GameHandle(scene->GetGameHandle()), CollisionObj(nullptr), DebugRenderMaterial(nullptr), DebugRenderLastFrameMVP(glm::mat4(1.0f))
{
}
Component::Component(GameScene* scene, const std::string& name, glm::vec3 pos, glm::vec3 rot, glm::vec3 scale):
	Name(name), ComponentTransform(pos, rot, scale), Scene(scene), GameHandle(scene->GetGameHandle()), CollisionObj(nullptr), DebugRenderMaterial(nullptr), DebugRenderLastFrameMVP(glm::mat4(1.0f))
{
}

void Component::OnStart()
{
	if (!DebugRenderMaterial)
		LoadDebugRenderMaterial(GameHandle, "GEE_Mat_Default_Debug_Component", "EditorAssets/ComponentDebug.png");

}

void Component::OnStartAll()
{
	OnStart();
	for (int i = 0; i < static_cast<int>(Children.size()); i++)
		Children[i]->OnStartAll();
}

/*void Component::Setup(std::stringstream& filestr, Actor* myActor)
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
		*./

	//////Check for an error

	std::string lastWord;
	filestr >> lastWord;
	if (lastWord != "end")
		std::cerr << "ERROR: There is no ''end'' after component's " << name << " definition! Detected word: " << lastWord << ".\n";
}*/

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

GameScene* Component::GetScene() const
{
	return Scene;
}

void Component::GenerateFromNode(const MeshSystem::TemplateNode* node, Material* overrideMaterial)
{
	ComponentTransform *= node->GetTemplateTransform();

	SetCollisionObject(node->InstantiateCollisionObj());
}

void Component::SetName(std::string name)
{
	Name = name;
}
void Component::SetTransform(Transform transform)
{ 
	//Note that operator= doesn't change the ParentTransform pointer; it's not needed in this case
	ComponentTransform = transform;
}


CollisionObject* Component::SetCollisionObject(std::unique_ptr<CollisionObject>& obj)
{
	if (!obj)
		return nullptr;
	obj.swap(CollisionObj);
	CollisionObj->TransformPtr = &ComponentTransform;
	//Scene->GetPhysicsData()->AddCollisionObject(CollisionObj.get());
	GameHandle->GetPhysicsHandle()->AddCollisionObject(Scene->GetPhysicsData(), CollisionObj.get());

	return CollisionObj.get();
}

void Component::AddComponent(Component* component)
{
	component->GetTransform().SetParentTransform(&this->ComponentTransform);
	Children.push_back(component);
}
void Component::AddComponents(std::vector<Component*> components)
{
	for (int i = 0; i < static_cast<int>(components.size()); i++)
	{
		components[i]->GetTransform().SetParentTransform(&this->ComponentTransform);
		Children.push_back(components[i]);
	}
}
void Component::Update(float deltaTime)
{
	ComponentTransform.Update(deltaTime);
}
void Component::UpdateAll(float dt)
{
	Update(dt);
	for (int i = 0; i < static_cast<int>(Children.size()); i++)
		Children[i]->UpdateAll(dt);
}
void Component::QueueAnimation(Animation* animation)
{
	for (int i = 0; i < static_cast<int>(animation->Channels.size()); i++)
	{
		if (animation->Channels[i]->Name != Name)
			continue;

		AnimChannel& channel = *animation->Channels[i];

		for (int j = 0; j < static_cast<int>(channel.PosKeys.size() - 1); j++)
			ComponentTransform.AddInterpolator<glm::vec3>("position", (float)channel.PosKeys[j]->Time, (float)channel.PosKeys[j + 1]->Time - (float)channel.PosKeys[j]->Time, channel.PosKeys[j]->Value, channel.PosKeys[j + 1]->Value);

		for (int j = 0; j < static_cast<int>(channel.RotKeys.size() - 1); j++)
			ComponentTransform.AddInterpolator<glm::quat>("rotation", (float)channel.RotKeys[j]->Time, (float)channel.RotKeys[j + 1]->Time - (float)channel.RotKeys[j]->Time, channel.RotKeys[j]->Value, channel.RotKeys[j + 1]->Value);

		for (int j = 0; j < static_cast<int>(channel.ScaleKeys.size() - 1); j++)
			ComponentTransform.AddInterpolator<glm::vec3>("scale", (float)channel.ScaleKeys[j]->Time, (float)channel.ScaleKeys[j + 1]->Time - (float)channel.ScaleKeys[j]->Time, channel.ScaleKeys[j]->Value, channel.ScaleKeys[j + 1]->Value);
	}
}
void Component::QueueAnimationAll(Animation* animation)
{
	QueueAnimation(animation);
	for (int i = 0; i < static_cast<int>(Children.size()); i++)
		Children[i]->QueueAnimationAll(animation);
}

void Component::DebugRender(RenderInfo info, Shader* shader) const
{
	Transform transform = GetTransform().GetWorldTransform();
	transform.SetScale(glm::vec3(0.1f));
	GameHandle->GetRenderEngineHandle()->RenderStaticMesh(info, GameHandle->GetRenderEngineHandle()->GetBasicShapeMesh(EngineBasicShape::QUAD), transform, shader, &DebugRenderLastFrameMVP, DebugRenderMaterial, true);
}

void Component::DebugRenderAll(RenderInfo info, Shader* shader) const
{
	DebugRender(info, shader);

	for (int i = 0; i < static_cast<int>(Children.size()); i++)
		Children[i]->DebugRenderAll(info, shader);
}

void Component::LoadDebugRenderMaterial(GameManager* gameHandle, std::string materialName, std::string path)
{
	if (DebugRenderMaterial = gameHandle->GetRenderEngineHandle()->FindMaterial(materialName))
		return;

	DebugRenderMaterial = new Material(materialName);
	NamedTexture* texture = new NamedTexture(textureFromFile(path, GL_RGBA));

	texture->Bind();
	texture->SetShaderName("albedo1");
	DebugRenderMaterial->AddTexture(texture);
	DebugRenderMaterial->SetRenderShaderName("Forward_NoLight");
	gameHandle->GetRenderEngineHandle()->AddMaterial(DebugRenderMaterial);
}

Component* Component::SearchForComponent(std::string name)
{
	if (Name == name)
		return this;

	for (int i = 0; i < static_cast<int>(Children.size()); i++)
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
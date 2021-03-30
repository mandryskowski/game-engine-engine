#include <scene/Component.h>
#include <assetload/FileLoader.h>
#include <scene/BoneComponent.h>
#include <rendering/MeshSystem.h>
#include <rendering/RenderInfo.h>
#include <rendering/Mesh.h>
#include <rendering/Texture.h>
#include <UI/UICanvasActor.h>
#include <scene/UIInputBoxActor.h>

Component::Component(GameScene& scene, const std::string& name, const Transform& t):
	Name(name), ComponentTransform(t), Scene(scene), GameHandle(scene.GetGameHandle()), CollisionObj(nullptr), DebugRenderMat(nullptr), DebugRenderMatInst(nullptr), DebugRenderLastFrameMVP(glm::mat4(1.0f))
{
}
Component::Component(GameScene& scene, const std::string& name, glm::vec3 pos, glm::vec3 rot, glm::vec3 scale):
	Component(scene, name, Transform(pos, rot, scale))
{
}

Component::Component(Component&& comp) :
	Name(comp.Name),
	ComponentTransform(comp.ComponentTransform),
	Children(std::move(comp.Children)),
	CollisionObj(std::move(comp.CollisionObj)),
	Scene(comp.Scene),
	GameHandle(comp.GameHandle),
	DebugRenderMat(comp.DebugRenderMat),
	DebugRenderMatInst(comp.DebugRenderMatInst),
	DebugRenderLastFrameMVP(comp.DebugRenderLastFrameMVP)
{
}

Component& Component::operator=(Component&& comp)
{
	Name = comp.Name;
	ComponentTransform = comp.ComponentTransform;
	Children = std::move(comp.Children);
	CollisionObj = std::move(comp.CollisionObj);
	DebugRenderMat = comp.DebugRenderMat;
	DebugRenderMatInst = comp.DebugRenderMatInst;
	DebugRenderLastFrameMVP = DebugRenderLastFrameMVP;
	return *this;
}

Component& Component::operator=(const Component& compT)
{
	Name = compT.Name;
	ComponentTransform *= compT.ComponentTransform;
	CollisionObj = std::make_unique<CollisionObject>(CollisionObject(*compT.CollisionObj));
	DebugRenderMat = compT.DebugRenderMat;
	DebugRenderMatInst = compT.DebugRenderMatInst;
	DebugRenderLastFrameMVP = DebugRenderLastFrameMVP;

	return *this;
}

void Component::OnStart()
{
	if (!DebugRenderMatInst)
		DebugRenderMatInst = std::make_shared<MaterialInstance>(GetDebugMatInst(EditorIconState::IDLE));
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

	///////////////////////////////3. Load its tranformations

	if (isNextWordEqual(filestr, "transform"))
		EngineDataLoader::LoadTransform(filestr, comp->GetTransform());

	///////////////////////////////4.  After the component has been created, we can take care of its hierarchy stuff (it was loaded in 2.)

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

const std::string& Component::GetName() const
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
	std::vector<Component*> children;
	children.reserve(Children.size());
	std::transform(Children.begin(), Children.end(), std::back_inserter(children), [](std::unique_ptr<Component>& child) -> Component* { return child.get(); });
	return children;
}

GameScene& Component::GetScene() const
{
	return Scene;
}

std::unique_ptr<Component> Component::DetachChild(Component& soughtChild)
{
	for (auto& it = Children.begin(); it != Children.end(); it++)
		if (it->get() == &soughtChild)
		{
			std::unique_ptr<Component> temp = std::move(*it);
			Children.erase(it);
			return std::move(temp);
		}

	for (auto& it : Children)
		if (std::unique_ptr<Component> found = it->DetachChild(soughtChild))
			return std::move(found);

	return nullptr;
}

void Component::MoveChildren(Component& comp)
{
	std::for_each(Children.begin(), Children.end(), [&comp](std::unique_ptr<Component>& child) { comp.AddComponent(std::move(child)); });
	Children.clear();
}

void Component::GenerateFromNode(const MeshSystem::TemplateNode& node, Material* overrideMaterial)
{
	ComponentTransform *= node.GetTemplateTransform();

	SetCollisionObject(node.InstantiateCollisionObj());
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
	Scene.GetPhysicsData()->AddCollisionObject(*CollisionObj, ComponentTransform);



	return CollisionObj.get();
}

void Component::AddComponent(std::unique_ptr<Component> component)
{
	component->GetTransform().SetParentTransform(&this->ComponentTransform);
	Children.push_back(std::move(component));
}

void Component::AddComponents(std::vector<std::unique_ptr<Component>> components)
{
	std::transform(components.begin(), components.end(), std::back_inserter(Children), [](std::unique_ptr<Component>& comp) { return std::move(comp); });
	components.clear();
	std::for_each(Children.begin(), Children.end(), [this](std::unique_ptr<Component>& comp) { comp->GetTransform().SetParentTransform(&this->ComponentTransform); });
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

		AnimationChannel& channel = *animation->Channels[i];

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

void Component::QueueKeyFrame(AnimationChannel& channel)
{
	for (int j = 0; j < static_cast<int>(channel.PosKeys.size() - 1); j++)
		ComponentTransform.AddInterpolator<glm::vec3>("position", (float)channel.PosKeys[j]->Time, (float)channel.PosKeys[j + 1]->Time - (float)channel.PosKeys[j]->Time, channel.PosKeys[j]->Value, channel.PosKeys[j + 1]->Value);

	for (int j = 0; j < static_cast<int>(channel.RotKeys.size() - 1); j++)
		ComponentTransform.AddInterpolator<glm::quat>("rotation", (float)channel.RotKeys[j]->Time, (float)channel.RotKeys[j + 1]->Time - (float)channel.RotKeys[j]->Time, channel.RotKeys[j]->Value, channel.RotKeys[j + 1]->Value);

	for (int j = 0; j < static_cast<int>(channel.ScaleKeys.size() - 1); j++)
		ComponentTransform.AddInterpolator<glm::vec3>("scale", (float)channel.ScaleKeys[j]->Time, (float)channel.ScaleKeys[j + 1]->Time - (float)channel.ScaleKeys[j]->Time, channel.ScaleKeys[j]->Value, channel.ScaleKeys[j + 1]->Value);
}

void Component::QueueKeyFrameAll(AnimationChannel&)
{
}

void Component::DebugRender(RenderInfo info, Shader* shader) const
{
	if (!DebugRenderMatInst)
		return;
	Transform transform = GetTransform().GetWorldTransform();
	transform.SetScale(glm::vec3(0.05f));
	GameHandle->GetRenderEngineHandle()->RenderStaticMesh(info, MeshInstance(GameHandle->GetRenderEngineHandle()->GetBasicShapeMesh(EngineBasicShape::QUAD), DebugRenderMatInst), transform, shader, &DebugRenderLastFrameMVP, nullptr, true);
}

void Component::DebugRenderAll(RenderInfo info, Shader* shader) const
{
	DebugRender(info, shader);

	for (int i = 0; i < static_cast<int>(Children.size()); i++)
		Children[i]->DebugRenderAll(info, shader);
}

AtlasMaterial& Component::LoadDebugRenderMaterial(const std::string& materialName, const std::string& path)
{
	if (DebugRenderMat)
		return *DebugRenderMat;

	if (DebugRenderMat = dynamic_cast<AtlasMaterial*>(GameHandle->GetRenderEngineHandle()->FindMaterial(materialName)))
		return *DebugRenderMat;

	DebugRenderMat = new AtlasMaterial(materialName, glm::ivec2(3, 1));
	NamedTexture* texture = new NamedTexture(textureFromFile(path, GL_RGBA, GL_LINEAR, GL_LINEAR_MIPMAP_LINEAR, true));

	texture->Bind();
	texture->SetShaderName("albedo1");
	DebugRenderMat->AddTexture(texture);
	DebugRenderMat->SetRenderShaderName("Forward_NoLight");
	GameHandle->GetRenderEngineHandle()->AddMaterial(DebugRenderMat);

	return *DebugRenderMat;
}

MaterialInstance Component::GetDebugMatInst(EditorIconState state)
{
	DebugRenderMat = &LoadDebugRenderMaterial("GEE_Mat_Default_Debug_Component", "EditorAssets/component_icon.png");
	switch (state)
	{
	case EditorIconState::IDLE:
	default:
		return MaterialInstance(*DebugRenderMat, DebugRenderMat->GetTextureIDInterpolatorTemplate(0.0f));
	case EditorIconState::HOVER:
	case EditorIconState::BEING_CLICKED_OUTSIDE:
		return MaterialInstance(*DebugRenderMat, DebugRenderMat->GetTextureIDInterpolatorTemplate(1.0f));
	case EditorIconState::BEING_CLICKED_INSIDE:
		return MaterialInstance(*DebugRenderMat, DebugRenderMat->GetTextureIDInterpolatorTemplate(2.0f));
	}
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

#include <UI/UICanvasField.h>
#include <UI/UICanvasActor.h>
void Component::GetEditorDescription(UIActor& editorParent, GameScene& editorScene)
{
	UIInputBoxActor& textActor = editorParent.CreateChild(UIInputBoxActor(editorScene, "ComponentsNameActor"));
	textActor.SetOnInputFunc([this](const std::string& content) { SetName(content); }, [this]() -> std::string { return GetName(); });
	textActor.DeleteButtonModel();
	textActor.SetTransform(Transform(glm::vec2(0.0f, 0.75f), glm::vec2(0.25f)));
	//TextComponent& textComp = textActor.CreateComponent(TextComponent(editorScene, "ComponentsNameActorTextComp", Transform(glm::vec2(0.0f, 0.75f), glm::vec3(0.0f), glm::vec2(0.25f)), comp->GetName(), "fonts/expressway rg.ttf", std::pair<TextAlignment, TextAlignment>(TextAlignment::CENTER, TextAlignment::CENTER)));

	/*for (int x = 0, y = 0; !(y == 2 && x == 3); x++)
	{
		if ((y == 0 && x == 3) || (y == 1 && x == 4))
		{
			x = 0;
			y++;
		}
		UIInputBoxActor& inputBoxActor = editorParent.CreateChild(UIInputBoxActor(editorScene, "TransformBox" + std::to_string(x) + ":" + std::to_string(y)));
		inputBoxActor.SetTransform(Transform(glm::vec3(-0.75f + float(x) * 0.505f, 0.25f - float(y) * 0.505f, 0.0f), glm::vec3(0.0f), glm::vec3(0.25f)));

		switch (y)
		{
		case 0: inputBoxActor.SetOnInputFunc([this, x](float val) {glm::vec3 pos = GetTransform().PositionRef; pos[x] = val; GetTransform().SetPosition(pos); }, [this, x]() { return GetTransform().PositionRef[x]; }, true); break;
		case 1: inputBoxActor.SetOnInputFunc([this, x](float val) {glm::quat rot = GetTransform().RotationRef; rot[x] = val; GetTransform().SetRotation(glm::normalize(rot)); }, [this, x]() { return GetTransform().RotationRef[x]; }, true); break;
		case 2: inputBoxActor.SetOnInputFunc([this, x](float val) {glm::vec3 scale = GetTransform().ScaleRef; scale[x] = val; GetTransform().SetScale(scale); }, [this, x]() { return GetTransform().ScaleRef[x]; }, true); break;
		}
	}*/

	AddFieldToCanvas("Position", editorParent).GetTemplates().VecInput<glm::vec3>([this](float x, float val) {glm::vec3 pos = GetTransform().PositionRef; pos[x] = val; GetTransform().SetPosition(pos); }, [this](float x) { return GetTransform().PositionRef[x]; });
	AddFieldToCanvas("Rotation", editorParent).GetTemplates().VecInput<glm::quat>([this](float x, float val) {glm::quat rot = GetTransform().RotationRef; rot[x] = val; GetTransform().SetRotation(glm::normalize(rot)); }, [this](float x) { return GetTransform().RotationRef[x]; });
	AddFieldToCanvas("Scale", editorParent).GetTemplates().VecInput<glm::vec3>([this](float x, float val) {glm::vec3 scale = GetTransform().ScaleRef; scale[x] = val; GetTransform().SetScale(scale); }, [this](float x) { return GetTransform().ScaleRef[x]; });

	AddFieldToCanvas("Delete", editorParent).CreateChild(UIButtonActor(editorScene, "DeleteButton", "Delete", [this]() {}));
}

Component::~Component()
{
	//std::cout << "Erasing component " << Name << " " << this << ".\n";
	if (ComponentTransform.GetParentTransform())
		ComponentTransform.GetParentTransform()->RemoveChild(&ComponentTransform);
	std::for_each(Children.begin(), Children.end(), [](std::unique_ptr<Component>& comp) {comp->GetTransform().SetParentTransform(nullptr); });
	Children.clear();
}
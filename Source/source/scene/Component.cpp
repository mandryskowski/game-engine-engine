#include <scene/Component.h>
#include <assetload/FileLoader.h>
#include <scene/BoneComponent.h>
#include <scene/HierarchyTemplate.h>
#include <rendering/RenderInfo.h>
#include <rendering/Mesh.h>
#include <rendering/Texture.h>
#include <UI/UICanvasActor.h>
#include <scene/UIInputBoxActor.h>

Actor* cereal::LoadAndConstruct<Component>::ActorRef = nullptr;
Component* cereal::LoadAndConstruct<Component>::ParentComp = nullptr;

Component::Component(Actor& actor, Component* parentComp, const std::string& name, const Transform& t):
	Name(name), ComponentTransform(t), Scene(actor.GetScene()), ActorRef(actor), ParentComponent(parentComp), GameHandle(actor.GetScene().GetGameHandle()), CollisionObj(nullptr), DebugRenderMat(nullptr), DebugRenderMatInst(nullptr), DebugRenderLastFrameMVP(glm::mat4(1.0f)), bKillingProcessStarted(false)
{
}

Component::Component(Component&& comp) :
	Name(comp.Name),
	ComponentTransform(comp.ComponentTransform),
	Children(std::move(comp.Children)),
	CollisionObj(std::move(comp.CollisionObj)),
	Scene(comp.Scene),
	ActorRef(comp.ActorRef),
	ParentComponent(comp.ParentComponent),
	GameHandle(comp.GameHandle),
	DebugRenderMat(comp.DebugRenderMat),
	DebugRenderMatInst(comp.DebugRenderMatInst),
	DebugRenderLastFrameMVP(comp.DebugRenderLastFrameMVP),
	bKillingProcessStarted(comp.bKillingProcessStarted)
{
	std::cout << "Komponentowy move...\n";
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
	CollisionObj = (compT.CollisionObj) ? (std::make_unique<CollisionObject>(*compT.CollisionObj)) : (nullptr);
	if (CollisionObj)
		CollisionObj->TransformPtr = &ComponentTransform;
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

Actor& Component::GetActor() const
{
	return ActorRef;
}

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

CollisionObject* Component::GetCollisionObj() const
{
	return CollisionObj.get();
}

bool Component::IsBeingKilled() const
{
	return bKillingProcessStarted;
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

void Component::Delete()
{
	if (ParentComponent)
		ParentComponent->Children.erase(std::remove_if(ParentComponent->Children.begin(), ParentComponent->Children.end(), [this](std::unique_ptr<Component>& child) { return child.get() == this; }), ParentComponent->Children.end());
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

	if (IsBeingKilled())
	{
		Delete();
		return;
	}
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

void CollisionObjRendering(RenderInfo& info, GameManager& gameHandle, CollisionObject& obj, const Transform& t, const glm::vec3& color)
{
	RenderEngineManager& renderEngHandle = *gameHandle.GetRenderEngineHandle();
	Shader* shader = renderEngHandle.FindShader("Forward_NoLight");
	shader->Use();
	Material material("CollisionShapeDebugMaterial\n");
	material.SetColor(color);
	if (!shader)
		return;

	for (auto& shape : obj.Shapes)
	{
		EngineBasicShape geomShape;
		switch (shape->Type)
		{
			case CollisionShapeType::COLLISION_BOX:	default: geomShape = EngineBasicShape::CUBE; break;
			case CollisionShapeType::COLLISION_SPHERE: geomShape = EngineBasicShape::SPHERE; break;
			case CollisionShapeType::COLLISION_CAPSULE:	continue;
			case CollisionShapeType::COLLISION_TRIANGLE_MESH: continue;
		}

		const Transform& shapeTransform = shape->ShapeTransform;

		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		renderEngHandle.RenderStaticMesh(info, MeshInstance(renderEngHandle.GetBasicShapeMesh(geomShape), &material), t * shapeTransform, shader);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}
}

#include <input/InputDevicesStateRetriever.h>
void Component::DebugRender(RenderInfo info, Shader* shader) const
{
	if (GameHandle->GetInputRetriever().IsKeyPressed(Key::P))
		return;
	if (!DebugRenderMatInst)
		return;

	if (CollisionObj)
		CollisionObjRendering(info, *GameHandle, *CollisionObj, GetTransform().GetWorldTransform());

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

std::shared_ptr<AtlasMaterial> Component::LoadDebugRenderMaterial(const std::string& materialName, const std::string& path)
{
	if (DebugRenderMat)
		return DebugRenderMat;

	if (DebugRenderMat = std::dynamic_pointer_cast<AtlasMaterial>(GameHandle->GetRenderEngineHandle()->FindMaterial(materialName)))
		return DebugRenderMat;

	DebugRenderMat = std::make_shared<AtlasMaterial>(materialName, glm::ivec2(3, 1));
	std::shared_ptr<NamedTexture> texture = std::make_shared<NamedTexture>(textureFromFile(path, GL_RGBA, GL_LINEAR, GL_LINEAR_MIPMAP_LINEAR, true));

	texture->Bind();
	texture->SetShaderName("albedo1");
	DebugRenderMat->AddTexture(texture);
	DebugRenderMat->SetRenderShaderName("Forward_NoLight");
	GameHandle->GetRenderEngineHandle()->AddMaterial(DebugRenderMat);

	return DebugRenderMat;
}

MaterialInstance Component::GetDebugMatInst(EditorIconState state)
{
	DebugRenderMat = LoadDebugRenderMaterial("GEE_Mat_Default_Debug_Component", "EditorAssets/component_icon.png");
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
#include <UI/UIListActor.h>
#include <scene/TextComponent.h>
void Component::GetEditorDescription(EditorDescriptionBuilder descBuilder)
{
	UIInputBoxActor& textActor = descBuilder.CreateActor<UIInputBoxActor>("ComponentsNameActor");
	textActor.SetTransform(Transform(glm::vec2(0.0f, 1.5f), glm::vec2(1.0f)));

	ModelComponent& prettyQuad = textActor.CreateComponent<ModelComponent>("PrettyQuad");
	prettyQuad.AddMeshInst(MeshInstance(GameHandle->GetRenderEngineHandle()->GetBasicShapeMesh(EngineBasicShape::QUAD), const_cast<Material*>(textActor.GetButtonModel()->GetMeshInstance(0).GetMaterialPtr())));
	prettyQuad.SetTransform(Transform(glm::vec2(0.0f, -0.625f), glm::vec2(textActor.GetRoot()->GetComponent<TextComponent>("ComponentsNameActorText")->GetTextLength() * 4.0f, 0.025f)));	//Component name text has scale (0.5, 0.5), so pos (0, -0.625) equals to 125% text half extent.
	textActor.SetOnInputFunc([this, &prettyQuad, &textActor](const std::string& content) { SetName(content); prettyQuad.SetTransform(Transform(glm::vec2(0.0f, -0.625f), glm::vec2(textActor.GetRoot()->GetComponent<TextComponent>("ComponentsNameActorText")->GetTextLength(), 0.025f))); }, [this]() -> std::string { return GetName(); });

	textActor.DeleteButtonModel();
	//TextComponent& textComp = textActor.NowyCreate<TextComponent>("ComponentsNameActorTextComp", Transform(glm::vec2(0.0f, 0.75f), glm::vec3(0.0f), glm::vec2(0.25f)), comp->GetName(), "fonts/expressway rg.ttf", std::pair<TextAlignment, TextAlignment>(TextAlignment::CENTER, TextAlignment::CENTER));

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

	descBuilder.AddField("Position").GetTemplates().VecInput<glm::vec3>([this](float x, float val) {glm::vec3 pos = GetTransform().PositionRef; pos[x] = val; GetTransform().SetPosition(pos); }, [this](float x) { return GetTransform().PositionRef[x]; });
	descBuilder.AddField("Rotation").GetTemplates().VecInput<glm::quat>([this](float x, float val) {glm::quat rot = GetTransform().RotationRef; rot[x] = val; GetTransform().SetRotation(glm::normalize(rot)); }, [this](float x) { return GetTransform().RotationRef[x]; });
	descBuilder.AddField("Scale").GetTemplates().VecInput<glm::vec3>([this](float x, float val) {glm::vec3 scale = GetTransform().ScaleRef; scale[x] = val; GetTransform().SetScale(scale); }, [this](float x) { return GetTransform().ScaleRef[x]; });

	descBuilder.AddField("Delete").CreateChild<UIButtonActor>("DeleteButton", "Delete", [this, descBuilder]() mutable 
		{
			MarkAsKilled();

			if (!ParentComponent)	//If this is the root component, refresh the scene (to remove the killed actor)
				descBuilder.RefreshScene();

			descBuilder.SelectActor(&ActorRef); //Refresh the actor (to remove the killed component). Component will be deselected automatically - SelectComponent(nullptr) is always called inside SelectActor(...).
		}).GetTransform()->Move(glm::vec2(1.0f, 0.0f));

	UICanvasField& shapesListField = descBuilder.AddField("Collision object");
	shapesListField.GetTransform()->Move(glm::vec2(1.0f, 0.0f));
	UIAutomaticListActor& shapesList = shapesListField.CreateChild<UIAutomaticListActor>("ShapesList");
	dynamic_cast<UICanvasActor*>(shapesListField.GetCanvasPtr())->FieldsList->SetListElementOffset(dynamic_cast<UICanvasActor*>(shapesListField.GetCanvasPtr())->FieldsList->GetListElementCount() - 1, [&shapesListField, &shapesList]()-> glm::vec3 { return static_cast<glm::mat3>(shapesListField.GetTransform()->GetMatrix()) * (shapesList.GetListOffset());});
	dynamic_cast<UICanvasActor*>(shapesListField.GetCanvasPtr())->FieldsList->SetListCenterOffset(dynamic_cast<UICanvasActor*>(shapesListField.GetCanvasPtr())->FieldsList->GetListElementCount() - 1, [&shapesListField]()-> glm::vec3 { return glm::vec3(0.0f, -shapesListField.GetTransform()->ScaleRef.y, 0.0f); });
	UIButtonActor& colObjectPresenceButton = shapesList.CreateChild<UIButtonActor>("CollisionObjectButton", (CollisionObj) ? ("V") : ("X"));
	colObjectPresenceButton.GetTransform()->Move(Vec2f(1.0f, 0.0f));
		
	shapesListField.CreateChild<UIButtonActor>("AddColShape", "+", [this, &shapesList, &colObjectPresenceButton]() { if (!CollisionObj) { this->SetCollisionObject(std::make_unique<CollisionObject>(CollisionObject(true, CollisionShapeType::COLLISION_BOX))); }  /*CollisionObj->AddShape(CollisionShapeType::COLLISION_BOX); */shapesList.Refresh();}).SetTransform(Transform(glm::vec2(3.0f, 0.0f), glm::vec2(0.25f)));


	//shapesList.GetTransform()->Move(glm::vec2(0.0f, -0.2f));
	if (CollisionObj)
	{
		std::string shapeTypeName;
		for (auto& it : CollisionObj->Shapes)
		{
			switch (it->Type)
			{
				case CollisionShapeType::COLLISION_BOX: shapeTypeName = "Box"; break;
				case CollisionShapeType::COLLISION_SPHERE: shapeTypeName = "Sphere"; break;
				case CollisionShapeType::COLLISION_TRIANGLE_MESH: shapeTypeName = "Triangle Mesh"; break;
				case CollisionShapeType::COLLISION_CAPSULE: shapeTypeName = "Capsule"; break;
			}
			Actor& shapeActor = shapesList.CreateChild<Actor>("TextActor");
			shapeActor.CreateComponent<TextComponent>("Text", Transform(), ">" + shapeTypeName + "<", "fonts/expressway rg.ttf", std::pair<TextAlignment, TextAlignment>(TextAlignment::CENTER, TextAlignment::CENTER));
		
		}
	}
	//shapesList.GetTransform()->Move(-1.0f * static_cast<glm::mat3>(shapesListField.GetTransform()->GetMatrix()) * (shapesList.GetListBegin() + shapesList.GetListOffset()));
	shapesList.Refresh();
}

void Component::MarkAsKilled()
{
	bKillingProcessStarted = true;
	if (!ParentComponent)	//If this is the root component of an Actor, kill the Actor as well.
		ActorRef.MarkAsKilled();
	for (auto& it : Children)
		it->MarkAsKilled();
}

Component::~Component()
{
	//std::cout << "Erasing component " << Name << " " << this << ".\n";
	if (ComponentTransform.GetParentTransform())
		ComponentTransform.GetParentTransform()->RemoveChild(&ComponentTransform);
	std::for_each(Children.begin(), Children.end(), [](std::unique_ptr<Component>& comp) {comp->GetTransform().SetParentTransform(nullptr); });
	Children.clear();
}
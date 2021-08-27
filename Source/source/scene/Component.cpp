#include <scene/Component.h>
#include <assetload/FileLoader.h>
#include <scene/ModelComponent.h>
#include <physics/CollisionObject.h>
#include <rendering/RenderInfo.h>
#include <rendering/Mesh.h>
#include <rendering/Texture.h>
#include <UI/UICanvasActor.h>
#include <scene/UIInputBoxActor.h>
#include <scene/hierarchy/HierarchyTree.h>
#include <UI/UICanvasField.h>
#include <scene/UIWindowActor.h>
#include <UI/UIListActor.h>

#include <input/InputDevicesStateRetriever.h>

GEE::Actor* GEE::CerealComponentSerializationData::ActorRef = nullptr;
GEE::Component* GEE::CerealComponentSerializationData::ParentComp = nullptr;
namespace GEE
{
	Component::Component(Actor& actor, Component* parentComp, const std::string& name, const Transform& t) :
		Name(name), ComponentTransform(t), Scene(actor.GetScene()), ActorRef(actor), ParentComponent(parentComp), GameHandle(actor.GetScene().GetGameHandle()), CollisionObj(nullptr), DebugRenderMat(nullptr), DebugRenderMatInst(nullptr), DebugRenderLastFrameMVP(Mat4f(1.0f)), bKillingProcessStarted(false)
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
		CollisionObj = (compT.CollisionObj) ? (MakeUnique<Physics::CollisionObject>(*compT.CollisionObj)) : (nullptr);
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
			DebugRenderMatInst = MakeShared<MaterialInstance>(LoadDebugMatInst(EditorIconState::IDLE));
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
		std::transform(Children.begin(), Children.end(), std::back_inserter(children), [](UniquePtr<Component>& child) -> Component* { return child.get(); });
		return children;
	}

	GameScene& Component::GetScene() const
	{
		return Scene;
	}

	Physics::CollisionObject* Component::GetCollisionObj() const
	{
		return CollisionObj.get();
	}

	MaterialInstance* Component::GetDebugMatInst()
	{
		return DebugRenderMatInst.get();
	}

	bool Component::IsBeingKilled() const
	{
		return bKillingProcessStarted;
	}

	UniquePtr<Component> Component::DetachChild(Component& soughtChild)
	{
		for (auto& it = Children.begin(); it != Children.end(); it++)
			if (it->get() == &soughtChild)
			{
				UniquePtr<Component> temp = std::move(*it);
				Children.erase(it);
				return std::move(temp);
			}

		for (auto& it : Children)
			if (UniquePtr<Component> found = it->DetachChild(soughtChild))
				return std::move(found);

		return nullptr;
	}

	void Component::MoveChildren(Component& comp)
	{
		std::for_each(Children.begin(), Children.end(), [&comp](UniquePtr<Component>& child) { comp.AddComponent(std::move(child)); });
		Children.clear();
	}

	void Component::Delete()
	{
		if (ParentComponent)
			ParentComponent->Children.erase(std::remove_if(ParentComponent->Children.begin(), ParentComponent->Children.end(), [this](UniquePtr<Component>& child) { return child.get() == this; }), ParentComponent->Children.end());
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


	Physics::CollisionObject* Component::SetCollisionObject(UniquePtr<Physics::CollisionObject> obj)
	{
		CollisionObj = std::move(obj);
		if (CollisionObj)
			Scene.GetPhysicsData()->AddCollisionObject(*CollisionObj, ComponentTransform);

		return CollisionObj.get();
	}

	Component& Component::AddComponent(UniquePtr<Component> component)
	{
		component->GetTransform().SetParentTransform(&this->ComponentTransform);
		Children.push_back(std::move(component));
		return *Children.back();
	}

	void Component::AddComponents(std::vector<UniquePtr<Component>> components)
	{
		std::transform(components.begin(), components.end(), std::back_inserter(Children), [](UniquePtr<Component>& comp) { return std::move(comp); });
		components.clear();
		std::for_each(Children.begin(), Children.end(), [this](UniquePtr<Component>& comp) { comp->GetTransform().SetParentTransform(&this->ComponentTransform); });
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
				ComponentTransform.AddInterpolator<Vec3f>("position", channel.PosKeys[j]->Time, channel.PosKeys[j + 1]->Time - channel.PosKeys[j]->Time, channel.PosKeys[j]->Value, channel.PosKeys[j + 1]->Value);

			for (int j = 0; j < static_cast<int>(channel.RotKeys.size() - 1); j++)
				ComponentTransform.AddInterpolator<Quatf>("rotation", channel.RotKeys[j]->Time, channel.RotKeys[j + 1]->Time - channel.RotKeys[j]->Time, channel.RotKeys[j]->Value, channel.RotKeys[j + 1]->Value);

			for (int j = 0; j < static_cast<int>(channel.ScaleKeys.size() - 1); j++)
				ComponentTransform.AddInterpolator<Vec3f>("scale", channel.ScaleKeys[j]->Time, channel.ScaleKeys[j + 1]->Time - channel.ScaleKeys[j]->Time, channel.ScaleKeys[j]->Value, channel.ScaleKeys[j + 1]->Value);
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
			ComponentTransform.AddInterpolator<Vec3f>("position", channel.PosKeys[j]->Time, channel.PosKeys[j + 1]->Time - channel.PosKeys[j]->Time, channel.PosKeys[j]->Value, channel.PosKeys[j + 1]->Value);

		for (int j = 0; j < static_cast<int>(channel.RotKeys.size() - 1); j++)
			ComponentTransform.AddInterpolator<Quatf>("rotation", channel.RotKeys[j]->Time, channel.RotKeys[j + 1]->Time - channel.RotKeys[j]->Time, channel.RotKeys[j]->Value, channel.RotKeys[j + 1]->Value);

		for (int j = 0; j < static_cast<int>(channel.ScaleKeys.size() - 1); j++)
			ComponentTransform.AddInterpolator<Vec3f>("scale", channel.ScaleKeys[j]->Time, channel.ScaleKeys[j + 1]->Time - channel.ScaleKeys[j]->Time, channel.ScaleKeys[j]->Value, channel.ScaleKeys[j + 1]->Value);
	}

	void Component::QueueKeyFrameAll(AnimationChannel&)
	{
	}

	void CollisionObjRendering(RenderInfo& info, GameManager& gameHandle, Physics::CollisionObject& obj, const Transform& t, const Vec3f& color)
	{
		if (!obj.ActorPtr)
			return;
		RenderEngineManager& renderEngHandle = *gameHandle.GetRenderEngineHandle();
		Shader* shader = renderEngHandle.FindShader("Forward_NoLight");
		shader->Use();
		Material material("CollisionShapeDebugMaterial\n");

		physx::PxRigidDynamic* rigidDynamicCast = obj.ActorPtr->is<physx::PxRigidDynamic>();
		bool sleeping = (rigidDynamicCast && rigidDynamicCast->isSleeping());
		material.SetColor((sleeping) ? (Vec3f(1.0) - color) : (color));
		if (!shader)
			return;

		for (auto& shape : obj.Shapes)
		{
			Mesh* mesh = nullptr;
			switch (shape->Type)
			{
			case Physics::CollisionShapeType::COLLISION_BOX:	default: mesh = &renderEngHandle.GetBasicShapeMesh(EngineBasicShape::CUBE); break;
			case Physics::CollisionShapeType::COLLISION_SPHERE: mesh = &renderEngHandle.GetBasicShapeMesh(EngineBasicShape::SPHERE); break;
			case Physics::CollisionShapeType::COLLISION_CAPSULE:	continue;
			case Physics::CollisionShapeType::COLLISION_TRIANGLE_MESH: if (auto loc = shape->GetOptionalLocalization()) { HierarchyTemplate::HierarchyTreeT* tree = gameHandle.FindHierarchyTree(loc->GetTreeName()); mesh = tree->FindMesh(loc->ShapeMeshLoc.NodeName, loc->ShapeMeshLoc.SpecificName); if (!mesh) mesh = &loc->OptionalCorrespondingMesh; } break;
			}

			if (!mesh)
				continue;

			const Transform& shapeTransform = shape->ShapeTransform;

			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			//std::cout << "Debug-rendering col shape mesh " << mesh->GetLocalization().NodeName << " " << mesh->GetLocalization().SpecificName << '\n';
			//printVector(color, "Color");
			renderEngHandle.RenderStaticMesh(info, MeshInstance(*mesh, &material), t * shapeTransform, shader);
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		}
	}

	void Component::DebugRender(RenderInfo info, Shader* shader, const Vec3f& debugIconScale) const
	{
		if (GameHandle->GetInputRetriever().IsKeyPressed(Key::F2))
			return;
		if (!DebugRenderMatInst)
			return;

		if (CollisionObj)
			;// CollisionObjRendering(info, *GameHandle, *CollisionObj, GetTransform().GetWorldTransform());

		Transform transform = GetTransform().GetWorldTransform();
		transform.SetScale(debugIconScale);
		GameHandle->GetRenderEngineHandle()->RenderStaticMesh(info, MeshInstance(GameHandle->GetRenderEngineHandle()->GetBasicShapeMesh(EngineBasicShape::QUAD), DebugRenderMatInst), transform, shader, &DebugRenderLastFrameMVP, nullptr, true);
	}

	void Component::DebugRenderAll(RenderInfo info, Shader* shader) const
	{
		DebugRender(info, shader);

		for (int i = 0; i < static_cast<int>(Children.size()); i++)
			Children[i]->DebugRenderAll(info, shader);
	}

	SharedPtr<AtlasMaterial> Component::LoadDebugRenderMaterial(const std::string& materialName, const std::string& path)
	{
		if (DebugRenderMat)
			return DebugRenderMat;

		if (DebugRenderMat = std::dynamic_pointer_cast<AtlasMaterial>(GameHandle->GetRenderEngineHandle()->FindMaterial(materialName)))
			return DebugRenderMat;

		DebugRenderMat = MakeShared<AtlasMaterial>(materialName, glm::ivec2(3, 1));
		SharedPtr<NamedTexture> texture = MakeShared<NamedTexture>(Texture::Loader<>::FromFile2D(path, Texture::Format::RGBA(), true));

		texture->Bind();
		texture->SetShaderName("albedo1");
		DebugRenderMat->AddTexture(texture);
		DebugRenderMat->SetRenderShaderName("Forward_NoLight");
		GameHandle->GetRenderEngineHandle()->AddMaterial(DebugRenderMat);

		return DebugRenderMat;
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

	void Component::GetEditorDescription(EditorDescriptionBuilder descBuilder)
	{
		UIInputBoxActor& textActor = descBuilder.CreateActor<UIInputBoxActor>("ComponentsNameActor");
		textActor.SetTransform(Transform(Vec2f(0.0f, 1.5f), Vec2f(1.0f)));
		textActor.GetButtonModel()->SetHide(true);

		ModelComponent& prettyQuad = textActor.CreateComponent<ModelComponent>("PrettyQuad");
		prettyQuad.AddMeshInst(MeshInstance(GameHandle->GetRenderEngineHandle()->GetBasicShapeMesh(EngineBasicShape::QUAD), const_cast<Material*>(textActor.GetButtonModel()->GetMeshInstance(0).GetMaterialPtr())));
		prettyQuad.SetTransform(Transform(Vec2f(0.0f, -0.21f), Vec2f(1.0f, 0.01f)));
		textActor.SetOnInputFunc([this, &prettyQuad, &textActor](const std::string& content) { SetName(content); /*prettyQuad.SetTransform(Transform(Vec2f(0.0f, -0.625f), Vec2f(textActor.GetRoot()->GetComponent<TextComponent>("ComponentsNameActorText")->GetTextLength(), 0.025f)));*/ }, [this]() -> std::string { return GetName(); });

		//textActor.DeleteButtonModel();

		auto& transformCategory = descBuilder.AddCategory("Transform");
		ComponentTransform.GetEditorDescription(EditorDescriptionBuilder(descBuilder.GetEditorHandle(), transformCategory));

		UIButtonActor& deleteButton = descBuilder.AddField("Delete").CreateChild<UIButtonActor>("DeleteButton", "Delete", [this, descBuilder]() mutable
			{
				MarkAsKilled();

				if (!ParentComponent)	//If this is the root component, refresh the scene (to remove the killed actor)
					descBuilder.RefreshScene();

				descBuilder.SelectActor(&ActorRef); //Refresh the actor (to remove the killed component). Component will be deselected automatically - SelectComponent(nullptr) is always called inside SelectActor(...).
			});

		if (&ActorRef == Scene.GetRootActor() && !ParentComponent)	//Disallow deleting the root of a scene
			deleteButton.SetDisableInput(true);


		// Collision object category
		auto& shapesListCategory = descBuilder.AddCategory("Collision object");
		UIButtonActor& colObjectPresenceButton = shapesListCategory.CreateChild<UIButtonActor>("CollisionObjectButton", (CollisionObj) ? ("V") : ("X"));

		// Is static button
		shapesListCategory.AddField("Is static").GetTemplates().TickBox([this](bool makeStatic) {
			if (!CollisionObj) return;
			CollisionObj->ScenePhysicsData->EraseCollisionObject(*CollisionObj);
			CollisionObj->IsStatic = makeStatic;
			CollisionObj->ScenePhysicsData->AddCollisionObject(*CollisionObj, GetTransform());
			return;
		},
		[this]() -> bool {
			if (CollisionObj)
				return CollisionObj->ActorPtr->is<physx::PxRigidDynamic>() == nullptr;
			return false; });

		// Collision shapes list
		auto& shapesListField = shapesListCategory.AddField("List of shapes");
		UIAutomaticListActor& shapesList = shapesListField.CreateChild<UIAutomaticListActor>("ShapesList");

		shapesListCategory.SetListElementOffset(shapesListCategory.GetListElementCount() - 1,
			[descBuilder, &shapesListField, &shapesList, &shapesListCategory, defaultOffset = shapesListCategory.GetListElement(shapesListCategory.GetListElementCount() - 1).GetElementOffset()]() mutable -> Vec3f { return static_cast<Mat3f>(descBuilder.GetCanvas().ToCanvasSpace(shapesListField.GetTransform()->GetWorldTransform()).GetMatrix()) * (2.0f * shapesList.GetListOffset()); });

		shapesListField.CreateChild<UIButtonActor>("AddColShape", "+", [this, descBuilder]() mutable {
			UIWindowActor& shapeCreatorWindow = descBuilder.GetEditorScene().CreateActorAtRoot<UIWindowActor>("ShapeCreatorWindow");
			shapeCreatorWindow.GetTransform()->SetScale(Vec2f(0.25f));
			UIAutomaticListActor& buttonsList = shapeCreatorWindow.CreateChild<UIAutomaticListActor>("ButtonsList");

			std::function <void(SharedPtr<Physics::CollisionShape>)> addShapeFunc = [this, descBuilder, &shapeCreatorWindow](SharedPtr<Physics::CollisionShape> shape) mutable {
				if (!CollisionObj)
				{
					UniquePtr<Physics::CollisionObject> colObj = MakeUnique<Physics::CollisionObject>(true);
					colObj->AddShape(shape);
					this->SetCollisionObject(std::move(colObj));
				}
				else
					CollisionObj->AddShape(shape);

				shapeCreatorWindow.MarkAsKilled();
				descBuilder.SelectComponent(this);	//refresh

			};

			for (int i = Physics::CollisionShapeType::COLLISION_FIRST; i <= Physics::CollisionShapeType::COLLISION_LAST; i++)
			{
				Physics::CollisionShapeType shapeType = static_cast<Physics::CollisionShapeType>(i);
				UIButtonActor& shapeAddButton = buttonsList.CreateChild<UIButtonActor>("ShapeAddButton" + std::to_string(i), Physics::Util::collisionShapeTypeToString(shapeType));

				if (shapeType == Physics::CollisionShapeType::COLLISION_TRIANGLE_MESH)
					shapeAddButton.SetOnClickFunc([this, addShapeFunc, &shapeCreatorWindow]() {
					UIWindowActor& pathInputWindow = shapeCreatorWindow.CreateChild<UIWindowActor>("PathInputWindow");
					pathInputWindow.GetTransform()->SetScale(Vec2f(0.5f));


					UIElementTemplates(pathInputWindow).HierarchyTreeInput(*GameHandle, [this, &pathInputWindow, addShapeFunc](HierarchyTemplate::HierarchyTreeT& tree) {
						pathInputWindow.MarkAsKilled();

						SharedPtr<Physics::CollisionShape> shape = EngineDataLoader::LoadTriangleMeshCollisionShape(GameHandle->GetPhysicsHandle(), tree.GetMeshes()[0]);
						addShapeFunc(shape);
						});

					/*UIInputBoxActor& treeNameInputBox = pathInputWindow.CreateChild<UIInputBoxActor>("TreeNameInputBox");
					treeNameInputBox.SetOnInputFunc([this, &pathInputWindow, addShapeFunc](const std::string& path) {
							HierarchyTemplate::HierarchyTreeT* tree = GameHandle->FindHierarchyTree(path);
							if (!tree)
								return;

							pathInputWindow.MarkAsKilled();

							SharedPtr<CollisionShape> shape = EngineDataLoader::LoadTriangleMeshCollisionShape(GameHandle->GetPhysicsHandle(), tree->GetMeshes()[0]);
							addShapeFunc(shape);
						}, []() { return "Enter tree path"; });*/
						});
				else
					shapeAddButton.SetOnClickFunc([shapeType, addShapeFunc]() { addShapeFunc(MakeShared<Physics::CollisionShape>(shapeType)); });
			}
			buttonsList.Refresh();
		}).SetTransform(Transform(Vec2f(4.0f, 0.0f), Vec2f(0.5f)));

		shapesListField.CreateChild<UIButtonActor>("RemoveColObject", "-", [&]() { SetCollisionObject(nullptr); }, Transform(Vec2f(5.0f, 0.0f), Vec2f(0.5f)));


		if (CollisionObj)
			for (auto& it : CollisionObj->Shapes)
			{
				std::string shapeTypeName = Physics::Util::collisionShapeTypeToString(it->Type);
				UIButtonActor& shapeActor = shapesList.CreateChild<UIButtonActor>("ShapeSelectButton", shapeTypeName, [this, descBuilder, it]() mutable {
					UIWindowActor& shapeTransformWindow = descBuilder.GetEditorScene().CreateActorAtRoot<UIWindowActor>("ShapeTransformWindow");
					shapeTransformWindow.GetTransform()->SetScale(Vec2f(0.25f));

					EditorDescriptionBuilder shapeDescBuilder(descBuilder.GetEditorHandle(), *shapeTransformWindow.GetScaleActor());
					it->ShapeTransform.GetEditorDescription(shapeDescBuilder);
					shapeDescBuilder.AddField("Delete").CreateChild<UIButtonActor>("DeleteButton", "Delete", [this, it, descBuilder, &shapeTransformWindow]() mutable { CollisionObj->DetachShape(*it); shapeTransformWindow.MarkAsKilled(); descBuilder.RefreshComponent(); });

					shapeTransformWindow.RefreshFieldsList();
					shapeTransformWindow.AutoClampView();
					});
				shapeActor.GetButtonModel()->SetHide(true);

			}

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

	MaterialInstance Component::LoadDebugMatInst(EditorIconState state)
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

	Component::~Component()
	{
		//std::cout << "Erasing component " << Name << " " << this << ".\n";
		if (ComponentTransform.GetParentTransform())
			ComponentTransform.GetParentTransform()->RemoveChild(&ComponentTransform);
		std::for_each(Children.begin(), Children.end(), [](UniquePtr<Component>& comp) {comp->GetTransform().SetParentTransform(nullptr); });
		Children.clear();
	}
}
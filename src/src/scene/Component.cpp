#include <scene/Component.h>
#include <game/GameScene.h>
#include <game/IDSystem.h>
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
#include <scene/hierarchy/HierarchyNode.h>

#include <rendering/Renderer.h>

#include <physics/DynamicPhysicsObjects.h>

#include <input/InputDevicesStateRetriever.h>

GEE::Actor* GEE::CerealComponentSerializationData::ActorRef = nullptr;
GEE::Component* GEE::CerealComponentSerializationData::ParentComp = nullptr;
namespace GEE
{
	Component::Component(Actor& actor, Component* parentComp, const std::string& name, const Transform& t) :
		Name(name),
		ComponentGEEID(IDSystem<Component>::GenerateID()),
		ComponentTransform(t),
		Scene(actor.GetScene()),
		ActorRef(actor),
		ParentComponent(parentComp),
		GameHandle(actor.GetScene().GetGameHandle()),
		CollisionObj(nullptr),
		DebugRenderMat(nullptr),
		DebugRenderMatInst(nullptr),
		DebugRenderLastFrameMVP(Mat4f(1.0f)),
		KillingProcessFrame(0)
	{
	}

	Component::Component(Component&& comp) :
		Name(comp.Name),
		ComponentGEEID(comp.ComponentGEEID),
		ComponentTransform(comp.ComponentTransform),
		Children(std::move(comp.Children)),
		CollisionObj(std::move(comp.CollisionObj)),
		Scene(comp.Scene),
		ActorRef(comp.ActorRef),
		ParentComponent(comp.ParentComponent),
		GameHandle(comp.GameHandle),
		DebugRenderMat(comp.DebugRenderMat),
		DebugRenderLastFrameMVP(comp.DebugRenderLastFrameMVP),
		KillingProcessFrame(comp.KillingProcessFrame)
	{
		std::cout << "Komponentowy move...\n";
	}

	Component& Component::operator=(Component&& comp)
	{
		Name = comp.Name;
		ComponentGEEID = comp.ComponentGEEID;
		ComponentTransform = comp.ComponentTransform;
		Children = std::move(comp.Children);
		CollisionObj = std::move(comp.CollisionObj);
		DebugRenderMat = comp.DebugRenderMat;
		DebugRenderLastFrameMVP = DebugRenderLastFrameMVP;
		return *this;
	}

	Component& Component::operator=(const Component& compT)
	{
		Name = compT.Name;
		ComponentGEEID = IDSystem<Component>::GenerateID();
		ComponentTransform *= compT.ComponentTransform;
		SetCollisionObject((compT.CollisionObj) ? (MakeUnique<Physics::CollisionObject>(*compT.CollisionObj)) : (nullptr));
		DebugRenderMat = compT.DebugRenderMat;
		DebugRenderLastFrameMVP = DebugRenderLastFrameMVP;

		return *this;
	}

	void Component::OnStart()
	{
		if (!DebugRenderMatInst)
			DebugRenderMatInst = MakeShared<MaterialInstance>(GetDebugMatInst(ButtonMaterialType::Idle));
	}

	void Component::OnStartAll()
	{
		OnStart();
		for (int i = 0; i < static_cast<int>(Children.size()); i++)
			Children[i]->OnStartAll();
	}

	void Component::DebugHierarchy(int nrTabs)
	{
		std::string tabs;
		for (int i = 0; i < nrTabs; i++)	tabs += "	";
		std::cout << tabs + "-" + Name << " " << this << '\n';

		for (int i = 0; i < static_cast<int>(Children.size()); i++)
			Children[i]->DebugHierarchy(nrTabs + 1);
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
		return KillingProcessFrame != 0;
	}

	UniquePtr<Component> Component::DetachChild(Component& soughtChild)
	{
		for (auto it = Children.begin(); it != Children.end(); it++)
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
		while (!Children.empty())
			Children.front()->Delete();

		if (ParentComponent)
			ParentComponent->Children.erase(std::remove_if(ParentComponent->Children.begin(), ParentComponent->Children.end(), [this](UniquePtr<Component>& child) { return child.get() == this; }), ParentComponent->Children.end());
	}

	void Component::SetName(const String& name)
	{
		Name = name;
	}
	void Component::SetTransform(const Transform& transform)
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
		component->ParentComponent = this;
		Children.push_back(std::move(component));

		if (Scene.HasStarted())
			Children.back()->OnStartAll();

		return *Children.back();
	}

	void Component::AddComponents(std::vector<UniquePtr<Component>> components)
	{
		std::transform(components.begin(), components.end(), std::back_inserter(Children), [](UniquePtr<Component>& comp) { return std::move(comp); });
		components.clear();
		std::for_each(Children.begin(), Children.end(), [this](UniquePtr<Component>& comp) { comp->GetTransform().SetParentTransform(&this->ComponentTransform); });
	}

	void Component::Update(Time dt)
	{
		ComponentTransform.Update(dt);
	}

	void Component::UpdateAll(Time dt)
	{
		Update(dt);
		for (int i = 0; i < static_cast<int>(Children.size()); i++)
			Children[i]->UpdateAll(dt);

		if (IsBeingKilled() && GameHandle->GetTotalFrameCount() != KillingProcessFrame)
		{
			Delete();
			return;
		}
	}

	void Component::HandleEventAll(const Event& ev)
	{
		HandleEvent(ev);
		for (int i = 0; i < static_cast<int>(Children.size()); i++)
			Children[i]->HandleEventAll(ev);
	}

	void Component::QueueAnimation(Animation* animation)
	{
		for (int i = 0; i < static_cast<int>(animation->Channels.size()); i++)
		{
			if (animation->Channels[i]->Name != Name)
				continue;

			AnimationChannel& channel = *animation->Channels[i];

			for (int j = 0; j < static_cast<int>(channel.PosKeys.size() - 1); j++)
				ComponentTransform.AddInterpolator<Vec3f>("position", channel.PosKeys[j]->KeyTime, channel.PosKeys[j + 1]->KeyTime - channel.PosKeys[j]->KeyTime, channel.PosKeys[j]->Value, channel.PosKeys[j + 1]->Value);

			for (int j = 0; j < static_cast<int>(channel.RotKeys.size() - 1); j++)
				ComponentTransform.AddInterpolator<Quatf>("rotation", channel.RotKeys[j]->KeyTime, channel.RotKeys[j + 1]->KeyTime - channel.RotKeys[j]->KeyTime, channel.RotKeys[j]->Value, channel.RotKeys[j + 1]->Value);

			for (int j = 0; j < static_cast<int>(channel.ScaleKeys.size() - 1); j++)
				ComponentTransform.AddInterpolator<Vec3f>("scale", channel.ScaleKeys[j]->KeyTime, channel.ScaleKeys[j + 1]->KeyTime - channel.ScaleKeys[j]->KeyTime, channel.ScaleKeys[j]->Value, channel.ScaleKeys[j + 1]->Value);
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
			ComponentTransform.AddInterpolator<Vec3f>("position", channel.PosKeys[j]->KeyTime, channel.PosKeys[j + 1]->KeyTime - channel.PosKeys[j]->KeyTime, channel.PosKeys[j]->Value, channel.PosKeys[j + 1]->Value);

		for (int j = 0; j < static_cast<int>(channel.RotKeys.size() - 1); j++)
			ComponentTransform.AddInterpolator<Quatf>("rotation", channel.RotKeys[j]->KeyTime, channel.RotKeys[j + 1]->KeyTime - channel.RotKeys[j]->KeyTime, channel.RotKeys[j]->Value, channel.RotKeys[j + 1]->Value);

		for (int j = 0; j < static_cast<int>(channel.ScaleKeys.size() - 1); j++)
			ComponentTransform.AddInterpolator<Vec3f>("scale", channel.ScaleKeys[j]->KeyTime, channel.ScaleKeys[j + 1]->KeyTime - channel.ScaleKeys[j]->KeyTime, channel.ScaleKeys[j]->Value, channel.ScaleKeys[j + 1]->Value);
	}

	void Component::QueueKeyFrameAll(AnimationChannel&)
	{
	}

	void CollisionObjRendering(const SceneMatrixInfo& info, GameManager& gameHandle, Physics::CollisionObject& obj, const Transform& t, const Vec3f& color)
	{
		if (!obj.ActorPtr)
			return;
		RenderEngineManager& renderEngHandle = *gameHandle.GetRenderEngineHandle();
		Shader* shader = renderEngHandle.GetSimpleShader();
		shader->Use();
		auto material = MakeShared<Material>("CollisionShapeDebugMaterial");

		physx::PxRigidDynamic* rigidDynamicCast = obj.ActorPtr->is<physx::PxRigidDynamic>();
		bool sleeping = (rigidDynamicCast && rigidDynamicCast->isSleeping());
		material->SetColor((sleeping) ? (Vec3f(1.0) - color) : (color));
		if (!shader)
			return;

		for (auto& shape : obj.Shapes)
		{
			Mesh* mesh = nullptr;
			switch (shape->Type)
			{
			case Physics::CollisionShapeType::COLLISION_BOX:	default: mesh = &renderEngHandle.GetBasicShapeMesh(EngineBasicShape::Cube); break;
			case Physics::CollisionShapeType::COLLISION_SPHERE: mesh = &renderEngHandle.GetBasicShapeMesh(EngineBasicShape::Sphere); break;
			case Physics::CollisionShapeType::COLLISION_CAPSULE:	continue;
			case Physics::CollisionShapeType::COLLISION_TRIANGLE_MESH: if (auto loc = shape->GetOptionalLocalization()) { Hierarchy::Tree* tree = gameHandle.FindHierarchyTree(loc->GetTreeName()); mesh = tree->FindMesh(loc->ShapeMeshLoc.NodeName, loc->ShapeMeshLoc.SpecificName); if (!mesh) mesh = &loc->OptionalCorrespondingMesh; } break;
			}

			if (!mesh)
				continue;

			const Transform& shapeTransform = shape->ShapeTransform;

			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			//std::cout << "Debug-rendering col shape mesh " << mesh->GetLocalization().NodeName << " " << mesh->GetLocalization().SpecificName << '\n';
			//printVector(color, "Color");
			 
			Renderer(*gameHandle.GetRenderEngineHandle()).StaticMeshInstances(info, { MeshInstance(*mesh, material) }, t * shapeTransform, *shader);
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		}
	}

	void Component::DebugRender(SceneMatrixInfo info, Shader& shader, const Vec3f& debugIconScale) const
	{
		if (GameHandle->GetDefInputRetriever().IsKeyPressed(Key::F2))
			return;
		if (!DebugRenderMatInst)
			return;

		//if (CollisionObj)
			// CollisionObjRendering(info, *GameHandle, *CollisionObj, GetTransform().GetWorldTransform());

		Transform transform = GetTransform().GetWorldTransform();
		transform.SetScale(debugIconScale);
		
		Renderer(*GameHandle->GetRenderEngineHandle(), 0).StaticMeshInstances(info, { MeshInstance(GameHandle->GetRenderEngineHandle()->GetBasicShapeMesh(EngineBasicShape::Quad), DebugRenderMatInst) }, transform, shader, true);
	}

	void Component::DebugRenderAll(SceneMatrixInfo info, Shader& shader) const
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

	void Component::GetEditorDescription(ComponentDescriptionBuilder descBuilder)
	{
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
		Physics::CollisionObject* colObj = (descBuilder.IsNodeBeingBuilt()) ? (descBuilder.GetNodeBeingBuilt()->GetCollisionObject()) : (GetCollisionObj());
		UIButtonActor& colObjectPresenceButton = shapesListCategory.AddField("Present").CreateChild<UIButtonActor>("CollisionObjectButton", (colObj) ? ("V") : ("X"));

		colObjectPresenceButton.CreateChild<UIButtonActor>("AddColShape", "+", [this, colObj, descBuilder]() mutable {
			UIWindowActor& shapeCreatorWindow = descBuilder.GetEditorScene().CreateActorAtRoot<UIWindowActor>("ShapeCreatorWindow");
			shapeCreatorWindow.GetTransform()->SetScale(Vec2f(0.25f));
			UIAutomaticListActor& buttonsList = shapeCreatorWindow.CreateChild<UIAutomaticListActor>("ButtonsList");

			std::function<void(SharedPtr<Physics::CollisionShape>)> addShapeFunc = [this, colObj, descBuilder, &shapeCreatorWindow](SharedPtr<Physics::CollisionShape> shape) mutable {
				if (!colObj)
				{
					if (descBuilder.IsNodeBeingBuilt())
						colObj = descBuilder.GetNodeBeingBuilt()->SetCollisionObject(MakeUnique<Physics::CollisionObject>(true));
					else
						colObj = SetCollisionObject(MakeUnique<Physics::CollisionObject>(true));
				}
				
				colObj->AddShape(shape);

				shapeCreatorWindow.MarkAsKilled();
				descBuilder.Refresh();
			};

			for (int i = Physics::CollisionShapeType::COLLISION_FIRST; i <= Physics::CollisionShapeType::COLLISION_LAST; i++)
			{
				Physics::CollisionShapeType shapeType = static_cast<Physics::CollisionShapeType>(i);
				UIButtonActor& shapeAddButton = buttonsList.CreateChild<UIButtonActor>("ShapeAddButton" + std::to_string(i), Physics::Util::collisionShapeTypeToString(shapeType));

				if (shapeType == Physics::CollisionShapeType::COLLISION_TRIANGLE_MESH)
					shapeAddButton.SetOnClickFunc([this, addShapeFunc, &shapeCreatorWindow]() {
					UIWindowActor& pathInputWindow = shapeCreatorWindow.CreateChild<UIWindowActor>("PathInputWindow");
					pathInputWindow.GetTransform()->SetScale(Vec2f(0.5f));


					UIElementTemplates(pathInputWindow).HierarchyTreeInput(*GameHandle, [this, &pathInputWindow, addShapeFunc](Hierarchy::Tree& tree) {
						pathInputWindow.MarkAsKilled();

						SharedPtr<Physics::CollisionShape> shape = EngineDataLoader::LoadTriangleMeshCollisionShape(GameHandle->GetPhysicsHandle(), tree.GetMeshes()[0]);
						addShapeFunc(shape);
						});

					/*UIInputBoxActor& treeNameInputBox = pathInputWindow.CreateChild<UIInputBoxActor>("TreeNameInputBox");
					treeNameInputBox.SetOnInputFunc([this, &pathInputWindow, addShapeFunc](const std::string& path) {
							Hierarchy::Tree* tree = GameHandle->FindHierarchyTree(path);
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

		colObjectPresenceButton.CreateChild<UIButtonActor>("RemoveColObject", "-", [&]() { SetCollisionObject(nullptr); }, Transform(Vec2f(5.0f, 0.0f), Vec2f(0.5f)));

		// Is static button
		shapesListCategory.AddField("Is static").GetTemplates().TickBox([this, colObj, descBuilder](bool makeStatic) {
			if (!colObj) return;
			colObj->IsStatic = makeStatic;

			if (!descBuilder.IsNodeBeingBuilt())
			{
				colObj->ScenePhysicsData->EraseCollisionObject(*colObj);
				colObj->ScenePhysicsData->AddCollisionObject(*colObj, GetTransform());
			}
		},
		[colObj]() -> bool {
			if (colObj && colObj->ActorPtr)
				return colObj->ActorPtr->is<physx::PxRigidDynamic>() == nullptr;
			else if (colObj) return colObj->IsStatic;
			return false;
		});

		if (colObj && !colObj->IsStatic && colObj->ActorPtr)
		{
			auto& propertiesCategory = shapesListCategory.AddCategory("Physical properties");
			
			auto rigidDynamicPtr = colObj->ActorPtr->is<physx::PxRigidDynamic>();
			propertiesCategory.AddField("Linear damping").GetTemplates().SliderUnitInterval([colObj](float coeff) { Physics::SetLinearDamping(*colObj, coeff); }, rigidDynamicPtr->getLinearDamping());
			propertiesCategory.AddField("Angular damping").GetTemplates().SliderUnitInterval([colObj](float coeff) { Physics::SetAngularDamping(*colObj, coeff); }, rigidDynamicPtr->getAngularDamping());
		}


		// Collision shapes list
		auto& shapesListField = shapesListCategory.AddField("List of shapes");
		UIAutomaticListActor& shapesList = shapesListField.CreateChild<UIAutomaticListActor>("ShapesList");

		shapesListCategory.SetListElementOffset(shapesListCategory.GetListElementCount() - 1,
			[descBuilder, &shapesListField, &shapesList]() mutable -> Vec3f { return shapesList.GetListElementCount() == 0 ? Vec3f(0.0f, -2.0f, 0.0f) :  static_cast<Mat3f>(descBuilder.GetCanvas().ToCanvasSpace(shapesListField.GetTransform()->GetWorldTransform()).GetMatrix()) * shapesList.GetListOffset(); });



		if (colObj)
			for (auto& shape : colObj->Shapes)
			{
				std::string shapeTypeName = Physics::Util::collisionShapeTypeToString(shape->Type);
				UIButtonActor& shapeActor = shapesList.CreateChild<UIButtonActor>("ShapeSelectButton", shapeTypeName, [colObj , descBuilder, shape, shapeTypeName]() mutable {
					UIWindowActor& shapeWindow = descBuilder.GetEditorScene().CreateActorAtRoot<UIWindowActor>(shapeTypeName + "ShapeWindow");
					shapeWindow.GetTransform()->SetScale(Vec2f(0.25f));


					EditorDescriptionBuilder shapeDescBuilder(descBuilder.GetEditorHandle(), *shapeWindow.GetScaleActor());
					shapeDescBuilder.SetDeleteFunction([&shapeWindow]() { shapeWindow.MarkAsKilled(); });
					shape->GetEditorDescription(shapeDescBuilder, *colObj);

					shapeWindow.RefreshFieldsList();
					shapeWindow.AutoClampView();
					});
				shapeActor.GetButtonModel()->SetHide(true);

			}

		shapesList.Refresh();
	}

	void Component::MarkAsKilled()
	{
		KillingProcessFrame = GameHandle->GetTotalFrameCount();
		if (CollisionObj && CollisionObj->ActorPtr && Scene.GetPhysicsData())
			Scene.GetPhysicsData()->EraseCollisionObject(*CollisionObj);

		if (!ParentComponent)	//If this is the root component of an Actor, kill the Actor as well.
			ActorRef.MarkAsKilled();
		for (auto& it : Children)
			it->MarkAsKilled();
	}

	MaterialInstance Component::GetDebugMatInst(ButtonMaterialType type)
	{
		LoadDebugRenderMaterial("GEE_Mat_Default_Debug_Component", "Assets/Editor/component_icon.png");
		switch (type)
		{
		case ButtonMaterialType::Idle:
		default:
			return MaterialInstance(DebugRenderMat, DebugRenderMat->GetTextureIDInterpolatorTemplate(0.0f));
		case ButtonMaterialType::Hover:
			return MaterialInstance(DebugRenderMat, DebugRenderMat->GetTextureIDInterpolatorTemplate(1.0f));
		case ButtonMaterialType::BeingClicked:
			return MaterialInstance(DebugRenderMat, DebugRenderMat->GetTextureIDInterpolatorTemplate(2.0f));
		}
	}

	template<typename Archive>
	void Component::Save(Archive& archive) const
	{
		archive(CEREAL_NVP(Name), CEREAL_NVP(ComponentGEEID), CEREAL_NVP(ComponentTransform), CEREAL_NVP(CollisionObj));
		archive(CEREAL_NVP(Children));
	}
	template<typename Archive>
	void Component::Load(Archive& archive)
	{
		archive(CEREAL_NVP(Name), CEREAL_NVP(ComponentGEEID), CEREAL_NVP(ComponentTransform), CEREAL_NVP(CollisionObj));
		if (CollisionObj)
			Scene.GetPhysicsData()->AddCollisionObject(*CollisionObj, ComponentTransform);

		std::cout << "Serializing comp " << Name << '\n';
		if (Scene.HasStarted())
			OnStartAll();

		/*
		This code is not very elegant; it's because Components and Actors are not default_constructible.
		We always need to pass at least one reference to either the GameScene (in the case of Actors) or to the Actor (of the constructed Component).
		Furthermore, if an Actor/Component is not at the root of hierarchy (of a scene/an actor) we should always pass the pointer to the parent to the constructor, since users might wrongly assume that an Actor/Component is the root if the parent pointer passed to the constructor is nullptr.
		This might lead to bugs, so we avoid that.
		*/

		CerealComponentSerializationData::ParentComp = this;	//Set the static parent pointer to this
		archive(CEREAL_NVP(Children));	//We want all children to be constructed using the pointer. IMPORTANT: The first child will be serialized (the Load method will be called) before the next child is constructed! So the ParentComp pointer will be changed to the address of the new child when we construct its children. We counteract that in the next line.
		CerealComponentSerializationData::ParentComp = ParentComponent;	//There are no more children of this to serialize, so we "move up" the hierarchy and change the parent pointer to the parent of this, to allow brothers (other children of parent of this) to be constructed.

		for (auto& it : Children)
			it->GetTransform().SetParentTransform(&ComponentTransform);
	}

	Component::~Component()
	{
		//std::cout << "Erasing component " << Name << " " << this << ".\n";
		if (ComponentTransform.GetParentTransform())
			ComponentTransform.GetParentTransform()->RemoveChild(&ComponentTransform);
		std::for_each(Children.begin(), Children.end(), [](UniquePtr<Component>& comp) {comp->GetTransform().SetParentTransform(nullptr); });
		Children.clear();
	}

	template void Component::Save<cereal::JSONOutputArchive>(cereal::JSONOutputArchive&) const;
	template void Component::Load<cereal::JSONInputArchive>(cereal::JSONInputArchive&);
}
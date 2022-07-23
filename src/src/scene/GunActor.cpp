#include <scene/GunActor.h>
#include <physics/DynamicPhysicsObjects.h>
#include <game/GameScene.h>
#include <scene/SoundSourceComponent.h>
#include <assetload/FileLoader.h>
#include <input/Event.h>
#include <scene/PawnActor.h>

#include <UI/UICanvasActor.h>
#include <UI/UIActor.h>
#include <UI/UICanvasField.h>
#include <scene/UIButtonActor.h>

namespace GEE
{
	GunActor::GunActor(GameScene& scene, Actor* parentActor, std::string name) :
		Actor(scene, parentActor, name),
		ParticleMeshInst(nullptr),
		FireModel(nullptr),
		GunBlast(nullptr),
		FireCooldown(2.0f),
		CooldownLeft(0.0f),
		BulletRadius(0.2f),
		ImpulseFactor(0.25f)
	{
	}

	void GunActor::Setup()
	{
		if (!SetupStream)
		{
			std::cerr << "ERROR! Can't setup GunActor " << Name << " dla " << this << ".\n";
			return;
		}

		std::string str;
		(*SetupStream) >> str;

		if (str == "FireMaterial")
		{
			(*SetupStream) >> str;
			std::cout << "Firematerial = " << GameHandle->GetRenderEngineHandle()->FindMaterial(str) << ".\n";
		}

		SetFireModel(GetRoot()->GetComponent<ModelComponent>("FireParticle"));
		std::cout << "Wyszukalem se: " << FireModel << ".\n";

		GunBlast = Scene.GetAudioData()->FindSource("DoubleBarrelBlast");

		Actor::Setup();
	}

	void GunActor::Update(float deltaTime)
	{
		if (GunBlast && GunBlast->IsBeingKilled())
			GunBlast = nullptr;
		if (FireModel && FireModel->IsBeingKilled())
			SetFireModel(nullptr);

		GetRoot()->GetTransform().Update(deltaTime);	//update for recoil animation
		CooldownLeft -= deltaTime;
		Actor::Update(deltaTime);
	}

	void GunActor::HandleEvent(const Event& ev)
	{
		Actor::HandleEvent(ev);
	}

	void GunActor::SetFireModel(ModelComponent* model)
	{
		FireModel = model;
		if (!FireModel)
		{
			ParticleMeshInst = nullptr;
			return;
		}

		dynamic_cast<ModelComponent*>(FireModel->SearchForComponent("Quad"))->SetRenderAsBillboard(true);
		ParticleMeshInst = FireModel->FindMeshInstance("Quad");

		AtlasMaterial* fireMaterial = dynamic_cast<AtlasMaterial*>(&ParticleMeshInst->GetMaterialInst()->GetMaterialRef());
		if (!fireMaterial)
			return;

		ParticleMeshInst->GetMaterialInst()->SetInterp(&fireMaterial->GetTextureIDInterpolatorTemplate(Interpolation(0.0f, 0.25f, InterpolationType::Linear), 0.0f, dynamic_cast<AtlasMaterial*>(&ParticleMeshInst->GetMaterialInst()->GetMaterialRef())->GetMaxTextureID()));
		ParticleMeshInst->GetMaterialInst()->SetDrawBeforeAnim(false);
		ParticleMeshInst->GetMaterialInst()->SetDrawAfterAnim(false);
	}

	void GunActor::FireWeapon()
	{
		if (CooldownLeft > 0.0f)
			return;

		if (ParticleMeshInst)
			if (ParticleMeshInst->GetMaterialInst()->IsAnimated())
				ParticleMeshInst->GetMaterialInst()->ResetAnimation();
			else
				SetFireModel(FireModel);	//update fire model
		if (GunBlast)
			GunBlast->Play();

		std::vector<PawnActor*> pawnActors;
		Scene.GetRootActor()->GetAllActors<PawnActor>(&pawnActors);

		if (!pawnActors.empty())
			pawnActors[0]->InflictDamage(50.0f, Math::CutYAxis(GetTransform()->GetWorldTransform().GetPos() - pawnActors[0]->GetTransform()->GetWorldTransform().GetPos()));

		GetRoot()->GetTransform().AddInterpolator<Quatf>("rotation", 0.0f, 0.25f, Quatf(Vec3f(0.0f)), toQuat(Vec3f(30.0f, 0.0f, 0.0f)), InterpolationType::Quintic, true);
		GetRoot()->GetTransform().AddInterpolator<Quatf>("rotation", 0.25f, 1.25f, Quatf(Vec3f(0.0f)), InterpolationType::Quadratic, true);

		Actor& actor = Scene.CreateActorAtRoot<Actor>("Bullet");
		//TODO: Change it so the bullet is fired at the barrel, not at the center
		UniquePtr<ModelComponent> bulletModel = MakeUnique<ModelComponent>(ModelComponent(actor, nullptr, "BulletModel", Transform(GetTransform()->GetWorldTransform().GetPos(), Vec3f(0.0f), Vec3f(BulletRadius))));
		bulletModel->OnStart();
		SharedPtr<Material> rustedIronMaterial = GameHandle->GetRenderEngineHandle()->FindMaterial("RustedIron");
		if (!rustedIronMaterial)
		{
			rustedIronMaterial = GameHandle->GetRenderEngineHandle()->AddMaterial(MakeShared<Material>("RustedIron"));
			rustedIronMaterial->AddTexture(MakeShared<NamedTexture>(Texture::Loader<>::FromFile2D("Assets/External/Materials/rustediron/rustediron_albedo.png", Texture::Format::SRGB(), false, Texture::MinFilter::Trilinear(), Texture::MagFilter::Bilinear()), "albedo1"));
			rustedIronMaterial->AddTexture(MakeShared<NamedTexture>(Texture::Loader<>::FromFile2D("Assets/External/Materials/rustediron/rustediron_metallic.png"), "metallic1"));
			rustedIronMaterial->AddTexture(MakeShared<NamedTexture>(Texture::Loader<>::FromFile2D("Assets/External/Materials/rustediron/rustediron_roughness.png"), "roughness1"));
			rustedIronMaterial->AddTexture(MakeShared<NamedTexture>(Texture::Loader<>::FromFile2D("Assets/External/Materials/rustediron/rustediron_normal.png"), "normal1"));
		}

		{
			EngineDataLoader::LoadModel("Assets/External/hqSphere/hqSphere.obj", *bulletModel, MeshTreeInstancingType::ROOTTREE);
			std::vector<ModelComponent*> models;
			models.push_back(bulletModel.get());
			bulletModel->GetAllComponents<ModelComponent>(&models);
			for (auto& it : models)
				it->OverrideInstancesMaterial(rustedIronMaterial);
		}

		UniquePtr<Physics::CollisionObject> dupa = MakeUnique<Physics::CollisionObject>(false, Physics::CollisionShapeType::COLLISION_SPHERE);
		Physics::CollisionObject& col = *bulletModel->SetCollisionObject(std::move(dupa));	//dupa is no longer valid
		Physics::ApplyForce(col, GetRoot()->GetTransform().GetWorldTransform().GetFrontVec() * ImpulseFactor);
		col.ActorPtr->is<physx::PxRigidDynamic>()->setLinearDamping(0.5f);
		col.ActorPtr->is<physx::PxRigidDynamic>()->setAngularDamping(0.5f);

		actor.ReplaceRoot(std::move(bulletModel));

		actor.DebugHierarchy();

		CooldownLeft = FireCooldown;
	}
	void GunActor::GetEditorDescription(EditorDescriptionBuilder descBuilder)
	{
		Actor::GetEditorDescription(descBuilder);

		UICanvasField& blastField = descBuilder.AddField("Blast sound");
		blastField.GetTemplates().ObjectInput<Component, Audio::SoundSourceComponent>(*GetRoot(), GunBlast);

		descBuilder.AddField("Fire model").GetTemplates().ObjectInput<Component, ModelComponent>(*GetRoot(), [this](ModelComponent* model) { SetFireModel(model); });
		
		descBuilder.AddField("Bullet radius").CreateChild<UIInputBoxActor>("BulletRadiusInputBox", [&](float val) {BulletRadius = val; }, [&]() { return BulletRadius; });
		descBuilder.AddField("Impulse factor").CreateChild<UIInputBoxActor>("ImpulseFactorInputBox", [&](float val) {ImpulseFactor = val; }, [&]() { return ImpulseFactor; });

		UICanvasField& fireField = descBuilder.AddField("Fire");
		fireField.CreateChild<UIButtonActor>("FireButton", "Fire", [this]() { FireWeapon(); });

		//[this]() {}
	}
}
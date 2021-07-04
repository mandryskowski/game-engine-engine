#include <scene/GunActor.h>
#include <scene/SoundSourceComponent.h>
#include <assetload/FileLoader.h>
#include <input/Event.h>

#include <UI/UICanvasActor.h>
#include <UI/UIActor.h>
#include <UI/UICanvasField.h>
#include <scene/UIButtonActor.h>
#include <UI/UIListActor.h>

namespace GEE
{
	GunActor::GunActor(GameScene& scene, Actor* parentActor, std::string name) :
		Actor(scene, parentActor, name),
		ParticleMeshInst(nullptr),
		FireModel(nullptr),
		GunBlast(nullptr)
	{
		FireCooldown = 2.0f;
		CooldownLeft = 0.0f;
		FiredBullets = 0;
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


		/*
		GunModel->GetTransform()->AddInterpolator("position", 10.0f, 20.0f, Vec3f(0.0f, 0.0f, -10.0f), InterpolationType::LINEAR);
		GunModel->GetTransform()->AddInterpolator("rotation", 10.0f, 5.0f, Vec3f(360.0f, 0.0f, 0.0f), InterpolationType::QUADRATIC, false, AnimBehaviour::STOP, AnimBehaviour::EXTRAPOLATE);
		GunModel->GetTransform()->AddInterpolator("scale", 10.0f, 10.0f, Vec3f(0.3f, 0.3f, 0.2f), InterpolationType::CONSTANT);*/

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

		AtlasMaterial* fireMaterial = dynamic_cast<AtlasMaterial*>(const_cast<Material*>(&ParticleMeshInst->GetMaterialInst()->GetMaterialRef()));
		if (!fireMaterial)
			return;

		ParticleMeshInst->GetMaterialInst()->SetInterp(&fireMaterial->GetTextureIDInterpolatorTemplate(Interpolation(0.0f, 0.25f, InterpolationType::LINEAR), 0.0f, dynamic_cast<AtlasMaterial*>(&ParticleMeshInst->GetMaterialInst()->GetMaterialRef())->GetMaxTextureID()));
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

		GetRoot()->GetTransform().AddInterpolator<Quatf>("rotation", 0.0f, 0.25f, Quatf(Vec3f(0.0f)), toQuat(Vec3f(30.0f, 0.0f, 0.0f)), InterpolationType::QUINTIC, true);
		GetRoot()->GetTransform().AddInterpolator<Quatf>("rotation", 0.25f, 1.25f, Quatf(Vec3f(0.0f)), InterpolationType::QUADRATIC, true);

		Actor& actor = Scene.CreateActorAtRoot<Actor>("Bullet" + std::to_string(FiredBullets));
		//TODO: Change it so the bullet is fired at the barrel, not at the center
		std::unique_ptr<ModelComponent> bulletModel = std::make_unique<ModelComponent>(ModelComponent(actor, nullptr, "BulletModel" + std::to_string(FiredBullets++), Transform(GetTransform()->GetWorldTransform().Pos(), Vec3f(0.0f), Vec3f(0.2f))));
		bulletModel->OnStart();
		Material* rustedIronMaterial = GameHandle->GetRenderEngineHandle()->FindMaterial("RustedIron").get();
		if (!rustedIronMaterial)
		{
			rustedIronMaterial = GameHandle->GetRenderEngineHandle()->AddMaterial(std::make_shared<Material>("RustedIron"));
			rustedIronMaterial->AddTexture(std::make_shared<NamedTexture>(Texture::Loader<>::FromFile2D("EngineMaterials/rustediron_albedo.png", Texture::Format::SRGB(), false, Texture::MinFilter::Trilinear(), Texture::MagFilter::Bilinear()), "albedo1"));
			rustedIronMaterial->AddTexture(std::make_shared<NamedTexture>(Texture::Loader<>::FromFile2D("EngineMaterials/rustediron_metallic.png"), "metallic1"));
			rustedIronMaterial->AddTexture(std::make_shared<NamedTexture>(Texture::Loader<>::FromFile2D("EngineMaterials/rustediron_roughness.png"), "roughness1"));
			rustedIronMaterial->AddTexture(std::make_shared<NamedTexture>(Texture::Loader<>::FromFile2D("EngineMaterials/rustediron_normal.png"), "normal1"));
		}

		{
			EngineDataLoader::LoadModel("hqSphere/hqSphere.obj", *bulletModel, MeshTreeInstancingType::ROOTTREE);
			std::vector<ModelComponent*> models;
			models.push_back(bulletModel.get());
			bulletModel->GetAllComponents<ModelComponent>(&models);
			for (auto& it : models)
				it->OverrideInstancesMaterial(rustedIronMaterial);
		}

		std::unique_ptr<Physics::CollisionObject> dupa = std::make_unique<Physics::CollisionObject>(false, Physics::CollisionShapeType::COLLISION_SPHERE);
		Physics::CollisionObject& col = *bulletModel->SetCollisionObject(std::move(dupa));	//dupa is no longer valid
		GameHandle->GetPhysicsHandle()->ApplyForce(col, GetRoot()->GetTransform().GetWorldTransform().GetFrontVec() * 0.25f);
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
		blastField.GetTemplates().ComponentInput<Audio::SoundSourceComponent>(*GetRoot(), GunBlast);

		descBuilder.AddField("Fire model").GetTemplates().ComponentInput<ModelComponent>(*GetRoot(), [this](ModelComponent* model) { SetFireModel(model); });

		UICanvasField& fireField = descBuilder.AddField("Fire");
		fireField.CreateChild<UIButtonActor>("FireButton", "Fire", [this]() { FireWeapon(); });

		//[this]() {}
	}

}
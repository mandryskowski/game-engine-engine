#include <scene/GunActor.h>
#include <physics/DynamicPhysicsObjects.h>
#include <game/GameScene.h>
#include <scene/SoundSourceComponent.h>
#include <scene/LightComponent.h>
#include <assetload/FileLoader.h>
#include <input/Event.h>
#include <PhysX/PxQueryReport.h>
#include <scene/PawnActor.h>

#include <UI/UICanvasActor.h>
#include <UI/UIActor.h>
#include <UI/UICanvasField.h>
#include <scene/UIButtonActor.h>

#include "physics/CollisionObject.h"
#include "rendering/RenderEngineManager.h"

namespace GEE
{
	GunActor::GunActor(GameScene& scene, Actor* parentActor, std::string name) :
		Actor(scene, parentActor, name),
		ParticleMeshInst(nullptr),
		FireModel(nullptr),
		BlastSound(nullptr),
		GunLight(nullptr),
		FireCooldown(2.0),
		CooldownLeft(0.0),
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

		String str;
		(*SetupStream) >> str;

		if (str == "FireMaterial")
		{
			(*SetupStream) >> str;
			std::cout << "Firematerial = " << GameHandle->GetRenderEngineHandle()->FindMaterial(str) << ".\n";
		}

		SetFireModel(GetRoot()->GetComponent<ModelComponent>("FireParticle"));
		std::cout << "Wyszukalem se: " << FireModel << ".\n";

		BlastSound = Scene.GetAudioData()->FindSource("DoubleBarrelBlast");

		Actor::Setup();
	}

	void GunActor::Update(Time dt)
	{
		if (BlastSound && BlastSound->IsBeingKilled())
			BlastSound = nullptr;
		if (FireModel && FireModel->IsBeingKilled())
			SetFireModel(nullptr);

		GetRoot()->GetTransform().Update(dt);	//update for recoil animation
		CooldownLeft -= dt;
		Actor::Update(dt);
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
		if (BlastSound)
			BlastSound->Play();

		if (GunLight)
		{
			Interpolation interp(0.0f, 0.25f);
			interp.SetOnUpdateFunc([this](double T) mutable { GunLight->SetDiffuse(Vec3f(5.0f, 0.2f, 0.0f) * static_cast<float>(1.0 - T)); return T == 1.0; });
			GameHandle->AddInterpolation(interp);
		}

		physx::PxRaycastBuffer castBuffer;
		physx::PxU32 maxHits = 1;
		physx::PxHitFlags hitFlags = physx::PxHitFlag::ePOSITION | physx::PxHitFlag::eNORMAL | physx::PxHitFlag::eUV;
		Transform possessedWorld = GetTransform()->GetWorldTransform();

		auto frontVec = possessedWorld.GetFrontVec();
		bool bHit = Scene.GetPhysicsData()->GetPxScene()->raycast(Physics::Util::toPx(possessedWorld.GetPos() + frontVec), Physics::Util::toPx(frontVec), 100.0f, castBuffer);
		std::cout << "Hit? " << bHit << '\n';
		if (bHit)
		{
			auto hit = castBuffer.getAnyHit(0);
			std::cout << "Hit distance: " << hit.distance << '\n';
			std::cout << "Hit actor ptr: " << hit.actor << '\n';

			std::vector<PawnActor*> pawnActors;
			Scene.GetRootActor()->GetAllActors<PawnActor>(&pawnActors);

			std::cout << "Found " << pawnActors.size() << " pawn actors.\n";

			for (auto pawn : pawnActors)
				if (auto foundHead = pawn->GetRoot()->GetComponent<Component>("Head"))
					if (foundHead->GetCollisionObj() && foundHead->GetCollisionObj()->ActorPtr == hit.actor)
					{
						std::cout << "HEADSHOT!\n";
						pawn->InflictDamage(100.0f, -Math::CutYAxis(GetTransform()->GetWorldTransform().GetPos() - pawn->GetTransform()->GetWorldTransform().GetPos()));
					}

			if (hit.actor)
				if (hit.actor->getName())
					std::cout << "Hit actor name: " << hit.actor->getName() << '\n';
		}
		//physx::PxU32 hitCount = physx::PxGeometryQuery::raycast(origin, unitDir, geom, pose, maxDist, hitFlags, maxHits, &hitInfo);

		std::vector<PawnActor*> pawnActors;
		Scene.GetRootActor()->GetAllActors<PawnActor>(&pawnActors);

		if (!pawnActors.empty())
			for (auto pawn : pawnActors)
				;// pawn->InflictDamage(50.0f, Math::CutYAxis(GetTransform()->GetWorldTransform().GetPos() - pawn->GetTransform()->GetWorldTransform().GetPos()));

		// Gun recoil animation
		{
			Interpolation recoilUp(0.0, 0.25, InterpolationType::Quintic, true);
			recoilUp.SetOnUpdateFunc([this](double T) mutable {
				GetTransform()->SetVecAxis<TVec::RotationEuler, VecAxis::X>(static_cast<float>(30.0 * T));
				return T == 1.0;
			});
			GameHandle->AddInterpolation(recoilUp);

			Interpolation recoilDown(0.25, 1.25, InterpolationType::Quadratic, true);
			recoilDown.SetOnUpdateFunc([this](double T) mutable {
				GetTransform()->SetVecAxis<TVec::RotationEuler, VecAxis::X>(static_cast<float>(30.0 * (1.0 - T)));
				return T == 1.0;
			});
			GameHandle->AddInterpolation(recoilDown);

			//GetRoot()->GetTransform().AddInterpolator<Quatf>("rotation", 0.0f, 0.25f, Quatf(Vec3f(0.0f)), toQuat(Vec3f(30.0f, 0.0f, 0.0f)), InterpolationType::Quintic, true);
			//GetRoot()->GetTransform().AddInterpolator<Quatf>("rotation", 0.25f, 1.25f, Quatf(Vec3f(0.0f)), InterpolationType::Quadratic, true);
		}

		if (BulletRadius > 0.0f)
		{
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
		}


		CooldownLeft = FireCooldown;
	}
	void GunActor::GetEditorDescription(EditorDescriptionBuilder descBuilder)
	{
		Actor::GetEditorDescription(descBuilder);

		UICanvasField& blastField = descBuilder.AddField("Blast sound");
		blastField.GetTemplates().ObjectInput<Component, Audio::SoundSourceComponent>(*GetRoot(), BlastSound);

		descBuilder.AddField("Fire model").GetTemplates().ObjectInput<Component, ModelComponent>(*GetRoot(), [this](ModelComponent* model) { SetFireModel(model); });
		descBuilder.AddField("Gun light").GetTemplates().ObjectInput<Component, LightComponent>(*GetRoot(), [this](LightComponent* light) { GunLight = light; });
		
		descBuilder.AddField("Bullet radius").CreateChild<UIInputBoxActor>("BulletRadiusInputBox", [&](float val) {BulletRadius = val; }, [&]() { return BulletRadius; });
		descBuilder.AddField("Impulse factor").CreateChild<UIInputBoxActor>("ImpulseFactorInputBox", [&](float val) {ImpulseFactor = val; }, [&]() { return ImpulseFactor; });

		UICanvasField& fireField = descBuilder.AddField("Fire");
		fireField.CreateChild<UIButtonActor>("FireButton", "Fire", [this]() { FireWeapon(); });

		//[this]() {}
	}

	template<typename Archive>
	void GunActor::Save(Archive& archive) const
	{
		archive(cereal::make_nvp("FireModelID", GetGEEIDPtr(FireModel)), cereal::make_nvp("BlastSoundID", GetGEEIDPtr(BlastSound)), cereal::make_nvp("GunLightID", GetGEEIDPtr(GunLight)),
				CEREAL_NVP(BulletRadius), CEREAL_NVP(ImpulseFactor), cereal::base_class<Actor>(this));
	}

	template<typename Archive>
	void GunActor::Load(Archive& archive)
	{
		GEEID fireModelID(0), blastSoundID(0), lightID(0);
		archive(cereal::make_nvp("FireModelID", fireModelID), cereal::make_nvp("BlastSoundID", blastSoundID), cereal::make_nvp("GunLightID", lightID),
				CEREAL_NVP(BulletRadius), CEREAL_NVP(ImpulseFactor), cereal::base_class<Actor>(this));

		Scene.AddPostLoadLambda([this, fireModelID, blastSoundID, lightID]() {
			SetFireModel(GetRoot()->GetComponent<ModelComponent>(fireModelID));
			BlastSound = GetRoot()->GetComponent<Audio::SoundSourceComponent>(blastSoundID);
			GunLight = GetRoot()->GetComponent<LightComponent>(lightID);
			});
	}

	template void GunActor::Save<cereal::JSONOutputArchive>(cereal::JSONOutputArchive&) const;
	template void GunActor::Load<cereal::JSONInputArchive>(cereal::JSONInputArchive&);
}

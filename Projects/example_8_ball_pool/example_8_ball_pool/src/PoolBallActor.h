#pragma once
#include <scene/Actor.h>

namespace GEE
{
	class PoolBallActor : public GEE::Actor
	{
	public:
		PoolBallActor(GEE::GameScene&, GEE::Actor* parentActor, const std::string& name, const GEE::Transform& t = GEE::Transform());
		void Update(Time) override;
	};
}

GEE_POLYMORPHIC_SERIALIZABLE_ACTOR(GEE::Actor, GEE::PoolBallActor);
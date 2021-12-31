#pragma once
#include <scene/Actor.h>

class PoolBallActor : public GEE::Actor
{
public:
	PoolBallActor(GEE::GameScene&, GEE::Actor* parentActor, const std::string& name, const GEE::Transform& t = GEE::Transform());
};


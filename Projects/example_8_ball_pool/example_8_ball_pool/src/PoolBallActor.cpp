#include "PoolBallActor.h"
//#include <PoolBall.h>

namespace GEE
{
	PoolBallActor::PoolBallActor(GEE::GameScene& scene, GEE::Actor* parentActor, const std::string& name, const GEE::Transform& t):
		Actor(scene, parentActor, name, t)
	{
	}

	void PoolBallActor::Update(Time time)
	{
		Actor::Update(time);


		
	}
}

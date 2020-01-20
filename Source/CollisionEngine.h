#pragma once
#include "Collision.h"

class CollisionEngine
{
	std::vector <CollisionComponent*> CollisionInstances;

public:

	void AddCollisionInstance(CollisionComponent* comp) { CollisionInstances.push_back(comp); }
	bool CheckForCollision(CollisionComponent*, std::vector<glm::vec3>*);
	bool CheckForCollision(std::vector<CollisionComponent*>, std::vector<glm::vec3>*, std::vector<CollisionComponent*>* = nullptr);
	void Draw(Shader*);
};

template<class T>bool isComponentInVector(std::vector<T*>, T*);
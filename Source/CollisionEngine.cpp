#include "CollisionEngine.h"

bool CollisionEngine::CheckForCollision(CollisionComponent* component, std::vector<glm::vec3>* bounceNormals)
{
	for (unsigned int i = 0; i < CollisionInstances.size(); i++)
	{
		if (component == CollisionInstances[i])
			continue;

		CollisionType type = Collision::CheckForCollision(CollisionInstances[i], component, bounceNormals);
		if ((!CollisionInstances[i]->bFlipCollisionSide && type != CollisionType::NONE) || (CollisionInstances[i]->bFlipCollisionSide && type == CollisionType::INTERSECT))
			return true;
	}
	return false;
}

bool CollisionEngine::CheckForCollision(std::vector<CollisionComponent*> components, std::vector<glm::vec3>* bounceNormals, std::vector<CollisionComponent*>* collidingComponents)
{
	bool bCollision = false;	//domyslnie kolizja nie zachodzi...
	for (unsigned int i = 0; i < CollisionInstances.size(); i++)
	{
		if (isComponentInVector(components, CollisionInstances[i]))
			continue;

		for (unsigned int j = 0; j < components.size(); j++)
		{
			if (components[j] == CollisionInstances[i])
				continue;

			CollisionType type = Collision::CheckForCollision(CollisionInstances[i], components[j], bounceNormals);
			if ((!CollisionInstances[i]->bFlipCollisionSide && type != CollisionType::NONE) || (CollisionInstances[i]->bFlipCollisionSide && type == CollisionType::INTERSECT))	//zaszla kolizja
			{
				if (collidingComponents)	//moze sie zdarzyc, ze funkcja wywolujaca bedzie chciala otrzymac te obiekty, ktore koliduja z przekazanymi - przekazmy je wiec
					collidingComponents->push_back(CollisionInstances[i]);
				else if (!bounceNormals)
					return true;	//...jesli nie potrzebujemy obliczac normalnych, nie ma sensu sprawdzac dalej kolizji - funkcja na pewno zostala wywolana tylko w celu sprawdzenia kolizji (bez liczenia normalsow)
				bCollision = true; //...jesli musimy liczyc, to zaznaczamy ze kolizja zaszla na 100% i nie ma innego wyjscia; sprawdzamy dalej
			}
		}
		
	}
	return bCollision;
}

std::vector <std::pair<EngineObjectTypes, const Transform*>> CollisionEngine::GetCollisionInstancesDebugData()
{
	std::vector <std::pair<EngineObjectTypes, const Transform*>> data;
	for (unsigned int i = 0; i < CollisionInstances.size(); i++)
	{
		std::pair <EngineObjectTypes, const Transform*> instanceData;

		if (dynamic_cast<BSphere*>(CollisionInstances[i]))
			instanceData.first = EngineObjectTypes::SPHERE;
		else
			instanceData.first = EngineObjectTypes::CUBE;

		instanceData.second = CollisionInstances[i]->GetTransform();
		data.push_back(instanceData);
	}

	return data;
}

template<class T>bool isComponentInVector(std::vector <T*> v, T* comp)
{
	for (unsigned int i = 0; i < v.size(); i++)
		if (v[i] == comp)
			return true;

	return false;
}
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

void CollisionEngine::Draw(Shader* shader)
{
	shader->Use();
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glDisable(GL_CULL_FACE);

	glm::vec3 vertices[] = {
		//FRONT
		glm::vec3( 0.5f, -0.5f, -0.5f),
		glm::vec3(-0.5f, -0.5f, -0.5f),
		glm::vec3(-0.5f,  0.5f, -0.5f),

		glm::vec3(-0.5f,  0.5f, -0.5f),
		glm::vec3( 0.5f,  0.5f, -0.5f),
		glm::vec3( 0.5f, -0.5f, -0.5f),

		//BACK
		glm::vec3(-0.5f, -0.5f,  0.5f),
		glm::vec3( 0.5f, -0.5f,  0.5f),
		glm::vec3( 0.5f,  0.5f,  0.5f),

		glm::vec3( 0.5f,  0.5f,  0.5f),
		glm::vec3(-0.5f,  0.5f,  0.5f),
		glm::vec3(-0.5f, -0.5f,  0.5f),

		//LEFT
		glm::vec3(-0.5f, -0.5f, -0.5f),
		glm::vec3(-0.5f, -0.5f,  0.5f),
		glm::vec3(-0.5f,  0.5f,  0.5f),

		glm::vec3(-0.5f,  0.5f,  0.5f),
		glm::vec3(-0.5f,  0.5f, -0.5f),
		glm::vec3(-0.5f, -0.5f, -0.5f),
		
		//RIGHT
		glm::vec3( 0.5f, -0.5f,  0.5f),
		glm::vec3( 0.5f, -0.5f, -0.5f),
		glm::vec3( 0.5f,  0.5f, -0.5f),

		glm::vec3( 0.5f,  0.5f, -0.5f),
		glm::vec3( 0.5f,  0.5f,  0.5f),
		glm::vec3( 0.5f, -0.5f,  0.5f),

		//TOP
		glm::vec3(-0.5f,  0.5f,  0.5f),
		glm::vec3( 0.5f,  0.5f,  0.5f),
		glm::vec3( 0.5f,  0.5f, -0.5f),

		glm::vec3( 0.5f,  0.5f, -0.5f),
		glm::vec3(-0.5f,  0.5f, -0.5f),
		glm::vec3(-0.5f,  0.5f,  0.5f),

		//BOTTOM
		glm::vec3(-0.5f, -0.5f, -0.5f),
		glm::vec3( 0.5f, -0.5f, -0.5f),
		glm::vec3( 0.5f, -0.5f,  0.5f),

		glm::vec3 (0.5f, -0.5f,  0.5f),
		glm::vec3(-0.5f, -0.5f,  0.5f),
		glm::vec3(-0.5f, -0.5f, -0.5f)
	};

	unsigned int VBO, VAO;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);

	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)(nullptr));
	glEnableVertexAttribArray(0);
	
	glm::vec3 colors[6] = {
		glm::vec3(1.0f, 0.0f, 0.0f),
		glm::vec3(0.0f, 1.0f, 0.0f),
		glm::vec3(0.0f, 0.0f, 1.0f),
		glm::vec3(1.0f, 1.0f, 0.0f),
		glm::vec3(1.0f, 0.0f, 1.0f),
		glm::vec3(0.0f, 1.0f, 1.0f)
	};
	for (unsigned int i = 0; i < CollisionInstances.size(); i++)
	{
		Transform t = CollisionInstances[i]->GetTransform()->GetWorldTransform();
		shader->UniformMatrix4fv("model", t.GetMatrix());
		shader->Uniform3fv("color", colors[i % 4]);
		glDrawArrays(GL_TRIANGLES, 0, 36);
	}

	glDeleteBuffers(1, &VBO);
	glDeleteVertexArrays(1, &VAO);

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	//glEnable(GL_CULL_FACE);
}

template<class T>bool isComponentInVector(std::vector <T*> v, T* comp)
{
	for (unsigned int i = 0; i < v.size(); i++)
		if (v[i] == comp)
			return true;

	return false;
}
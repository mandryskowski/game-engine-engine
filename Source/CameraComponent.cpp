#include "CameraComponent.h"

CameraComponent::CameraComponent(std::string name, glm::uvec2 screenSize, CollisionEngine *collisionEng, float speedPerSec, Transform transform):
	Component(name, transform),
	CollisionEng(collisionEng),
	VelocityPerSec(glm::vec3(0.0f)),
	SpeedPerSec(speedPerSec),
	GroundCheckComponent(new BBox("twojababka")),
	HoverHeight(0.3f),
	bHoverHeightUnlocked(false),
	Projection(glm::perspective(glm::radians(90.0f), ((float)screenSize.x / (float)screenSize.y), 0.01f, 100.0f))
{
	ComponentTransform.SetFront(glm::vec3(0.0f, 0.0f, -1.0f));	//to jest kierunek, w ktorym poczatkowo patrzy sie nasz komponent (domyslnie negatywne Z)
	ComponentTransform.bConstrain = true;		//zmieniamy rotacje tego komponentu z XYZ na YXZ, zeby w wygodny sposob obracac nim myszka (w przypadku XYZ obrot pitch i yaw dodaje rowniez roll - utrudnia to obliczenia)
	GroundCheckComponent->SetTransform(Transform(glm::vec3(0.0f, -0.65f, 0.0f), glm::vec3(0.0f), glm::vec3(1.0f)));
}


glm::mat4 CameraComponent::GetProjectionMat()
{
	return Projection;
}

glm::mat4 CameraComponent::GetVP(Transform* worldTransform)
{
	if (worldTransform)
		return Projection * worldTransform->GetViewMatrix();
	else
		return Projection * ComponentTransform.GetWorldTransform().GetViewMatrix();
}

bool CameraComponent::PerformCollisionCheck(std::vector<CollisionComponent*> components, glm::vec3 offset, std::vector<glm::vec3>* bounceNormals, std::vector<CollisionComponent*>* collidingComponents)
{
	ComponentTransform.Move(offset);
	bool type = CollisionEng->CheckForCollision(components, bounceNormals, collidingComponents);
	ComponentTransform.Move(-offset);
	return type;
}

glm::vec3 CameraComponent::ApplyCollisionResponse(glm::vec3 offset)
{
	if (offset == glm::vec3(0.0f))
		return glm::vec3(0.0f);
	
	std::vector <CollisionComponent*> collisionChildren;
	GetAllComponents<CollisionComponent>(&collisionChildren);
//	GroundCheckComponent = collisionChildren[0];
	std::vector <glm::vec3> bounceNormals;

	if(!PerformCollisionCheck(collisionChildren, offset, &bounceNormals))	//na poczatku sprawdzamy, czy przesuniecie obiektu o offset wywola kolizje; jesli nie, to po pomijamy reszte funkcji -  nie ma potrzeby zmieniac offsetu
		return offset;

	glm::vec3 offsetSum = offset;
	glm::vec3 potentialVelocity = VelocityPerSec;
	for (unsigned int i = 0; i < bounceNormals.size(); i++)
	{
		glm::mat3 inverseRotMat = ComponentTransform.GetParentTransform()->GetWorldTransform().GetRotationMatrix(-1.0f);	//obliczamy odwrotna macierz rotacji, aby przeksztalcic wektory normalne do przestrzeni lokalnej (local space);
		bounceNormals[i] = inverseRotMat * bounceNormals[i];	//przeksztalcamy do local space, poniewaz offset jest wlasnie w local space; obliczanie zmiany pozycji w Update tez jest w tej przestrzeni!
		glm::vec3 offsetProjected = dot(offset, bounceNormals[i]) * bounceNormals[i];
		glm::vec3 currentOffset = offset - offsetProjected;
		offsetSum -= offsetProjected;
		potentialVelocity -= dot(VelocityPerSec, bounceNormals[i]) * bounceNormals[i];

		if (!PerformCollisionCheck(collisionChildren, currentOffset, nullptr))
		{
			VelocityPerSec -= dot(VelocityPerSec, bounceNormals[i]) * bounceNormals[i];
			return currentOffset;
		}
	}

	if (!PerformCollisionCheck(collisionChildren, offsetSum, nullptr))
	{
		VelocityPerSec = potentialVelocity;
		return offsetSum;
	}

	return glm::vec3(0.0f);	//nie pozwalamy sie przemiescic obiektowi w zaden wziety pod uwage sposob; nie dopuszczamy do ruchu
}

void CameraComponent::RotateWithMouse(glm::vec2 mouseOffset)
{
	glm::vec3 Rotation = ComponentTransform.RotationRef;

	float sensitivity = 0.15f;
	Rotation.x -= mouseOffset.y * sensitivity;
	Rotation.y -= mouseOffset.x * sensitivity;

	Rotation.x = glm::clamp(Rotation.x, -89.9f, 89.9f);
	Rotation.y = fmod(Rotation.y, 360.0f);

	ComponentTransform.SetRotation(Rotation);

	std::vector <CollisionComponent*> collisionChildren;
	GetAllComponents<CollisionComponent, BBox>(&collisionChildren);

	glm::vec3 minSize(0.1f);
	for (unsigned int i = 0; i < collisionChildren.size(); i++)
	{
		glm::vec3 childSize = collisionChildren[i]->GetTransform()->ScaleRef;

		if (i == 0)
		{
			minSize = childSize;
			continue;
		}

		if (length2(childSize) < length2(minSize))
			minSize = childSize;
	}

	std::vector <glm::vec3> bounceNormals;
	CollisionEng->CheckForCollision(collisionChildren, &bounceNormals);
	std::vector <glm::vec3> bNormals;

	while (CollisionEng->CheckForCollision(collisionChildren, &bNormals))
	{
		if ((bNormals.empty()) || (bNormals != bounceNormals))	//jesli wektor normalnych zmieni sie w trakcie przemieszczania, to nasz komponent utknal - nie da sie go przemiescic w zaden realistyczny sposob
			break;
		bounceNormals = bNormals;

		for (unsigned int i = 0; i < bounceNormals.size(); i++)
		{
			glm::mat3 inverseRotMat = ComponentTransform.GetParentTransform()->GetWorldTransform().GetRotationMatrix(-1.0f);
			bounceNormals[i] = inverseRotMat * bounceNormals[i];
			ComponentTransform.Move(bounceNormals[i] * minSize * glm::vec3(0.01f)); //mnozymy normalna przez czesc rozmiaru obiektu kolizji, aby odsunac go od sciany
		}
	}
}

void CameraComponent::HandleInputs(GLFWwindow* window, float deltaTime)
{
	glm::vec3 Front = ComponentTransform.GetRotationMatrix() * ComponentTransform.FrontRef;
	glm::vec3 FrontFPS = glm::normalize(glm::vec3(Front.x, 0.0f, Front.z));

	if (true)
	{
		VelocityPerSec.x = 0.0f;
		VelocityPerSec.z = 0.0f;
		if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
			VelocityPerSec += FrontFPS;
		if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
			VelocityPerSec += glm::cross(FrontFPS, glm::vec3(0.0f, 1.0f, 0.0f));
		if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
			VelocityPerSec -= glm::cross(FrontFPS, glm::vec3(0.0f, 1.0f, 0.0f));
		if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
			VelocityPerSec -= FrontFPS;
	}
	if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS)
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS)
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	if (glfwGetKey(window, GLFW_KEY_9) == GLFW_PRESS)
	{
		for (unsigned int xd = 0; xd < Children.size(); xd++)
		{
			CollisionComponent* col = dynamic_cast<CollisionComponent*>(Children[xd]);
			if (!col)
				continue;
			glm::vec3 scale = Children[xd]->GetTransform()->ScaleRef;
			Children[xd]->SetTransform(ComponentTransform);
			Children[xd]->GetTransform()->SetParentTransform(ComponentTransform.GetParentTransform());
			Children[xd]->GetTransform()->SetScale(scale);
		}
		Children.clear();
	}
	if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
		HoverHeight += deltaTime;
	if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
		HoverHeight -= deltaTime;
	else
		bHoverHeightUnlocked = false;
}

void CameraComponent::Update(float deltaTime)
{
	glm::vec3 offset(0.0f);
	offset += VelocityPerSec * deltaTime;

	offset = ApplyCollisionResponse(offset);

	ComponentTransform.Move(offset);	//przemieszczamy obiekt (WAZNE: W LOCAL SPACE!!!)

	/*std::vector <CollisionComponent*> collidingAtGround;
	GroundCheckComponent->GetTransform()->SetPosition(ComponentTransform.PositionRef - glm::vec3(0.0f, 0.65f, 0.0f));
	if (GroundCheckComponent)
		PerformCollisionCheck(std::vector<CollisionComponent*> {GroundCheckComponent}, glm::vec3(0.0f, -0.5f, 0.0f), nullptr, &collidingAtGround);

	float distanceFromGround = 0.0f;
	for (unsigned int i = 0; i < collidingAtGround.size(); i++)
	{
		float topmostPosition = collidingAtGround[i]->GetTransform()->GetWorldTransform().PositionRef.y - (collidingAtGround[i]->GetTransform()->GetWorldTransform().ScaleRef.y / 2.0f);
		float dist = ComponentTransform.PositionRef.y - topmostPosition;
		if (dist < distanceFromGround || i == 0)
			distanceFromGround = dist;
	}*/

	float limit = 5.0f;
	HoverHeight = glm::clamp(HoverHeight, -limit, limit);

	//std::cout << distanceFromGround << '\n';

	float ypos = ComponentTransform.PositionRef.y;

	if (ypos != HoverHeight)
		VelocityPerSec.y = (HoverHeight - ypos) * 5.0f;
}
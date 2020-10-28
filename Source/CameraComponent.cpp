#include "CameraComponent.h"

CameraComponent::CameraComponent(GameManager* gameHandle, std::string name, float speedPerSec) :
	Component(gameHandle, name, Transform(glm::vec3(0.0f), glm::vec3(0.0f), glm::vec3(1.0f), glm::vec3(0.0f, 0.0f, -1.0f))),
	RotationEuler(0.0f),
	VelocityPerSec(glm::vec3(0.0f)),
	SpeedPerSec(speedPerSec),
	HoverHeight(0.5f),
	bHoverHeightUnlocked(false),
	Projection(glm::perspective(glm::radians(90.0f), ((float)gameHandle->GetGameSettings()->WindowSize.x / (float)gameHandle->GetGameSettings()->WindowSize.y), 0.01f, 100.0f)),
	MovementAxises()
{
	ComponentTransform.SetFront(glm::vec3(0.0f, 0.0f, -1.0f));	//to jest kierunek, w ktorym poczatkowo patrzy sie nasz komponent (domyslnie negatywne Z)
	ComponentTransform.bConstrain = true;		//zmieniamy rotacje tego komponentu z XYZ na YXZ, zeby w wygodny sposob obracac nim myszka (w przypadku XYZ obrot pitch i yaw dodaje rowniez roll - utrudnia to obliczenia)

	std::generate(MovementAxises.begin(), MovementAxises.end(),
		[]()
		{
			static int i = 0;
			MovementAxis dir;
			dir.MovementInterpolator = std::make_unique<Interpolator<float>>(Interpolator<float>(0.0f, 0.15f, 0.0f, 1.0f, InterpolationType::LINEAR));
			dir.Inversed = false;

			switch (static_cast<MovementDir>(i))
			{
			case FORWARD: dir.Direction = glm::vec3(0.0f, 0.0f, -1.0f); break;
			case BACKWARD: dir.Direction = glm::vec3(0.0f, 0.0f, 1.0f); break;
			case LEFT: dir.Direction = glm::vec3(-1.0f, 0.0f, 0.0f); break;
			case RIGHT: dir.Direction = glm::vec3(1.0f, 0.0f, 0.0f); break;
			}
			i++;
			return dir;
		});
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

void CameraComponent::RotateWithMouse(glm::vec2 mouseOffset)
{
	float sensitivity = 0.15f;
	RotationEuler.x -= mouseOffset.y * sensitivity;
	RotationEuler.y -= mouseOffset.x * sensitivity;

	RotationEuler.x = glm::clamp(RotationEuler.x, -89.9f, 89.9f);
	RotationEuler.y = fmod(RotationEuler.y, 360.0f);

	ComponentTransform.SetRotation(RotationEuler);

	/*
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
	*/
}

void CameraComponent::HandleInputs(GLFWwindow* window, float deltaTime)
{
	glm::vec3 Front = ComponentTransform.GetRotationMatrix() * ComponentTransform.FrontRef;
	glm::vec3 FrontFPS = glm::normalize(glm::vec3(Front.x, 0.0f, Front.z));

	VelocityPerSec.x = 0.0f;
	VelocityPerSec.z = 0.0f;
	HandleMovementAxis(glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS, MovementAxises[FORWARD]);
	HandleMovementAxis(glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS, MovementAxises[BACKWARD]);
	HandleMovementAxis(glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS, MovementAxises[LEFT]);
	HandleMovementAxis(glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS, MovementAxises[RIGHT]);
	if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS)
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS)
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
		HoverHeight += deltaTime;
	if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
		HoverHeight -= deltaTime;
	else
		bHoverHeightUnlocked = false;
}

void CameraComponent::HandleMovementAxis(bool pressed, MovementAxis& axis)
{
	if ((pressed && axis.Inversed) || (!pressed && !axis.Inversed))
	{
		axis.MovementInterpolator->Inverse();
		axis.Inversed = !axis.Inversed;
	}
}

void CameraComponent::Update(float deltaTime)
{
	VelocityPerSec.x = 0.0f;
	VelocityPerSec.z = 0.0f;
	for (int i = 0; i < 4; i++)
	{
		MovementAxises[i].MovementInterpolator->Update(deltaTime);
		glm::vec3 dir = ComponentTransform.GetRotationMatrix() * MovementAxises[i].Direction;
		dir = glm::normalize(glm::vec3(dir.x, 0.0f, dir.z));
		VelocityPerSec += dir * MovementAxises[i].MovementInterpolator->GetCurrentValue();
	}
	//for (auto& it : MovementInterpolators)
	{
		//float val = MovementAxises[0].MovementInterpolator->Update(deltaTime);
		float t = MovementAxises[0].MovementInterpolator->GetInterp()->GetT();

	//	std::cout << "Wartosc: " << MovementAxises[0].MovementInterpolator->GetCurrentValue() << '\n';
	}
	glm::vec3 offset(0.0f);
	offset += VelocityPerSec * deltaTime;

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
#include "Collision.h"
#include <glm/gtx/euler_angles.hpp>
/*  ===========================================  */
/*  ===========================================  */
/*  ===========================================  */
/*  ===========================================  */

/*						  SAT						 */

void Collision::GetCubeMinMax(glm::vec3 axis, BBox cube, float* min, float* max)
{
	glm::vec3 cubeVerts[] = {
		glm::vec3(-0.5f, -0.5f,  0.5f),
		glm::vec3( 0.5f, -0.5f,  0.5f),
		glm::vec3( 0.5f,  0.5f,  0.5f),
		glm::vec3(-0.5f,  0.5f,  0.5f),

		glm::vec3(-0.5f, -0.5f, -0.5f),
		glm::vec3( 0.5f, -0.5f, -0.5f),
		glm::vec3( 0.5f,  0.5f, -0.5f),
		glm::vec3(-0.5f,  0.5f, -0.5f)
	};

	glm::mat4 model = cube.GetTransform()->GetWorldTransform().GetMatrix();
	for (unsigned int i = 0; i < 8; i++)
	{
		cubeVerts[i] = model * glm::vec4(cubeVerts[i], 1.0f);

		float proj = glm::dot(cubeVerts[i], axis);

		if (i == 0)
		{
			*max = proj;
			*min = proj;
			continue;
		}

		if (proj < *min) *min = proj;
		if (proj > * max) *max = proj;
	}
}

CollisionType Collision::CheckAxis(glm::vec3 axis, BBox cube1, BBox cube2, bool& bPositiveNormal)
{
	float min1, max1;
	float min2, max2;

	GetCubeMinMax(axis, cube1, &min1, &max1);
	GetCubeMinMax(axis, cube2, &min2, &max2);

	if (min2 >= min1) bPositiveNormal = true;
	else bPositiveNormal = false;

	if (cube1.bFlipCollisionSide) bPositiveNormal = !bPositiveNormal;

	if ((min1 >= min2 && max1 <= max2) || (min2 >= min1 && max2 <= max1))
		return CollisionType::CONTAIN;

	if ((min2 < max1) && (max2 > min1))
		return CollisionType::INTERSECT;

	return CollisionType::NONE;
}

//////////////////////////SAT//////////////////////////

CollisionType Collision::CubeCube(BBox cube1, BBox cube2, std::vector<glm::vec3>* bounceNormals)
{
	std::vector <glm::vec3> axises;
	unsigned int containments = 0;

	for (int i = 0; i < 2; i++)
	{
		glm::mat3 normalMat = (i == 0) ? (cube1.GetTransform()->GetWorldTransform().GetRotationMatrix()) : (cube2.GetTransform()->GetWorldTransform().GetRotationMatrix());

		axises.push_back(normalMat * glm::vec3(1.0f, 0.0f, 0.0f));
		axises.push_back(normalMat * glm::vec3(0.0f, 1.0f, 0.0f));
		axises.push_back(normalMat * glm::vec3(0.0f, 0.0f, 1.0f));

		if (cube1.GetTransform()->GetWorldTransform().Rotation == cube2.GetTransform()->GetWorldTransform().Rotation)
			break;
	}
	
	for (unsigned int i = 0; i < axises.size(); i++)
	{
		bool bPositiveNormal; //ten boolean jest podawany dalej do CheckAxis, gdzie sprawdzane jest czy sciana obiektu 1 ktora potencjalnie koliduje z obiektem 2 ma normalna zwrocona w strone dodatnia osi, czy jest ujemna
		CollisionType type = CheckAxis(axises[i], cube1, cube2, bPositiveNormal);

		if (type == INTERSECT && i < 3 && bounceNormals)
			bounceNormals->push_back(glm::normalize(axises[i] * ((bPositiveNormal) ? (1.0f) : (-1.0f))));

		else if (type == CONTAIN) containments++;

		else if (type == NONE) return CollisionType::NONE;
	}
	
	if (containments == axises.size())
		return CollisionType::CONTAIN;

	return CollisionType::INTERSECT;
}

CollisionType Collision::CubeSphere(BBox cube1, BSphere sphere1, std::vector<glm::vec3>* bounceNormals)
{
	Transform* sphereTransform = sphere1.GetTransform();
	Transform* cubeTransform = cube1.GetTransform();

	*sphereTransform = sphereTransform->GetWorldTransform();
	*cubeTransform = cubeTransform->GetWorldTransform();
	sphereTransform->SetParentTransform(cubeTransform, true);
	glm::mat4 rotMat = cubeTransform->GetRotationMatrix(1.0f);

	float radius2 = pow(sphere1.GetTransform()->Scale.x / 2.0f, 2);
	glm::vec3 cubeHalfSize = cube1.GetTransform()->Scale / 2.0f;	//obliczamy polowe rozmiarow kostki (w 3 wymiarach, pamietajmy ze w kazdym moga byc inne) aby uproscic obliczenia w nastepnej linijce
	glm::vec3 closestPointInBox = glm::clamp(sphereTransform->Position, -cubeHalfSize, cubeHalfSize); //znajdujemy punkt w kostce, ktory jest najblizej srodka kuli

	glm::vec3 displacement = sphereTransform->Position - closestPointInBox;	//obliczamy odleglosc najblizszego punktu do srodka kuli od srodka kuli...

	if (length2(displacement) > radius2)							//...jesli odleglosc srodka kuli od tego punktu jest wieksza niz promien tego kola, to mamy 100% pewnosc, ze kolizja nie zachodzi
		return CollisionType::NONE;

	std::vector <glm::vec3> bNormals = getBounceNormals(cube1, sphere1);


	if ((displacement == glm::vec3(0.0f)) && (bNormals.empty()))	//...moze nie byc zadnej odleglosci, jesli srodek kuli znajduje sie w kostce; wiemy, ze kula znajduje sie w srodku kostki
		return CollisionType::CONTAIN;

	for (unsigned int i = 0; i < bNormals.size(); i++)
		bNormals[i] = rotMat * glm::vec4(bNormals[i], 1.0f);

	if (bounceNormals)
		bounceNormals->insert(bounceNormals->end(), bNormals.begin(), bNormals.end());
	
	return CollisionType::INTERSECT;
}

bool Collision::SpherePoint(BSphere sphere, glm::vec3 point)
{
	float radius2 = pow(sphere.GetTransform()->Scale.x, 2);
	float distance2 = length2(point - sphere.GetTransform()->Position);
	return distance2 <= radius2;
}

CollisionType Collision::SphereSphere(BSphere sphere1, BSphere sphere2)
{
	float radius1 = sphere1.GetTransform()->Scale.x, radius2 = sphere2.GetTransform()->Scale.x;
	float distance2 = length2(sphere2.GetTransform()->Position - sphere1.GetTransform()->Position);
	float radiusSum2 = pow((radius1 + radius2), 2);

	if (distance2 > radiusSum2)
		return CollisionType::NONE;

	float distance = sqrt(distance2);
	if ((distance + radius1 <= radius2) || (distance + radius2 <= radius1))
		return CollisionType::CONTAIN;

	return CollisionType::INTERSECT;
}

CollisionType Collision::CheckForCollision(CollisionComponent* comp1, CollisionComponent* comp2, std::vector <glm::vec3>* bounceNormals)
{
	BBox *box1 = dynamic_cast <BBox*>(comp1);
	BBox *box2 = dynamic_cast <BBox*>(comp2);
	BSphere* sphere1 = dynamic_cast<BSphere*>(comp1);
	BSphere* sphere2 = dynamic_cast<BSphere*>(comp2);

	if (box1 && box2)
		return Collision::CubeCube(*box1, *box2, bounceNormals);
	if (sphere1 && sphere2)
		return Collision::SphereSphere(*sphere1, *sphere2);
	if (box1 && sphere2)
		return Collision::CubeSphere(*box1, *sphere2, bounceNormals);

	return CollisionType::NONE;
}

float length2(glm::vec3 vec)
{
	return (vec.x * vec.x) + (vec.y * vec.y) + (vec.z * vec.z);
}

glm::vec3 clampToNearestFace(BBox cube, glm::vec3 point)
{
	glm::vec3 cubeHalfSize = cube.GetTransform()->Scale / 2.0f;
	glm::vec3 cubePos = cube.GetTransform()->Position;
	glm::vec3 closestClampPos(0.0f);
	float minDistance2 = length2(cubeHalfSize);

	for (int face = 0; face < 3; face++)
	{
		for (int sign = -1; sign <= 1; sign += 2)
		{
			glm::vec3 clampedPos = point;
			clampedPos[face] = cubePos[face] + (cubeHalfSize[face] * sign);

			float distance2FromFace = length2(point - clampedPos);

			if (distance2FromFace < minDistance2)
			{
				minDistance2 = distance2FromFace;
				closestClampPos = clampedPos;
			}
		}
	}

	return closestClampPos;
}

std::vector <glm::vec3> getBounceNormals(BBox cube, BSphere sphere)
{
	std::vector <glm::vec3> collidingNormals;

	glm::vec3 cubeHalfSize = cube.GetTransform()->Scale / 2.0f;
	float sphereRadius = sphere.GetTransform()->Scale.x / 2.0f;
	                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 
	for (int axis = 0; axis < 3; axis++)	//dla wszystkich osi...
	{
		for (int sign = -1; sign <= 1; sign += 2)
		{
			float distanceFromClamped = abs(cubeHalfSize[axis] * sign - sphere.GetTransform()->Position[axis]);	//...sprawdzamy najblizszy dystans punktu do sciany kostki o "axis" osi w kierunku "sign"
			//obie bryly (kostka i kula) znajduja sie w Przestrzeni Kostki, czyli kostka znajduje sie w (0, 0, 0) tego ukladu, jej rotacja rowniez wynosi 0 itd itp...
			//kula jest natomiast przemieszczona i obrocona wzglednie do kostki, co znacznie uproscilo nam obliczenia

			if (distanceFromClamped < sphereRadius)	//jesli dystans miedzy srodkiem kuli a wskazana sciana jest mniejszy niz promien kuli, to znaczy ze kula ta
			{
				glm::vec3 normal(0.0f);
				normal[axis] = (float)-sign;
				collidingNormals.push_back(normal);
				//break;	//wychodzimy z zagniezdzonej petli znaku, bo nie ma zadnego sensu sprawdzac normalne, ktore sa do siebie rownolegle
						//przekazanie dalej rownoleglych normalnych wymusi dodatkowe obliczenia, lub nawet wplynie na swiat gry (np obiekt kolizji od wartosci przemieszczenia odejmuje normalne, aby nie przemiescic sie w sciane...
						//a odjecie 2 normalnych na tej samej osi spowoduje odjecie "normalna - normalna", czyli "-(2 * normalna)" - obiekt odbije sie, zamiast plynnie przeslizgnac po powierzchni sciany
			}
		}
	}

	return collidingNormals;
}
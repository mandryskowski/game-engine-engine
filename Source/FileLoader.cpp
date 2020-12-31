#include "Mesh.h"
#include <assimp/scene.h>
#include "GunActor.h"
#include "CameraComponent.h"
#include "BoneComponent.h"
#include "LightComponent.h"
#include "MeshSystem.h"
#include "FileLoader.h"
#include "Texture.h"
#include "LightProbe.h"
#include "SoundSourceComponent.h"
#include "TextComponent.h"
#include "Font.h"
#include "Controller.h"
#include "GameScene.h"
#include <ft2build.h>
#include FT_FREETYPE_H
#include <functional>


void EngineDataLoader::LoadMaterials(RenderEngineManager* renderHandle, std::string path, std::string directory)
{
	std::ifstream file;
	file.open(path);
	std::stringstream filestr;
	filestr << file.rdbuf();

	if (!file.good())
	{
		std::cerr << "Cannot open materials file " << path << "!\n";
		return;
	}

	if (isNextWordEqual(filestr, "shaders"))
		LoadShaders(renderHandle, filestr, directory);

	while (!isNextWordEqual(filestr, "end"))
	{
		Material* material = nullptr;
		std::string materialName, shaderName;
		float shininess, depthScale;
		Shader* shader = nullptr;
		unsigned int texCount;

		filestr >> materialName;

		if (isNextWordEqual(filestr, "atlas"))
		{
			glm::vec2 size;
			filestr >> size.x >> size.y;
			material = new AtlasMaterial(materialName, size);
		}
		else
			material = new Material(materialName);

		filestr >> shaderName >> shininess >> depthScale >> texCount;

		material->SetRenderShaderName(shaderName);
		material->SetShininess(shininess);
		material->SetDepthScale(depthScale);

		renderHandle->AddMaterial(material);

		for (unsigned int i = 0; i < texCount; i++)
		{
			std::string path, shaderUniformName, isSRGB;
			filestr >> path >> shaderUniformName >> isSRGB;

			if (!(path.size() > 3 && path.substr(0, 2) == "./"))
				path = directory + path;
			material->AddTexture(new NamedTexture(textureFromFile(path, (toBool(isSRGB)) ? (GL_SRGB) : (GL_RGB)), shaderUniformName));
		}
	}
}

void EngineDataLoader::LoadShaders(RenderEngineManager* renderEngHandle, std::stringstream& filestr, std::string directory)
{
	while (!isNextWordEqual(filestr, "end"))
	{
		std::string name;
		std::string paths[3];
		unsigned int fileCount;

		filestr >> name >> fileCount;
		for (unsigned int i = 0; i < fileCount; i++)
		{
			filestr >> paths[i];

			if (!(paths[i].size() > 3 && paths[i].substr(0, 2) == "./"))
				paths[i] = directory + paths[i];
		}

		std::shared_ptr<Shader> shader = ShaderLoader::LoadShaders(name, paths[0], paths[1], paths[2]);

		unsigned int expectedMatricesCount, texUnitsCount;

		filestr >> expectedMatricesCount;
		for (unsigned int i = 0; i < expectedMatricesCount; i++)
		{
			std::string expectedMatrixType;
			filestr >> expectedMatrixType;

			shader->AddExpectedMatrix(expectedMatrixType);
		}

		filestr >> texUnitsCount;
		for (unsigned int i = 0; i < texUnitsCount; i++)
		{
			unsigned int unitIndex;
			std::string texUnitName;
			filestr >> unitIndex >> texUnitName;

			shader->AddTextureUnit(unitIndex, texUnitName);
		}

		renderEngHandle->AddShader(shader, true);
	}
}

void EngineDataLoader::LoadComponentData(GameManager* gameHandle, std::stringstream& filestr, Actor* currentActor, GameScene* scene)
{
	///////////////////////////////1. Load the type and the name of the current component
	RenderEngineManager* renderEngHandle = gameHandle->GetRenderEngineHandle();

	Component* comp = nullptr;
	std::string type, name;
	filestr >> type;
	name = multipleWordInput(filestr);

	///////////////////////////////2. Load info about its position in hierarchy

	std::string familyWord, parentName;
	Component* parent = nullptr;
	familyWord = lookupNextWord(filestr);
	if (familyWord == "child")
	{
		filestr >> familyWord >> parentName;	///skip familyword and load parentname
		if (Component* found = currentActor->GetRoot()->SearchForComponent(parentName))
			parent = found;
		else
			std::cerr << "ERROR! Can't find target parent " << parentName << "!\n";
	}
	else if (familyWord == "root")
		filestr >> familyWord;	///skip familyword
	else
	{
		parent = currentActor->GetRoot();
		familyWord = "attachToRoot";
	}

	///////////////////////////////3. Load component's data

	if (type == "model")
	{
		ModelComponent* modelPtr = nullptr;
		std::string path, materialName;

		path = multipleWordInput(filestr);

		MeshTreeInstancingType instancingType = MeshTreeInstancingType::ROOTTREE;
		if (isNextWordEqual(filestr, "roottree"))
			;
		else if (isNextWordEqual(filestr, "merge"))
			instancingType = MeshTreeInstancingType::MERGE;

		modelPtr = dynamic_cast<ModelComponent*>(scene->GetRenderData()->AddRenderable(std::make_shared<ModelComponent>(ModelComponent(scene, name, Transform()))).get());

		MeshSystem::MeshTree* tree = LoadMeshTree(gameHandle, renderEngHandle, path);
		std::unique_ptr<MeshSystem::MeshTree> tempTreeCopyForEdit = nullptr;
		if (isNextWordEqual(filestr, "edit"))
		{
			tempTreeCopyForEdit = std::make_unique<MeshSystem::MeshTree>(MeshSystem::MeshTree(*tree, "temp"));
			tree = tempTreeCopyForEdit.get();
			LoadCustomMeshNode(gameHandle, filestr, nullptr, tree);
		}
		InstantiateTree(modelPtr, *tree, instancingType, renderEngHandle->FindMaterial(materialName));

		comp = modelPtr;
	}
	else if (type == "light")
	{
		std::string lightType;
		filestr >> lightType;

		glm::mat4 projection;
		if (lightType == "point" || lightType == "spot")
			projection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
		else
			projection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, 1.0f, 10.0f);

		std::shared_ptr<LightComponent> lightPtr = std::make_shared<LightComponent>(scene, name, toLightType(lightType), scene->GetRenderData()->GetAvailableLightIndex(), scene->GetRenderData()->GetAvailableLightIndex(), 10.0f, projection);
		for (unsigned int vector = 0; vector < 3; vector++)
			for (unsigned int axis = 0; axis < 3; axis++)
				filestr >> (*lightPtr)[vector][axis];	//TODO: sprawdzic czy sie wywala

		glm::vec3 additionalData(0.0f);
		for (unsigned int i = 0; i < 3; i++)
			filestr >> additionalData[i];
		lightPtr->SetAdditionalData(additionalData);

		lightPtr->CalculateLightRadius();

		scene->GetRenderData()->AddLight(lightPtr);
		comp = lightPtr.get();
	}
	else if (type == "camera")
	{
		float speedPerSec;
		filestr >> speedPerSec;

		CameraComponent* camComp = new CameraComponent(scene, name, glm::perspective(glm::radians(90.0f), ((float)scene->GetGameHandle()->GetGameSettings()->Video.Resolution.x / (float)scene->GetGameHandle()->GetGameSettings()->Video.Resolution.y), 0.01f, 100.0f));

		if (isNextWordEqual(filestr, "active"))
			scene->BindActiveCamera(camComp);
		if (isNextWordEqual(filestr, "automousecontrol"))
		{
			std::shared_ptr<Controller> controller = std::make_shared<Controller>(Controller(scene, "MojTestowyController"));
			scene->AddActorToRoot(controller);
			gameHandle->PassMouseControl(controller.get());
		}

		comp = camComp;
	}
	else if (type == "soundsource")
	{
		SoundSourceComponent* sourcePtr = nullptr;
		std::string path;
		filestr >> path;
		sourcePtr = new SoundSourceComponent(scene, name, gameHandle->GetAudioEngineHandle()->LoadBufferFromFile(path));
		scene->GetAudioData()->AddSource(sourcePtr);

		comp = sourcePtr;
	}
	else if (type == "text")
	{
		std::string fontPath = multipleWordInput(filestr);
		std::string content = multipleWordInput(filestr);

		comp = TextComponent::Of(TextComponent(scene, name, Transform(), content, fontPath)).get();

	}
	else if (type == "empty")
		comp = new Component(scene, name, Transform());
	else
		return;

	///////////////////////////////4. Load its tranformations

	if (isNextWordEqual(filestr, "transform"))
		LoadTransform(filestr, comp->GetTransform());

	///////////////////////////////5.  After the component has been created, we can take care of its hierarchy stuff (it was loaded in 2.)

	if (familyWord == "root")
		currentActor->ReplaceRoot(comp);
	else if (parent)
		parent->AddComponent(comp);


	//////Check for an error

	std::string lastWord;
	filestr >> lastWord;
	if (lastWord != "end")
		std::cerr << "ERROR: There is no ''end'' after component's " << name << " definition! Detected word: " << lastWord << ".\n";
}

std::unique_ptr<CollisionObject> EngineDataLoader::LoadCollisionObject(PhysicsEngineManager* physicsHandle, std::stringstream& filestr)
{
	std::unique_ptr<CollisionObject> obj = std::make_unique<CollisionObject>();

	if (isNextWordEqual(filestr, "dynamic"))
		obj->IsStatic = false;


	int nrShapes, shapeType;
	std::string transformLoadType;

	filestr >> nrShapes >> transformLoadType;

	for (int i = 0; i < nrShapes; i++)
	{
		filestr >> shapeType;
		if (shapeType < CollisionShapeType::COLLISION_FIRST || shapeType > CollisionShapeType::COLLISION_LAST)
		{
			std::cerr << "ERROR! Shape type is " << shapeType << ", which is undefined.\n";
			continue;
		}

		if (shapeType == CollisionShapeType::COLLISION_TRIANGLE_MESH)
		{
			std::string path;
			filestr >> path;

			Assimp::Importer importer;
			const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate);
			if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
			{
				std::cerr << "Can't load col obj scene " << path << ".\n";
				continue;
			}

			Transform shapesTransform;
			LoadTransform(filestr, shapesTransform, transformLoadType);
			for (int j = 0; j < static_cast<int>(scene->mNumMeshes); j++)
				obj->AddShape(LoadTriangleMeshCollisionShape(physicsHandle, scene, *scene->mMeshes[i]))->ShapeTransform = shapesTransform;	//add new shape and set its transform to the loaded one
				//LoadMeshFromAi(nullptr, scene, scene->mMeshes[j], extractDirectory(path), false, nullptr, nullptr, &shape->VertData, &shape->IndicesData);
		}
		else
		{
			CollisionShape* shape = obj->AddShape(static_cast<CollisionShapeType>(shapeType));
			LoadTransform(filestr, shape->ShapeTransform, transformLoadType);
		}
	}

	return obj;
}

std::unique_ptr<CollisionShape> EngineDataLoader::LoadTriangleMeshCollisionShape(PhysicsEngineManager* physicsHandle, const aiScene* scene, aiMesh& mesh)
{
	std::unique_ptr<CollisionShape> shape = std::make_unique<CollisionShape>(CollisionShape(CollisionShapeType::COLLISION_TRIANGLE_MESH));
	LoadMeshFromAi(nullptr, scene, &mesh, "", false, nullptr, nullptr, &shape->VertData, &shape->IndicesData);

	return shape;
}

void EngineDataLoader::LoadLightProbes(GameScene* scene, std::stringstream& filestr)
{
	int probeCount;
	filestr >> probeCount;

	for (int i = 0; i < probeCount; i++)
	{
		bool bLocal;
		EngineBasicShape shape;
		std::string path;
		Transform probeTransform;
		
		filestr >> bLocal;
		if (bLocal)
		{
			int shapeNr;
			filestr >> shapeNr;
			shape = static_cast<EngineBasicShape>(shapeNr);
			LoadTransform(filestr, probeTransform);
			scene->GetRenderData()->AddLightProbe(std::make_shared<LocalLightProbe>(LocalLightProbe(scene->GetRenderData(), probeTransform, shape)));
		}
		else
		{
			filestr >> path;
			scene->GetRenderData()->AddLightProbe(LightProbeLoader::LightProbeFromFile(scene->GetRenderData(), path));
		}
	}
}

void EngineDataLoader::LoadMeshFromAi(Mesh* meshPtr, const aiScene* scene, aiMesh* mesh, std::string directory, bool bLoadMaterial, MaterialLoadingData* matLoadingData, BoneMapping* boneMapping, std::vector<glm::vec3>* vertsPosPtr, std::vector<unsigned int>* indicesPtr, std::vector<Vertex>* verticesPtr)
{
	std::vector <Vertex> vertices;
	std::vector <unsigned int> indices;

	if (!verticesPtr)	verticesPtr = &vertices;
	if (!indicesPtr)	indicesPtr = &indices;

	if (vertsPosPtr) vertsPosPtr->reserve(mesh->mNumVertices);
	verticesPtr->reserve(mesh->mNumVertices);
	indicesPtr->reserve(mesh->mNumFaces * 3);

	bool bNormals = mesh->HasNormals();
	bool bTexCoords = mesh->mTextureCoords[0];
	bool bTangentsBitangents = mesh->HasTangentsAndBitangents();
	
	for (int i = 0; i < static_cast<int>(mesh->mNumVertices); i++)
	{
		Vertex vert;

		aiVector3D pos = mesh->mVertices[i];
		vert.Position.x = pos.x;
		vert.Position.y = pos.y;
		vert.Position.z = pos.z;

		if (vertsPosPtr)
			vertsPosPtr->push_back(vert.Position);

		if (bNormals)
		{
			aiVector3D normal = mesh->mNormals[i];
			vert.Normal.x = normal.x;
			vert.Normal.y = normal.y;
			vert.Normal.z = normal.z;
		}

		if (bTexCoords)
		{
			vert.TexCoord.x = mesh->mTextureCoords[0][i].x;
			vert.TexCoord.y = mesh->mTextureCoords[0][i].y;
		}
		else
			vert.TexCoord = glm::vec2(0.0f);

		if (bTangentsBitangents)
		{
			aiVector3D tangent = mesh->mTangents[i];
			vert.Tangent.x = tangent.x;
			vert.Tangent.y = tangent.y;
			vert.Tangent.z = tangent.z;

			aiVector3D bitangent = mesh->mBitangents[i];
			vert.Bitangent.x = bitangent.x;
			vert.Bitangent.y = bitangent.y;
			vert.Bitangent.z = bitangent.z;
		}

		verticesPtr->push_back(vert);
	}
	
	for (int i = 0; i < static_cast<int>(mesh->mNumFaces); i++)
	{
		aiFace face = mesh->mFaces[i];
		for (unsigned int j = 0; j < face.mNumIndices; j++)
			indicesPtr->push_back(face.mIndices[j]);
	}

	if (verticesPtr->size() < 7)
	{
		for (const auto it : *verticesPtr)
			printVector(it.Position);

		for (const auto it : *indicesPtr)
			std::cout << it << " ";
		std::cout << "\n";
	}

	if (boneMapping)
	{
		for (int i = 0; i < static_cast<int>(mesh->mNumBones); i++)
		{
			aiBone& bone = *mesh->mBones[i];
			unsigned int boneID = boneMapping->GetBoneID(bone.mName.C_Str());

			std::cout << directory;

			if (bone.mNumWeights > 0 && !bone.mWeights)
			{
				std::cerr << directory <<  "ERROR! Number of weights of bone is greater than 0, but weights count is nullptr.\n";
				return;
			}

			for (int j = 0; j < static_cast<int>(bone.mNumWeights); j++)
				vertices[bone.mWeights[j].mVertexId].BoneData.AddWeight(boneID, bone.mWeights[j].mWeight);
		}
	}

	if (meshPtr)
		meshPtr->GenerateVAO(verticesPtr, indicesPtr);

	if (mesh->mMaterialIndex >= 0 && bLoadMaterial)
	{
		aiMaterial* assimpMaterial = scene->mMaterials[mesh->mMaterialIndex];
		if (matLoadingData && meshPtr)
		{
			std::vector<aiMaterial*>* loadedAiMaterialsPtr = &matLoadingData->LoadedAiMaterials;

			for (unsigned int i = 0; i < loadedAiMaterialsPtr->size(); i++)
			{
				if ((*loadedAiMaterialsPtr)[i] == assimpMaterial)
				{
					meshPtr->SetMaterial(matLoadingData->LoadedMaterials[i]);	//MaterialLoadingData::LoadedMaterials and MaterialLoadingData::LoadedAiMaterials are the same size, so the (assimp material -> engine material) indices match too
					return;
				}
			}
		}
		
		aiString materialName;
		if (assimpMaterial->Get(AI_MATKEY_NAME, materialName) != AI_SUCCESS)
		{
			std::cerr << "INFO: A material has no name.\n";
			materialName = "Material";
		}


		Material* material = new Material(materialName.C_Str());	//we name all materials "undefined" by default; their name should be contained in the files
		material->LoadFromAiMaterial(scene, assimpMaterial, directory, matLoadingData);

		if (meshPtr)
			meshPtr->SetMaterial(material);
		
		if (matLoadingData)
		{
			matLoadingData->LoadedMaterials.push_back(material);
			matLoadingData->LoadedAiMaterials.push_back(assimpMaterial);
		}
	}
}

void EngineDataLoader::LoadTransform(std::stringstream& filestr, Transform& transform)
{
	std::string input;

	for (int transformField = 0; transformField < 4; transformField++)
	{
		glm::vec3 value(0.0f);
		bool skipped = false;

		for (int i = 0; i < 3; i++)
		{
			size_t pos = (size_t)filestr.tellg();
			filestr >> input;
			if (input == "skip")
			{
				skipped = true;
				break;
			}

			else if (input == "end")
			{
				filestr.seekg(pos);	//go back to the position before the component's end, so other functions aren't alarmed
				return;
			}

			else if (input == "break")
				return;	//don't go back to the previous position, just break from loading

			value[i] = (float)atof(input.c_str());
		}

		if (!skipped)	//be sure to not override the default value if we skip a field; f.e. scale default value is vec3(1.0), not vec3(0.0)
			transform.Set(transformField, value);
	}
}

void EngineDataLoader::LoadTransform(std::stringstream& filestr, Transform& transform, std::string loadType)
{
	if (loadType == "-")
		return;

	for (int i = 0; i < static_cast<int>(loadType.size()); i++)
	{
		int loadedField;
		switch (loadType[i])
		{
		case 'p': loadedField = 0; break;
		case 'r': loadedField = 1; break;
		case 's': loadedField = 2; break;
		case 'f': loadedField = 3; break;
		default:
			std::cerr << "ERROR! Field " << loadType[i] << " is not known.\n";
			return;
		}

		glm::vec3 data;
		for (int j = 0; j < 3; j++)
			filestr >> data[j];

		transform.Set(loadedField, data);
	}
}

MeshSystem::MeshTree* EngineDataLoader::LoadCustomMeshTree(GameManager* gameHandle, std::stringstream& filestr, bool loadPath)
{
	std::string treeName;
	filestr >> treeName;

	MeshSystem::MeshTree* tree = ((loadPath) ? (LoadMeshTree(gameHandle, gameHandle->GetRenderEngineHandle(), treeName)) : (gameHandle->GetRenderEngineHandle()->CreateMeshTree(treeName)));
	LoadCustomMeshNode(gameHandle, filestr, &tree->GetRoot(), (loadPath) ? (tree) : (nullptr));

	return nullptr;
}

void EngineDataLoader::LoadCustomMeshNode(GameManager* gameHandle, std::stringstream& filestr, MeshSystem::TemplateNode* parent, MeshSystem::MeshTree* treeToEdit)
{
	std::string input;
	bool bCreateNodes = (treeToEdit) ? (false) : (true);

	while (input != "end" && !isNextWordEqual(filestr, "end"))
	{
		filestr >> input;	//get node path

		std::string treeName, nodeName;
		if (bCreateNodes)
		{
			size_t nodeNamePos = input.find_first_of(':');
			if (nodeNamePos == std::string::npos)
			{
				std::cerr << "ERROR! No node name in node path. String data: " << input + "\n";
				return;
			}
			treeName = input.substr(0, nodeNamePos);
			nodeName = input.substr(nodeNamePos + 1);

			if (PrimitiveDebugger::bDebugMeshTrees)
				std::cout << "Laduje " << treeName << ", a w nim " << nodeName << ".\n";
		}
		else
		{
			nodeName = input;
			if (PrimitiveDebugger::bDebugMeshTrees)
				std::cout << "Edytuje " << treeToEdit->GetPath() << ", a w nim " << nodeName << ".\n";
		}

		if (bCreateNodes)
			treeToEdit = LoadMeshTree(gameHandle, gameHandle->GetRenderEngineHandle(), treeName);
		MeshSystem::TemplateNode* foundNode = treeToEdit->FindNode(nodeName);
		if (!foundNode)
		{
			std::cerr << "ERROR! Can't load " << input << ".\n";
			return;
		}
		if (PrimitiveDebugger::bDebugMeshTrees)
			std::cout << "Znalazlem node " << foundNode->GetName() << ".\n";

		MeshSystem::TemplateNode& targetNode = (bCreateNodes) ? (parent->AddChild(*foundNode)) : (*foundNode);
		MeshSystem::MeshNode* meshNodeCast = dynamic_cast<MeshSystem::MeshNode*>(&targetNode);

		while (input != ":" && input != "," && input != "end")
		{
			filestr >> input;
			
			if (input == "material" && meshNodeCast)
			{
				input = multipleWordInput(filestr);	//get material name (could be a path, so multiple word input is possible)
				Material* foundMaterial = gameHandle->GetRenderEngineHandle()->FindMaterial(input);

				if (!foundMaterial) //if no material of this name was found, check if the input is of format: file.obj:name
				{
					size_t separatorPos = input.find(':');
					if (separatorPos == std::string::npos)
					{
						std::cerr << "ERROR! Can't load a material from passed input: " + input + ".\n";
						continue;
					}

					foundMaterial = LoadMeshTree(gameHandle, gameHandle->GetRenderEngineHandle(), input.substr(0, separatorPos))->FindMaterial(input.substr(separatorPos + 1));
					std::cout << "Found material " << foundMaterial->GetName() << ". The previous error message is not an error lol.\n";
				}
				meshNodeCast->SetOverrideMaterial(foundMaterial);
			}

			else if (input == "transform")
			{
				Transform t;
				LoadTransform(filestr, t);
				targetNode.SetTemplateTransform(t);
			}

			else if (input == "col")
				targetNode.SetCollisionObject(LoadCollisionObject(gameHandle->GetPhysicsHandle(), filestr));
		}

		if (input == ":")
			LoadCustomMeshNode(gameHandle, filestr, &targetNode, (bCreateNodes) ? (nullptr) : (treeToEdit));
	}
}

void EngineDataLoader::LoadMeshNodeFromAi(GameManager* gameHandle, const aiScene* scene, std::string directory, MaterialLoadingData* matLoadingData, MeshSystem::TemplateNode& meshSystemNode, aiNode* node, BoneMapping& boneMapping, aiBone* bone)
{
	for (unsigned int i = 0; i < node->mNumMeshes; i++)
	{
		aiMesh* assimpMesh = scene->mMeshes[node->mMeshes[i]];
		Mesh& mesh = dynamic_cast<MeshSystem::MeshNode*>(&meshSystemNode)->AddMesh(node->mName.C_Str());

		if (mesh.GetName() == "Barrel_Release_001")
			std::cout << "Bardzo wazny komunikat.\n";
		LoadMeshFromAi(&mesh, scene, assimpMesh, directory, true, matLoadingData, &boneMapping);
	}

	aiMatrix4x4 nodeMatrix = node->mTransformation;

	if (bone)
	{
		dynamic_cast<MeshSystem::BoneNode*>(&meshSystemNode)->SetBoneOffset(toGlm(bone->mOffsetMatrix));
		dynamic_cast<MeshSystem::BoneNode*>(&meshSystemNode)->SetBoneID(boneMapping.GetBoneID(bone->mName.C_Str()));
	}

	meshSystemNode.SetTemplateTransform(decompose(toGlm(nodeMatrix)));
	//meshSystemNode.GetTemplateTransform().Print(node->mName.C_Str());

	for (unsigned int i = 0; i < node->mNumChildren; i++)
	{	
		if (node->mChildren[i]->mNumMeshes > 0)
		{
			std::string meshName = node->mChildren[i]->mName.C_Str();	//retrieve one of the children's meshes name to try and find out whether this child node only contains collision meshes. We do not want to render them so we leave them out of the meshtree

			if (meshName.length() > 4 && meshName.substr(0, 4) == "GEEC")
				std::cout << "found GEEC: " << meshName << "\n";
			if (meshName.length() > 4 && meshName.substr(0, 4) == "GEEC")	//We are dealing with a collision node!
				for (int j = 0; j < static_cast<int>(node->mChildren[i]->mNumMeshes); j++)
					meshSystemNode.AddCollisionShape(LoadTriangleMeshCollisionShape(gameHandle->GetPhysicsHandle(), scene, *scene->mMeshes[node->mChildren[i]->mMeshes[j]]));
			else
				LoadMeshNodeFromAi(gameHandle, scene, directory, matLoadingData, *meshSystemNode.AddChild<MeshSystem::MeshNode>(node->mChildren[i]->mName.C_Str()), node->mChildren[i], boneMapping);
		}
		else if (aiBone* bone = CastAiNodeToBone(scene, node->mChildren[i])) // (isBone)
			LoadMeshNodeFromAi(gameHandle, scene, directory, matLoadingData, *meshSystemNode.AddChild<MeshSystem::BoneNode>(node->mChildren[i]->mName.C_Str()), node->mChildren[i], boneMapping, bone);
		else
			LoadMeshNodeFromAi(gameHandle, scene, directory, matLoadingData, *meshSystemNode.AddChild<MeshSystem::TemplateNode>(node->mChildren[i]->mName.C_Str()), node->mChildren[i], boneMapping);
	}
}

void EngineDataLoader::LoadComponentsFromMeshTree(Component* comp, const MeshSystem::MeshTree& tree, const MeshSystem::TemplateNode* node, SkeletonInfo& skeletonInfo, Material* overrideMaterial)
{
	const MeshSystem::MeshNode* meshNodeCast = dynamic_cast<const MeshSystem::MeshNode*>(node);
	ModelComponent* modelCast = dynamic_cast<ModelComponent*>(comp);
	const MeshSystem::BoneNode* boneNodeCast = dynamic_cast<const MeshSystem::BoneNode*>(node);
	BoneComponent* boneCast = dynamic_cast<BoneComponent*>(comp);

	if (PrimitiveDebugger::bDebugMeshTrees)
	{
		if (meshNodeCast)
		{
			std::cout << comp->GetName() << ":::::" << meshNodeCast->GetMeshCount() << "\n";
		}
		else
			std::cout << comp->GetName() << "\n";
	}

	if (meshNodeCast && meshNodeCast->GetMeshCount() > 0 && !modelCast)
	{
		std::cerr << "ERROR! Component hierarchy not aligned with mesh hierarchy.\n";
		return;
	}

	comp->GenerateFromNode(node, overrideMaterial);

	//printVector(modelCast->GetTransform().ScaleRef, modelCast->GetName());

	for (int i = 0; i < node->GetChildCount(); i++)	//WAZNE!!!! zmien meshNodeCast na node jak naprawisz childow
	{
		Component* child = nullptr;
		
		if (dynamic_cast<const MeshSystem::MeshNode*>(node->GetChild(i)))
		{
			ModelComponent* model = ModelComponent::Of(ModelComponent(comp->GetScene(), node->GetChild(i)->GetName(), Transform(), &skeletonInfo)).get();
			model->SetSkeletonInfo(&skeletonInfo);
			child = model;
		}
		else if (dynamic_cast<const MeshSystem::BoneNode*>(node->GetChild(i)))
		{
			BoneComponent* bone = new BoneComponent(comp->GetScene(), node->GetChild(i)->GetName(), Transform(), tree.GetBoneMapping()->GetBoneID(node->GetChild(i)->GetName()));
			skeletonInfo.AddBone(*bone);
			child = bone;
		}
		else
			child = new Component(comp->GetScene(), node->GetChild(i)->GetName(), Transform());
		comp->AddComponent(child);
		LoadComponentsFromMeshTree(child, tree, node->GetChild(i), skeletonInfo, overrideMaterial);
	}
}

std::unique_ptr<GameScene> EngineDataLoader::LoadSceneFromFile(GameManager* gameHandle, std::string path)
{
	std::unique_ptr<GameScene> scene = std::make_unique<GameScene>(GameScene(gameHandle));
	std::ifstream file(path);
	std::stringstream filestr;

	filestr << file.rdbuf();

	std::string type;
	std::shared_ptr<Actor> currentActor = nullptr;

	while (filestr >> type)
	{
		if (type == "newactor")
		{
			std::string actorName, typeName;
			Actor* parent = nullptr;

			filestr >> typeName;
			actorName = multipleWordInput(filestr);
			if (isNextWordEqual(filestr, "child"))
			{
				std::string parentName = multipleWordInput(filestr);
				parent = scene->FindActor(parentName);

				if (!parent)
					std::cerr << "ERROR! Can't find actor " + parentName + ", parent of " + actorName + " will be assigned automatically.\n";
			}

			if (typeName == "GunActor")
				currentActor = std::make_shared<GunActor>(GunActor(scene.get(), actorName));
			else if (typeName == "Actor")
				currentActor = std::make_shared<Actor>(Actor(scene.get(), actorName));
			else
			{
				std::cerr << "ERROR! Unrecognized actor type " << typeName << ".\n";
				continue;
			}

			if (parent)
				parent->AddChild(currentActor);
			else
				scene->AddActorToRoot(currentActor);
		}

		else if (type == "newcomp")
		{
			if (!currentActor)
			{
				std::cerr << "ERROR! Component defined without an actor\n";
				break;
			}
			LoadComponentData(gameHandle, filestr, currentActor.get(), scene.get());
		}

		else if (type == "newtree")
			LoadCustomMeshTree(gameHandle, filestr);

		else if (type == "edittree")
			LoadCustomMeshTree(gameHandle, filestr, true);

		else if (type == "newprobes")
			LoadLightProbes(scene.get(), filestr);

		else if (type == "materialsfile")
		{
			std::string path, directory;
			filestr >> path;

			directory = extractDirectory(path);

			LoadMaterials(gameHandle->GetRenderEngineHandle(), path, directory);
		}

		else if (type == "actorinfo" && currentActor)
		{
			std::stringstream* stream = new std::stringstream;
			std::string data;

			while (data != "end")
			{
				filestr >> data;
				(*stream) << data;
			}

			currentActor->SetSetupStream(stream);
		}
	}
	std::cout << "Level loading finished.\n";

	return scene;
}

void EngineDataLoader::LoadModel(std::string path, Component* comp, MeshTreeInstancingType type, Material* overrideMaterial)
{
	InstantiateTree(comp, *LoadMeshTree(comp->GetScene()->GetGameHandle(), comp->GetScene()->GetGameHandle()->GetRenderEngineHandle(), path), type, overrideMaterial);
}

MeshSystem::MeshTree* EngineDataLoader::LoadMeshTree(GameManager* gameHandle, RenderEngineManager* renderHandle, std::string path, MeshSystem::MeshTree* treePtr)
{
	if (path.empty())
	{
		if (!treePtr || treePtr->GetPath().empty())
			return nullptr;

		path = treePtr->GetPath();
	}

	if (MeshSystem::MeshTree* found = renderHandle->FindMeshTree(path, treePtr))
	{
		if (PrimitiveDebugger::bDebugMeshTrees)
			std::cout << "Found " << path << ".\n";
		return found;
	}
	if (!treePtr)
		treePtr = renderHandle->CreateMeshTree(path);

	Assimp::Importer importer;
	const aiScene* scene;
	MaterialLoadingData matLoadingData;

	scene = importer.ReadFile(path, aiProcess_OptimizeMeshes | aiProcess_SplitLargeMeshes | aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenNormals | aiProcess_CalcTangentSpace);
	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
	{
		std::cerr << "Can't load mesh scene " << path << ".\n";
		return nullptr;
	}

	std::string directory = extractDirectory(path);
	std::vector<ModelComponent*> modelsPtr;

	LoadMeshNodeFromAi(gameHandle, scene, directory, &matLoadingData, treePtr->GetRoot(), scene->mRootNode, *treePtr->GetBoneMapping());

	if (scene->mNumAnimations > 0)
	{
		treePtr->animation = std::make_shared<Animation>(Animation(scene->mAnimations[0]));
		std::cout << scene->mAnimations[0]->mDuration / scene->mAnimations[0]->mTicksPerSecond << "<- czas; " << scene->mAnimations[0]->mTicksPerSecond << "<- tps\n";
	}
	else
		treePtr->animation = nullptr;

	for (int j = 0; j < static_cast<int>(matLoadingData.LoadedMaterials.size()); j++)
		renderHandle->AddMaterial(matLoadingData.LoadedMaterials[j]);

	for (unsigned int i = 0; i < matLoadingData.LoadedMaterials.size(); i++)
		matLoadingData.LoadedMaterials[i]->SetRenderShaderName("Geometry");

	return treePtr;
}

void EngineDataLoader::InstantiateTree(Component* comp, MeshSystem::MeshTree& tree, MeshTreeInstancingType type, Material* overrideMaterial)
{
	switch (type)
	{
	case MeshTreeInstancingType::ROOTTREE:
	{
		SkeletonInfo& skelInfo = *comp->GetScene()->GetRenderData()->AddSkeletonInfo();
		LoadComponentsFromMeshTree(comp, tree, &tree.GetRoot(), skelInfo, overrideMaterial);
		skelInfo.SetGlobalInverseTransformPtr(&comp->GetTransform());
		skelInfo.SortBones();
		if (tree.animation)
		{
			comp->QueueAnimationAll(tree.animation.get());
			if (DUPA::AnimTime == 9999.0f)
				DUPA::AnimTime = 0.0f;
		}
		break;
	}
	case MeshTreeInstancingType::MERGE:
	{
		ModelComponent* mdl = dynamic_cast<ModelComponent*>(comp);
		int i = 0;

		if (!mdl)
		{
			std::cerr << "ERROR! Can't merge meshes to a non-ModelComponent object.\n";
			break;
		}

		while (MeshSystem::MeshNode* node = dynamic_cast<MeshSystem::MeshNode*>(tree.FindNode(i++)))
			mdl->GenerateFromNode(node);
		break;
	}
	}
}

std::shared_ptr<Font> EngineDataLoader::LoadFont(std::string path)
{
	FT_Library ft;
	if (FT_Init_FreeType(&ft))
	{
		std::cerr << "ERROR! Cannot init freetype library.\n";
		return nullptr;
	}

	FT_Face face;
	if (FT_New_Face(ft, path.c_str(), 0, &face))
	{
		std::cerr << "ERROR! Cannot load font " + path + ".\n";
		return nullptr;
	}

	FT_Set_Pixel_Sizes(face, 0, 48);

	std::shared_ptr<Font> font = std::make_shared<Font>(Font());
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	font->SetBaselineHeight(face->ascender / 64.0f);

	for (int i = 0; i < 128; i++)
	{
		if (FT_Load_Char(face, i, FT_LOAD_RENDER))
		{
			std::cerr << "Can't load glyph " << char(i) << ".\n";
			continue;
		}

		font->GetBitmapsArray().Bind(0);
		glTexSubImage3D(font->GetBitmapsArray().GetType(), 0, 0, 0, i, face->glyph->bitmap.width, face->glyph->bitmap.rows, 1, GL_RED, GL_UNSIGNED_BYTE, face->glyph->bitmap.buffer);
		Character character;
		character.ID = i;
		character.Size = glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows);
		character.Bearing = glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top);
		character.Advance = face->glyph->advance.x;

		font->AddCharacter(character);
	}

	glGenerateMipmap(font->GetBitmapsArray().GetType());

	return font;
}

template <class T> T EngineDataLoader::LoadSettingsFromFile(std::string path)
{
	T settings;
	std::fstream file;	//wczytaj plik inicjalizujacy
	file.open(path);
	std::stringstream filestr;
	filestr << file.rdbuf();

	std::string settingName;

	while (filestr >> settingName)	//wczytuj kolejne wyrazy w pliku inicjalzujacym
	{								//jesli napotkasz na wyraz, ktory sygnalizuje rodzaj danych to wczytaj te dane (sposob jest rozny w przypadku roznych danych)
		settings.LoadSetting(filestr, settingName);
	}

	return settings;
}

aiBone* CastAiNodeToBone(const aiScene* scene, aiNode* node, const aiMesh** ownerMesh)
{
	std::string dupa = std::string(node->mName.C_Str());

	for (int i = 0; i < static_cast<int>(scene->mNumMeshes); i++)
	{
		const aiMesh& mesh = *scene->mMeshes[i];

		for (int j = 0; j < static_cast<int>(scene->mMeshes[i]->mNumBones); j++)
		{
			if (mesh.mBones[j]->mName == node->mName)
			{
				if (ownerMesh)
					*ownerMesh = &mesh;

				return mesh.mBones[j];
			}
		}
	}

	return nullptr;
}

glm::mat4 toGlm(const aiMatrix4x4& aiMat)
{
	return glm::mat4(aiMat.a1, aiMat.b1, aiMat.c1, aiMat.d1,
					 aiMat.a2, aiMat.b2, aiMat.c2, aiMat.d2,
					 aiMat.a3, aiMat.b3, aiMat.c3, aiMat.d3,
				 	 aiMat.a4, aiMat.b4, aiMat.c4, aiMat.d4);
}

template GameSettings EngineDataLoader::LoadSettingsFromFile<GameSettings>(std::string path);
#include "Mesh.h"
#include "GunActor.h"
#include "CameraComponent.h"
#include "LightComponent.h"
#include "MeshSystem.h"
#include "FileLoader.h"
#include <functional>


void EngineDataLoader::LoadMaterials(RenderEngineManager* renderHandle, SearchEngine* searcher, std::string path, std::string directory)
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

		shader = searcher->FindShader(shaderName);

		material->SetRenderShader(shader);
		material->SetShininess(shininess);
		material->SetDepthScale(depthScale);

		renderHandle->AddMaterial(material);

		for (unsigned int i = 0; i < texCount; i++)
		{
			std::string path, shaderUniformName, isSRGB;
			filestr >> path >> shaderUniformName >> isSRGB;

			material->AddTexture(new Texture(directory + path, shaderUniformName, toBool(isSRGB)));
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

		Shader* shader = new Shader(paths[0], paths[1], paths[2]);
		shader->SetName(name);

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

		renderEngHandle->AddExternalShader(shader);
	}
}

void EngineDataLoader::LoadComponentData(GameManager* gameHandle, std::stringstream& filestr, Actor* currentActor)
{
	///////////////////////////////1. Load the type and the name of the current component
	SearchEngine* searcher = gameHandle->GetSearchEngine();
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

		modelPtr = renderEngHandle->CreateModel(ModelComponent(gameHandle, name)).get();

		MeshSystem::MeshTree* tree = LoadMeshTree(gameHandle, renderEngHandle, searcher, path);
		std::unique_ptr<MeshSystem::MeshTree> tempTreeCopyForEdit = nullptr;
		if (isNextWordEqual(filestr, "edit"))
		{
			tempTreeCopyForEdit = std::make_unique<MeshSystem::MeshTree>(MeshSystem::MeshTree(*tree, "temp"));
			tree = tempTreeCopyForEdit.get();
			LoadCustomMeshNode(gameHandle, filestr, nullptr, tree);
		}
		InstantiateTree(gameHandle, modelPtr, *tree, instancingType, searcher->FindMaterial(materialName));

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

		LightComponent* lightPtr = new LightComponent(gameHandle, name, toLightType(lightType), gameHandle->GetAvailableLightIndex(), 10.0f, projection);
		for (unsigned int vector = 0; vector < 3; vector++)
			for (unsigned int axis = 0; axis < 3; axis++)
				filestr >> (*lightPtr)[vector][axis];	//TODO: sprawdzic czy sie wywala

		glm::vec3 additionalData(0.0f);
		for (unsigned int i = 0; i < 3; i++)
			filestr >> additionalData[i];
		lightPtr->SetAdditionalData(additionalData);

		lightPtr->CalculateLightRadius();

		gameHandle->AddLightToScene(lightPtr);
		comp = lightPtr;
	}
	else if (type == "camera")
	{
		float speedPerSec;
		filestr >> speedPerSec;

		comp = new CameraComponent(gameHandle, name, speedPerSec);

		if (isNextWordEqual(filestr, "active"))
			gameHandle->BindActiveCamera(dynamic_cast<CameraComponent*>(comp));
	}
	else if (type == "soundsource")
	{
		SoundSourceComponent* sourcePtr = nullptr;
		std::string path;
		filestr >> path;
		sourcePtr = gameHandle->GetAudioEngineHandle()->AddSource(path, name);

		comp = sourcePtr;
	}
	else if (type == "empty")
		comp = new Component(gameHandle, name, Transform());
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

std::unique_ptr<CollisionObject> EngineDataLoader::LoadCollisionObject(std::string path, PhysicsEngineManager* physicsHandle, std::stringstream& filestr)
{
	std::unique_ptr<CollisionObject> obj = std::make_unique<CollisionObject>();
	obj->Name = path;

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

		CollisionShape* shape = obj->AddShape(static_cast<CollisionShapeType>(shapeType));

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

			for (int j = 0; j < static_cast<int>(scene->mNumMeshes); j++)
				LoadMeshFromAi(nullptr, scene, scene->mMeshes[j], extractDirectory(path), false, nullptr, &shape->VertData, &shape->IndicesData);
		}

		LoadTransform(filestr, shape->ShapeTransform, transformLoadType);
	}

	return obj;
}

void EngineDataLoader::LoadMeshFromAi(Mesh* meshPtr, const aiScene* scene, aiMesh* mesh, std::string directory, bool bLoadMaterial, MaterialLoadingData* matLoadingData, std::vector<glm::vec3>* vertsPosPtr, std::vector<unsigned int>* indicesPtr, std::vector<Vertex>* verticesPtr)
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

	for (unsigned int i = 0; i < mesh->mNumVertices; i++)
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

	for (unsigned int i = 0; i < mesh->mNumFaces; i++)
	{
		aiFace face = mesh->mFaces[i];
		for (unsigned int j = 0; j < face.mNumIndices; j++)
			indicesPtr->push_back(face.mIndices[j]);
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
			std::cerr << "INFO: A material has no name.\n";

		Material* material = new Material(materialName.C_Str());	//we name all materials "undefined" by default; their name should be contained in the files
		material->LoadFromAiMaterial(assimpMaterial, directory, matLoadingData);

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


void EngineDataLoader::LoadLevelFile(GameManager* gameHandle, std::string path)
{
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
				parent = gameHandle->GetSearchEngine()->FindActor(parentName);

				if (!parent)
					std::cerr << "ERROR! Can't find actor " + parentName + ", parent of " + actorName + " will be assigned automatically.\n";
			}

			if (typeName == "GunActor")
				currentActor = std::make_shared<GunActor>(gameHandle, actorName);
			else if (typeName == "Actor")
				currentActor = std::make_shared<Actor>(gameHandle, actorName);
			else
			{
				std::cerr << "ERROR! Unrecognized actor type " << typeName << ".\n";
				continue;
			}

			if (parent)
				parent->AddChild(currentActor);
			else
				gameHandle->AddActorToScene(currentActor);
		}

		else if (type == "newcomp")
		{
			if (!currentActor)
			{
				std::cerr << "ERROR! Component defined without an actor\n";
				break;
			}
			LoadComponentData(gameHandle, filestr, currentActor.get());
		}

		else if (type == "newtree")
			LoadCustomMeshTree(gameHandle, filestr);
		
		else if (type == "edittree")
			LoadCustomMeshTree(gameHandle, filestr, true);

		else if (type == "materialsfile")
		{
			std::string path, directory;
			filestr >> path;

			directory = extractDirectory(path);

			LoadMaterials(gameHandle->GetRenderEngineHandle(), gameHandle->GetSearchEngine(), path, directory);
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
}

MeshSystem::MeshTree* EngineDataLoader::LoadMeshTree(GameManager* gameHandle, RenderEngineManager* renderHandle, SearchEngine* searcher, std::string path, MeshSystem::MeshTree* treePtr)
{
	if (path.empty())
	{
		if (!treePtr || treePtr->GetFilePath().empty())
			return nullptr;

		path = treePtr->GetFilePath();
	}

	if (MeshSystem::MeshTree* found = renderHandle->FindMeshTree(path, treePtr))
	{
		std::cout << "Found " << path << ".\n";
		return found;
	}
	if (!treePtr)
		treePtr = renderHandle->CreateMeshTree(path);

	Assimp::Importer importer;
	const aiScene* scene;
	MaterialLoadingData matLoadingData;

	scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenNormals | aiProcess_CalcTangentSpace);
	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
	{
		std::cerr << "Can't load mesh scene " << path << ".\n";
		return nullptr;
	}

	std::string directory = extractDirectory(path);
	std::vector<ModelComponent*> modelsPtr;

	LoadMeshNodeFromAi(gameHandle, scene, directory, scene->mRootNode, &matLoadingData, treePtr->GetRoot());

	for (int j = 0; j < static_cast<int>(matLoadingData.LoadedMaterials.size()); j++)
		renderHandle->AddMaterial(matLoadingData.LoadedMaterials[j]);



	Shader* defaultShader = searcher->FindShader("deferred");
	for (unsigned int i = 0; i < matLoadingData.LoadedMaterials.size(); i++)
		matLoadingData.LoadedMaterials[i]->SetRenderShader(defaultShader);

	return treePtr;
}

MeshSystem::MeshTree* EngineDataLoader::LoadCustomMeshTree(GameManager* gameHandle, std::stringstream& filestr, bool loadPath)
{
	std::string treeName;
	filestr >> treeName;

	MeshSystem::MeshTree* tree = (loadPath) ? (LoadMeshTree(gameHandle, gameHandle->GetRenderEngineHandle(), gameHandle->GetSearchEngine(), treeName)) : (gameHandle->GetRenderEngineHandle()->CreateMeshTree(treeName));

	LoadCustomMeshNode(gameHandle, filestr, &tree->GetRoot(), (loadPath) ? (gameHandle->GetRenderEngineHandle()->FindMeshTree(treeName)) : (nullptr));

	return nullptr;
}

void EngineDataLoader::LoadCustomMeshNode(GameManager* gameHandle, std::stringstream& filestr, MeshSystem::MeshNode* parent, MeshSystem::MeshTree* treeToEdit)
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

			std::cout << "Laduje " << treeName << ", a w nim " << nodeName << ".\n";
		}
		else
		{
			nodeName = input;
			std::cout << "Edytuje " << treeToEdit->GetFilePath() << ", a w nim " << nodeName << ".\n";
		}

		if (bCreateNodes)
			treeToEdit = LoadMeshTree(gameHandle, gameHandle->GetRenderEngineHandle(), gameHandle->GetSearchEngine(), treeName);
		MeshSystem::MeshNode* foundNode = treeToEdit->FindNode(nodeName);
		if (!foundNode)
		{
			std::cerr << "ERROR! Can't load " << input << ".\n";
			return;
		}
		std::cout << "Znalazlem node " << foundNode->GetName() << ".\n";

		MeshSystem::MeshNode& copiedNode = (bCreateNodes) ? (parent->AddChild(*foundNode)) : (*foundNode);

		while (input != ":" && input != "," && input != "end")
		{
			filestr >> input;
			
			if (input == "material")
			{
				input = multipleWordInput(filestr);	//get material name (could be a path, so multiple word input is possible)
				Material* foundMaterial = gameHandle->GetSearchEngine()->FindMaterial(input);

				if (!foundMaterial) //if no material of this name was found, check if the input is of format: file.obj:name
				{
					size_t separatorPos = input.find(':');
					if (separatorPos == std::string::npos)
					{
						std::cerr << "ERROR! Can't load a material from passed input: " + input + ".\n";
						continue;
					}

					foundMaterial = LoadMeshTree(gameHandle, gameHandle->GetRenderEngineHandle(), gameHandle->GetSearchEngine(), input.substr(0, separatorPos))->FindMaterial(input.substr(separatorPos + 1));
				}
				copiedNode.SetOverrideMaterial(foundMaterial);
			}

			else if (input == "transform")
			{
				Transform t;
				LoadTransform(filestr, t);
				copiedNode.SetTemplateTransform(t);
			}

			else if (input == "col")
				copiedNode.SetCollisionObject(LoadCollisionObject("", gameHandle->GetPhysicsHandle(), filestr));
		}

		if (input == ":")
			LoadCustomMeshNode(gameHandle, filestr, &copiedNode, (bCreateNodes) ? (nullptr) : (treeToEdit));
	}
}

void EngineDataLoader::LoadMeshNodeFromAi(GameManager* gameHandle,const aiScene* scene, std::string directory, aiNode* node, MaterialLoadingData* matLoadingData, MeshSystem::MeshNode& meshSystemNode)
{
	for (unsigned int i = 0; i < node->mNumMeshes; i++)
	{
		aiMesh* assimpMesh = scene->mMeshes[node->mMeshes[i]];
		Mesh& mesh = meshSystemNode.AddMesh(node->mName.C_Str());

		LoadMeshFromAi(&mesh, scene, assimpMesh, directory, true, matLoadingData);
	}

	for (unsigned int i = 0; i < node->mNumChildren; i++)
		LoadMeshNodeFromAi(gameHandle, scene, directory, node->mChildren[i], matLoadingData, meshSystemNode.AddChild(node->mChildren[i]->mName.C_Str()));
}

void EngineDataLoader::LoadComponentsFromMeshTree(GameManager* gameHandle, Component* comp, const MeshSystem::MeshNode& node, Material* overrideMaterial)
{
	std::cout << comp->GetName() << ":::::" << node.GetMeshCount() << "\n";
	ModelComponent* modelCast = dynamic_cast<ModelComponent*>(comp);
	if (node.GetMeshCount() > 0 && !modelCast)
	{
		std::cerr << "ERROR! Component hierarchy not aligned with mesh hierarchy.\n";
		return;
	}

	if (modelCast)
		modelCast->GenerateFromNode(node, overrideMaterial);

	//printVector(modelCast->GetTransform().ScaleRef, modelCast->GetName());

	for (int i = 0; i < node.GetChildCount(); i++)
	{
		ModelComponent* child = gameHandle->GetRenderEngineHandle()->CreateModel(ModelComponent(gameHandle, node.GetChild(i)->GetName())).get();
		comp->AddComponent(child);
		LoadComponentsFromMeshTree(gameHandle, child, *node.GetChild(i), overrideMaterial);
	}
}

void EngineDataLoader::InstantiateTree(GameManager* gameHandle, Component* comp, MeshSystem::MeshTree& tree, MeshTreeInstancingType type, Material* overrideMaterial)
{
	switch (type)
	{
		case MeshTreeInstancingType::ROOTTREE:
		{
			LoadComponentsFromMeshTree(gameHandle, comp, tree.GetRoot(), overrideMaterial);
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

			while (MeshSystem::MeshNode* node = tree.FindNode(i++))
				mdl->GenerateFromNode(*node);
			break;
		}
	}
}

void EngineDataLoader::LoadModel(GameManager* gameHandle, std::string path, Component* comp, MeshTreeInstancingType type, Material* overrideMaterial)
{
	InstantiateTree(gameHandle, comp, *LoadMeshTree(gameHandle, gameHandle->GetRenderEngineHandle(), gameHandle->GetSearchEngine(), path), type, overrideMaterial);
}

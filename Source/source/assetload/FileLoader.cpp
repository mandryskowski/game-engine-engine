#define CEREAL_LOAD_FUNCTION_NAME Load
#define CEREAL_SAVE_FUNCTION_NAME Save
#define CEREAL_SERIALIZE_FUNCTION_NAME Serialize
#include <rendering/Mesh.h>
#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <scene/GunActor.h>
#include <scene/CameraComponent.h>
#include <scene/BoneComponent.h>
#include <animation/SkeletonInfo.h>
#include <scene/LightComponent.h>
#include <scene/LightProbeComponent.h>
#include <scene/hierarchy/HierarchyTree.h>
#include <scene/hierarchy/HierarchyNode.h>
#include <assetload/FileLoader.h>
#include <rendering/Texture.h>
#include <rendering/LightProbe.h>
#include <scene/SoundSourceComponent.h>
#include <scene/TextComponent.h>
#include <UI/Font.h>
#include <scene/Controller.h>
#include <game/GameScene.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <functional>
#include <fstream>

#include <animation/AnimationManagerComponent.h>

namespace GEE
{
	FT_Library* EngineDataLoader::FTLib = nullptr;


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
			SharedPtr<Material> material = nullptr;
			std::string materialName, shaderName;
			float shininess, depthScale;
			Shader* shader = nullptr;
			unsigned int texCount;

			filestr >> materialName;

			if (isNextWordEqual(filestr, "atlas"))
			{
				Vec2f size(0.0f);
				filestr >> size.x >> size.y;
				material = MakeShared<AtlasMaterial>(materialName, size);
			}
			else
				material = MakeShared<Material>(materialName);

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
				material->AddTexture(MakeShared<NamedTexture>(Texture::Loader<>::FromFile2D(path, (toBool(isSRGB)) ? (Texture::Format::SRGBA()) : (Texture::Format::RGBA()), false, Texture::MinFilter::Trilinear(), Texture::MagFilter::Bilinear()), shaderUniformName));
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

			SharedPtr<Shader> shader = ShaderLoader::LoadShaders(name, paths[0], paths[1], paths[2]);

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

	void EngineDataLoader::LoadComponentData(GameManager* gameHandle, std::stringstream& filestr, Actor* currentActor, GameScene& scene)
	{
		///////////////////////////////1. Load the type and the name of the current component
		RenderEngineManager* renderEngHandle = gameHandle->GetRenderEngineHandle();

		Component* comp = nullptr;
		std::string type, name;
		filestr >> type;
		name = multipleWordInput(filestr);

		///////////////////////////////2. Load info about its position in hierarchy
		Component* parent = nullptr;
		bool replaceRoot = false;
		if (isNextWordEqual(filestr, "child"))
		{
			std::string parentName;
			filestr >> parentName;	///skip familyword and load parentname
			if (Component* found = currentActor->GetRoot()->SearchForComponent(parentName))
				parent = found;
			else
			{
				std::cerr << "ERROR! Can't find target parent " << parentName << "!\n";
				parent = currentActor->GetRoot();
			}
		}
		else
		{
			parent = currentActor->GetRoot();
			if (isNextWordEqual(filestr, "root")) //Note: we set the parent as root for now, we will replace the root with this component after we create it. The design of this function requires this from us since we create components from their parents.
				replaceRoot = true;
		}

		///////////////////////////////3. Load component's data

		if (type == "model")
		{
			std::string path;

			path = multipleWordInput(filestr);

			ModelComponent& model = parent->CreateComponent<ModelComponent>(name, Transform());

			HierarchyTemplate::HierarchyTreeT* tree = LoadHierarchyTree(scene, path);
			if (tree)
			{
				UniquePtr<HierarchyTemplate::HierarchyTreeT> tempTreeCopyForEdit = nullptr;
				if (isNextWordEqual(filestr, "edit"))
				{
					tempTreeCopyForEdit = MakeUnique<HierarchyTemplate::HierarchyTreeT>(*tree);
					tree = tempTreeCopyForEdit.get();
					LoadCustomHierarchyNode(scene, filestr, nullptr, tree);
				}
				std::cout << "tree loading ended\n";
				InstantiateTree(model, *tree);
				model.SetName(name); //override the name, it is changed in InstantiateTree
				std::cout << "instiantiating tree ended\n";
			}

			comp = &model;
		}
		else if (type == "light")
		{
			std::string lightType;
			filestr >> lightType;

			Mat4f projection;
			if (lightType == "point" || lightType == "spot")
				projection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
			else
				projection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, 1.0f, 10.0f);


			LightComponent& light = parent->CreateComponent<LightComponent>(name, toLightType(lightType), scene.GetRenderData()->GetAvailableLightIndex(), scene.GetRenderData()->GetAvailableLightIndex(), 10.0f, projection);
			for (unsigned int vector = 0; vector < 3; vector++)
				for (unsigned int axis = 0; axis < 3; axis++)
					filestr >> (light)[vector][axis];	//TODO: sprawdzic czy sie wywala

			Vec3f additionalData(0.0f);
			for (unsigned int i = 0; i < 3; i++)
				filestr >> additionalData[i];
			light.SetAdditionalData(additionalData);

			light.CalculateLightRadius();
			comp = &light;
		}
		else if (type == "camera")
		{
			float speedPerSec;
			filestr >> speedPerSec;

			CameraComponent& cam = parent->CreateComponent<CameraComponent>(name, glm::perspective(glm::radians(90.0f), (scene.GetGameHandle()->GetGameSettings()->Video.Resolution.x / scene.GetGameHandle()->GetGameSettings()->Video.Resolution.y), 0.01f, 100.0f));
			if (isNextWordEqual(filestr, "active"))
				scene.BindActiveCamera(&cam);
			if (isNextWordEqual(filestr, "automousecontrol"))
			{
				ShootingController& controller = scene.CreateActorAtRoot<ShootingController>("MojTestowyController");
				gameHandle->PassMouseControl(&controller);
			}

			comp = &cam;
		}
		else if (type == "soundsource")
		{
			std::string path;
			filestr >> path;

			Audio::SoundSourceComponent& source = parent->CreateComponent<Audio::SoundSourceComponent>(name, path);
			scene.GetAudioData()->AddSource(source);

			comp = &source;
		}
		else if (type == "text")
		{
			std::string fontPath = multipleWordInput(filestr);
			std::string content = multipleWordInput(filestr);

			comp = &parent->CreateComponent<TextComponent>(name, Transform(), content, fontPath);

		}
		else if (type == "empty")
			comp = &parent->CreateComponent<Component>(name, Transform());
		else
			return;

		///////////////////////////////4. Load its tranformations

		if (isNextWordEqual(filestr, "transform"))
			LoadTransform(filestr, comp->GetTransform());

		///////////////////////////////5. Replace root if the user defined it as the root component
		std::cout << "Component loading ended\n";
		if (replaceRoot)
			currentActor->MoveCompToRoot(*comp);

		if (name == "CameraText")
			dynamic_cast<TextComponent*>(comp)->SetAlignment(TextAlignment::CENTER, TextAlignment::CENTER);

		//////Check for an error

		std::string lastWord;
		filestr >> lastWord;
		if (lastWord != "end")
			std::cerr << "ERROR: There is no ''end'' after component's " << name << " definition! Detected word: " << lastWord << ".\n";
	}

	UniquePtr<Physics::CollisionObject> EngineDataLoader::LoadCollisionObject(GameScene& scene, std::stringstream& filestr)
	{
		UniquePtr<Physics::CollisionObject> obj = MakeUnique<Physics::CollisionObject>();

		if (isNextWordEqual(filestr, "dynamic"))
			obj->IsStatic = false;


		int nrShapes, shapeType;
		std::string transformLoadType;

		filestr >> nrShapes >> transformLoadType;

		for (int i = 0; i < nrShapes; i++)
		{
			filestr >> shapeType;
			if (shapeType < Physics::CollisionShapeType::COLLISION_FIRST || shapeType > Physics::CollisionShapeType::COLLISION_LAST)
			{
				std::cerr << "ERROR! Shape type is " << shapeType << ", which is undefined.\n";
				continue;
			}

			if (shapeType == Physics::CollisionShapeType::COLLISION_TRIANGLE_MESH)
			{
				std::string path;
				filestr >> path;

				//const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate);
				std::cout << "PROBUJE ZALADOWAC STARY COLLISION OBJECT Z PATHA " + path << '\n';
				HierarchyTemplate::HierarchyTreeT* tree = LoadHierarchyTree(scene, path, nullptr, true);

				Transform shapesTransform;
				LoadTransform(filestr, shapesTransform, transformLoadType);

				std::vector<Mesh> meshes = tree->GetMeshes();
				for (Mesh& mesh : meshes)
				{
					Physics::CollisionShape& shape = obj->AddShape(LoadTriangleMeshCollisionShape(scene.GetGameHandle()->GetPhysicsHandle(), mesh));
					shape.ShapeTransform = shapesTransform;	//add new shape and set its transform to the loaded one
				}
			}
			else
			{
				Physics::CollisionShape& shape = obj->AddShape(static_cast<Physics::CollisionShapeType>(shapeType));
				LoadTransform(filestr, shape.ShapeTransform, transformLoadType);
			}
		}

		return obj;
	}

	SharedPtr<Physics::CollisionShape> EngineDataLoader::LoadTriangleMeshCollisionShape(Physics::PhysicsEngineManager* physicsHandle, const Mesh& mesh)
	{
		SharedPtr<Physics::CollisionShape> shape = MakeShared<Physics::CollisionShape>(Physics::CollisionShape(Physics::CollisionShapeType::COLLISION_TRIANGLE_MESH));

		shape->SetOptionalLocalization(mesh.GetLocalization());

		if (!mesh.GetVertsData() || !mesh.GetIndicesData())
		{
			std::cout << "ERROR: Mesh does not contain verts data or indices data. Cannot load triangle mesh collision shape.\n";
			return nullptr;
		}
		std::vector<Vertex>& vertsData = *mesh.GetVertsData();
		std::vector<unsigned int>& indicesData = *mesh.GetIndicesData();

		shape->VertData.resize(vertsData.size());
		std::cout << "Laduje triangle mesh col shape ktory ma " << vertsData.size() << " wierzcholkow i " << mesh.GetIndicesData()->size() << " indexow.\n";

		std::transform(vertsData.begin(), vertsData.end(), shape->VertData.begin(), [](const Vertex& vertex) { return vertex.Position; });
		shape->IndicesData = indicesData;

		return shape;
	}

	SharedPtr<Physics::CollisionShape> EngineDataLoader::LoadTriangleMeshCollisionShape(Physics::PhysicsEngineManager* physicsHandle, const aiScene* scene, aiMesh& mesh)
	{
		return SharedPtr<Physics::CollisionShape>();
	}

	void EngineDataLoader::LoadLightProbes(GameScene& scene, std::stringstream& filestr)
	{
		LightProbeLoader::LoadLightProbeTextureArrays(scene.GetRenderData());
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
				LightProbeComponent& probe = scene.GetRootActor()->CreateComponent<LightProbeComponent>("A LightProbeComponent", shape);
				probe.SetTransform(probeTransform);
			}
			else
			{
				filestr >> path;
				LightProbeComponent& probe = scene.GetRootActor()->CreateComponent<LightProbeComponent>("A LightProbeComponent", EngineBasicShape::QUAD);
				LightProbeLoader::LoadLightProbeFromFile(probe, path);
			}
		}
	}

	void EngineDataLoader::LoadMeshFromAi(Mesh* meshPtr, const aiScene* scene, const aiMesh* mesh, const HTreeObjectLoc& treeObjLoc, const std::string& directory, bool bLoadMaterial, MaterialLoadingData* matLoadingData, BoneMapping* boneMapping, bool keepVertsData)
	{
		std::vector <Vertex> vertices;
		std::vector <unsigned int> indices;

		// For bounding box
		Vec3f minCorner(std::numeric_limits<float>::max()), maxCorner(std::numeric_limits<float>::min());

		vertices.reserve(mesh->mNumVertices);
		indices.reserve(mesh->mNumFaces * 3);

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

			minCorner = glm::min(minCorner, vert.Position);
			maxCorner = glm::max(maxCorner, vert.Position);

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
				vert.TexCoord = Vec2f(0.0f);

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

			vertices.push_back(vert);
		}

		meshPtr->SetBoundingBox(Boxf<Vec3f>::FromMinMaxCorners(minCorner, maxCorner));
		

		for (int i = 0; i < static_cast<int>(mesh->mNumFaces); i++)
		{
			const aiFace& face = mesh->mFaces[i];
			for (unsigned int j = 0; j < face.mNumIndices; j++)
				indices.push_back(face.mIndices[j]);
		}

		if (boneMapping)
		{
			for (int i = 0; i < static_cast<int>(mesh->mNumBones); i++)
			{
				aiBone& bone = *mesh->mBones[i];
				unsigned int boneID = boneMapping->GetBoneID(bone.mName.C_Str());

				//GEE_CORE_ASSERT(boneID < mesh->mNumVertices);

				for (int j = 0; j < static_cast<int>(bone.mNumWeights); j++)
					vertices[bone.mWeights[j].mVertexId].BoneData.AddWeight(boneID, bone.mWeights[j].mWeight);
			}
		}

		if (meshPtr)
		{
			meshPtr->GenerateVAO(vertices, indices, keepVertsData);

			std::cout << meshPtr->GetLocalization().NodeName + "   Vertices:" << vertices.size() << "    Indices:" << indices.size() << "    Bones: " << mesh->mNumBones;
			if (mesh->mNumBones > 0)
			{
				std::cout << " (";
				for (int i = 0; i < mesh->mNumBones; i++)
					std::cout << mesh->mBones[i]->mName.C_Str() << (i == static_cast<int>(mesh->mNumBones) - 1) ? (")") : (", ");

			}
			std::cout << '\n';
		}
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
						meshPtr->SetMaterial(matLoadingData->LoadedMaterials[i].get());	//MaterialLoadingData::LoadedMaterials and MaterialLoadingData::LoadedAiMaterials are the same size, so the (assimp material -> engine material) indices match too
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

			
			SharedPtr<Material> material = nullptr;
			
			std::string materialNameStr = materialName.C_Str();
			if (materialNameStr.size() > 6 && materialNameStr.substr(materialNameStr.size() - 6) == "_atlas")
			{
				material = std::static_pointer_cast<Material>(MakeShared<AtlasMaterial>(Material(Material::MaterialLoc(treeObjLoc, materialNameStr), 0.0f, GameManager::Get().GetRenderEngineHandle()->FindShader("Forward_NoLight")), glm::ivec2(4, 2)));	//TODO: remove this ivec2(4, 2)
			}
			else
				material = MakeShared<Material>(Material::MaterialLoc(treeObjLoc, materialNameStr));	//we name all materials "undefined" by default; their name should be contained in the files

			material->LoadFromAiMaterial(scene, assimpMaterial, directory, matLoadingData);

			if (meshPtr)
				meshPtr->SetMaterial(material.get());

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
			Vec3f value(0.0f);
			bool skipped = false;

			for (int i = 0; i < 3; i++)
			{
				size_t pos = filestr.tellg();
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

			Vec3f data(0.0f);
			for (int j = 0; j < 3; j++)
				filestr >> data[j];

			transform.Set(loadedField, data);
		}
	}

	HierarchyTemplate::HierarchyTreeT* EngineDataLoader::LoadCustomHierarchyTree(GameScene& scene, std::stringstream& filestr, bool loadPath)
	{
		std::string treeName;
		filestr >> treeName;

		HierarchyTemplate::HierarchyTreeT* tree = ((loadPath) ? (LoadHierarchyTree(scene, treeName)) : (&scene.CreateHierarchyTree(treeName)));
		LoadCustomHierarchyNode(scene, filestr, &tree->GetRoot(), (loadPath) ? (tree) : (nullptr));

		return nullptr;
	}

	void EngineDataLoader::LoadCustomHierarchyNode(GameScene& scene, std::stringstream& filestr, HierarchyTemplate::HierarchyNodeBase* parent, HierarchyTemplate::HierarchyTreeT* treeToEdit)
	{
		GameManager& gameHandle = *scene.GetGameHandle();
		std::string input;
		bool bCreateNodes = (treeToEdit) ? (false) : (true);

		while (input != "end" && !isNextWordEqual(filestr, "end"))
		{
			filestr >> input;	//get node path

			std::string nodeName;
			if (bCreateNodes)
			{
				size_t nodeNamePos = input.find_first_of(':');
				if (nodeNamePos == std::string::npos)
				{
					std::cerr << "ERROR! No node name in node path. String data: " << input + "\n";
					return;
				}
				std::string treeName = input.substr(0, nodeNamePos);
				nodeName = input.substr(nodeNamePos + 1);

				treeToEdit = LoadHierarchyTree(scene, treeName);

				if (PrimitiveDebugger::bDebugMeshTrees)
					std::cout << "Laduje " << treeName << ", a w nim " << nodeName << ".\n";
			}
			else
			{
				nodeName = input;
				if (PrimitiveDebugger::bDebugMeshTrees)
					std::cout << "Edytuje " << treeToEdit->GetName() << ", a w nim " << nodeName << ".\n";
			}

			HierarchyTemplate::HierarchyNodeBase* foundNode = treeToEdit->GetRoot().FindNode(nodeName);
			if (!foundNode)
			{
				std::cerr << "ERROR! Can't find " << input << " in tree " << treeToEdit->GetName() << ".\n";
				return;
			}
			if (PrimitiveDebugger::bDebugMeshTrees)
				std::cout << "Znalazlem node " << foundNode->GetCompBaseType().GetName() << ".\n";

			HierarchyTemplate::HierarchyNodeBase& targetNode = (bCreateNodes) ? (parent->AddChild(foundNode->Copy(parent->GetCompBaseType().ActorRef))) : (*foundNode);
			HierarchyTemplate::HierarchyNode<ModelComponent>* modelNodeCast = dynamic_cast<HierarchyTemplate::HierarchyNode<ModelComponent>*>(&targetNode);

			while (input != ":" && input != "," && input != "end")
			{
				filestr >> input;

				if (input == "material" && modelNodeCast)
				{
					input = multipleWordInput(filestr);	//get material name (could be a path, so multiple word input is possible)
					Material* foundMaterial = gameHandle.GetRenderEngineHandle()->FindMaterial(input).get();

					if (!foundMaterial) //if no material of this name was found, check if the input is of format: file.obj:name
					{
						size_t separatorPos = input.find(':');
						if (separatorPos == std::string::npos)
						{
							std::cerr << "ERROR! Can't load a material from passed input: " + input + ".\n";
							continue;
						}

						std::function<Material* (HierarchyTemplate::HierarchyNodeBase&, const std::string&)> findMaterialFunc = [&findMaterialFunc](HierarchyTemplate::HierarchyNodeBase& node, const std::string& materialName) -> Material* {
							if (auto cast = dynamic_cast<HierarchyTemplate::HierarchyNode<ModelComponent>*>(&node))
							{
								std::cout << cast->GetCompT().GetName() << " lolxd " << cast->GetCompT().GetMeshInstanceCount() << "\n";
								for (int i = 0; i < cast->GetCompT().GetMeshInstanceCount(); i++)
								{
									std::cout << "mat " << cast->GetCompT().GetMeshInstance(i).GetMaterialPtr() << '\n';
									if (const Material* material = cast->GetCompT().GetMeshInstance(i).GetMaterialPtr())
									{
										std::cout << material->GetLocalization().Name << " is equal to " << materialName << "?\n";
										if (material->GetLocalization().Name == materialName)
											return const_cast<Material*>(material);
									}
								}
							}

							for (int i = 0; i < static_cast<int>(node.GetChildCount()); i++)
								if (auto found = findMaterialFunc(*node.GetChild(i), materialName))
									return found;

							return nullptr;
						};

						foundMaterial = findMaterialFunc(LoadHierarchyTree(scene, input.substr(0, separatorPos))->GetRoot(), input.substr(separatorPos + 1));

						std::cout << "Found material " << foundMaterial->GetLocalization().Name << ". The previous error message is not an error lol.\n";
					}
					std::cout << "Overriding " << modelNodeCast->GetCompT().GetName() << " material with " << foundMaterial->GetLocalization().Name << '\n';
					modelNodeCast->GetCompT().OverrideInstancesMaterial(foundMaterial);
				}

				else if (input == "transform")
				{
					Transform t;
					LoadTransform(filestr, t);
					targetNode.GetCompBaseType().SetTransform(t);
				}

				else if (input == "col")
					targetNode.SetCollisionObject(LoadCollisionObject(scene, filestr));
			}

			if (input == ":")
				LoadCustomHierarchyNode(scene, filestr, &targetNode, (bCreateNodes) ? (nullptr) : (treeToEdit));
		}
	}

	void EngineDataLoader::LoadHierarchyNodeFromAi(GameManager& gameHandle, const aiScene* assimpScene, const std::string& directory, MaterialLoadingData* matLoadingData, const HTreeObjectLoc& treeObjLoc, HierarchyTemplate::HierarchyNodeBase& hierarchyNode, aiNode* node, BoneMapping& boneMapping, aiBone* bone, const Transform& parentTransform, bool keepVertsData)
	{
		std::cout << "NUM MESHES: " << node->mNumMeshes << '\n';
		for (unsigned int i = 0; i < node->mNumMeshes; i++)
		{
			const aiMesh* assimpMesh = assimpScene->mMeshes[node->mMeshes[i]];
			Mesh* mesh = new Mesh(Mesh::MeshLoc(treeObjLoc, node->mName.C_Str(), assimpScene->mMeshes[node->mMeshes[i]]->mName.C_Str()));

			LoadMeshFromAi(mesh, assimpScene, assimpMesh, treeObjLoc, directory, true, matLoadingData, &boneMapping, keepVertsData);
			dynamic_cast<HierarchyTemplate::HierarchyNode<ModelComponent>*>(&hierarchyNode)->GetCompT().AddMeshInst(*mesh);
		}

		if (bone)
		{
			std::cout << "Found bone " << bone->mName.C_Str() << '\n';
			HierarchyTemplate::HierarchyNode<BoneComponent>* boneNode = dynamic_cast<HierarchyTemplate::HierarchyNode<BoneComponent>*>(&hierarchyNode);
			boneNode->GetCompT().SetBoneOffset(toGlm(bone->mOffsetMatrix));
			boneNode->GetCompT().SetID(boneMapping.GetBoneID(bone->mName.C_Str()));
		}

		aiMatrix4x4 nodeMatrix = node->mTransformation;
		hierarchyNode.GetCompBaseType().SetTransform(decompose(toGlm(nodeMatrix)));
		//meshSystemNode.GetTemplateTransform().Print(node->mName.C_Str());

		for (unsigned int i = 0; i < node->mNumChildren; i++)
		{
			if (node->mChildren[i]->mNumMeshes > 0)
			{
				std::string meshName = node->mChildren[i]->mName.C_Str();	//retrieve one of the children's meshes name to try and find out whether this child node only contains collision meshes. We do not want to render them so we leave them out of the meshtree

				if (meshName.length() > 4 && meshName.substr(0, 4) == "GEEC")	//We are dealing with a collision node!
				{
					std::cout << "found GEEC: " << meshName << "\n";
					for (int j = 0; j < static_cast<int>(node->mChildren[i]->mNumMeshes); j++)
					{
						Mesh mesh(Mesh::MeshLoc(treeObjLoc, meshName, meshName));
						LoadMeshFromAi(&mesh, assimpScene, assimpScene->mMeshes[node->mChildren[i]->mMeshes[j]], treeObjLoc, directory, false, nullptr, nullptr, true);
						SharedPtr<Physics::CollisionShape> shape = LoadTriangleMeshCollisionShape(gameHandle.GetPhysicsHandle(), mesh);


						hierarchyNode.AddCollisionShape(shape);
						shape->GetOptionalLocalization()->OptionalCorrespondingMesh = mesh;

						std::cout << "GEEC: " + hierarchyNode.GetCollisionObject()->Shapes.back()->GetOptionalLocalization()->GetTreeName() + " " + hierarchyNode.GetCollisionObject()->Shapes.back()->GetOptionalLocalization()->ShapeMeshLoc.NodeName + hierarchyNode.GetCollisionObject()->Shapes.back()->GetOptionalLocalization()->ShapeMeshLoc.SpecificName << '\n';
					}
				}
				else
					LoadHierarchyNodeFromAi(gameHandle, assimpScene, directory, matLoadingData, treeObjLoc, hierarchyNode.CreateChild<ModelComponent>(node->mChildren[i]->mName.C_Str()), node->mChildren[i], boneMapping, nullptr, Transform(), keepVertsData);
			}
			else if (aiBone* bone = CastAiNodeToBone(assimpScene, node->mChildren[i])) // (isBone)
				LoadHierarchyNodeFromAi(gameHandle, assimpScene, directory, matLoadingData, treeObjLoc, hierarchyNode.CreateChild<BoneComponent>(node->mChildren[i]->mName.C_Str()), node->mChildren[i], boneMapping, bone, Transform(), keepVertsData);
			else
				LoadHierarchyNodeFromAi(gameHandle, assimpScene, directory, matLoadingData, treeObjLoc, hierarchyNode.CreateChild<Component>(node->mChildren[i]->mName.C_Str()), node->mChildren[i], boneMapping, nullptr, Transform(), keepVertsData);
		}
	}

	void EngineDataLoader::LoadComponentsFromHierarchyTree(Component& comp, const HierarchyTemplate::HierarchyTreeT& tree, const HierarchyTemplate::HierarchyNodeBase& node, SkeletonInfo& skeletonInfo, Material* overrideMaterial)
	{
		if (PrimitiveDebugger::bDebugMeshTrees)
		{
			const HierarchyTemplate::HierarchyNode<ModelComponent>* meshNodeCast = dynamic_cast<const HierarchyTemplate::HierarchyNode<ModelComponent>*>(&node);
			if (meshNodeCast)
			{
				std::cout << comp.GetName() << ":::::" << meshNodeCast->GetCompT().GetMeshInstanceCount() << "\n";
			}
			else
				std::cout << comp.GetName() << "\n";
		}

		node.InstantiateToComp(comp);

		for (int i = 0; i < node.GetChildCount(); i++)	//WAZNE!!!! zmien meshNodeCast na node jak naprawisz childow
		{
			Component* child = nullptr;

			if (dynamic_cast<const HierarchyTemplate::HierarchyNode<ModelComponent>*>(node.GetChild(i)))
			{
				ModelComponent& model = comp.CreateComponent<ModelComponent>(node.GetChild(i)->GetCompBaseType().GetName(), Transform(), &skeletonInfo);
				model.SetSkeletonInfo(&skeletonInfo);
				child = &model;
			}
			else if (dynamic_cast<const HierarchyTemplate::HierarchyNode<BoneComponent>*>(node.GetChild(i)))
			{
				BoneComponent& bone = comp.CreateComponent<BoneComponent>(node.GetChild(i)->GetCompBaseType().GetName(), Transform(), tree.GetBoneMapping().GetBoneID(node.GetChild(i)->GetCompBaseType().GetName()));
				skeletonInfo.AddBone(bone);
				child = &bone;
			}
			else
				child = &comp.CreateComponent<Component>(node.GetChild(i)->GetCompBaseType().GetName(), Transform());

			LoadComponentsFromHierarchyTree(*child, tree, *node.GetChild(i), skeletonInfo, overrideMaterial);
		}
	}

	void EngineDataLoader::SetupSceneFromFile(GameManager* gameHandle, const std::string& filepath, const std::string& name)
	{	
		GameScene& scene = gameHandle->CreateScene((name.empty()) ? (filepath) : (name));
		std::ifstream file(filepath);
		std::string fileExtension = getFilepathExtension(filepath);
		std::stringstream filestr;

		filestr << file.rdbuf();
		file.close();

		if (fileExtension != ".geeprojectold")
		{
			std::cout << "Serializing...\n";
			GameManager::DefaultScene = &scene;
			if (filestr.good())
			{
				try
				{
					cereal::JSONInputArchive archive(filestr);
					GEE::CerealActorSerializationData::ScenePtr = &scene;
					scene.GetRenderData()->LoadSkeletonBatches(archive);
					LightProbeLoader::LoadLightProbeTextureArrays(scene.GetRenderData());
					scene.GetRootActor()->Load(archive);

					archive.serializeDeferments();

					scene.Load();
				}
				catch (cereal::Exception& ex)
				{
					std::cout << "ERROR: While loading scene: " << ex.what() << '\n';
				}
				scene.GetRootActor()->DebugHierarchy();
			}
			GameManager::DefaultScene = nullptr;
		}
		else
		{
			std::string type;
			Actor* currentActor = nullptr;
			UniquePtr<Actor> currentActorUniquePtr = nullptr;

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
						parent = scene.FindActor(parentName);

						if (!parent)
							std::cerr << "ERROR! Can't find actor " + parentName + ", parent of " + actorName + " will be assigned automatically.\n";
					}

					if (typeName == "GunActor")
						currentActor = (currentActorUniquePtr = MakeUnique<GunActor>(GunActor(scene, nullptr, actorName))).get();
					else if (typeName == "Actor")
						currentActor = (currentActorUniquePtr = MakeUnique<Actor>(Actor(scene, nullptr, actorName))).get();
					else
					{
						std::cerr << "ERROR! Unrecognized actor type " << typeName << ".\n";
						continue;
					}

					if (parent)
						parent->AddChild(std::move(currentActorUniquePtr));
					else
						scene.AddActorToRoot(std::move(currentActorUniquePtr));
				}

				else if (type == "newcomp")
				{
					if (!currentActor)
					{
						std::cerr << "ERROR! Component defined without an actor\n";
						break;
					}
					LoadComponentData(gameHandle, filestr, currentActor, scene);
				}

				else if (type == "newtree")
					LoadCustomHierarchyTree(scene, filestr);

				else if (type == "edittree")
					LoadCustomHierarchyTree(scene, filestr, true);

				else if (type == "newprobes")
					LoadLightProbes(scene, filestr);

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
		}

		std::cout << "Level loading finished.\n";
	}

	void EngineDataLoader::LoadModel(std::string path, Component& comp, MeshTreeInstancingType type, Material* overrideMaterial)
	{
		InstantiateTree(comp, *LoadHierarchyTree(comp.GetScene(), path), overrideMaterial);
	}


	HierarchyTemplate::HierarchyTreeT* EngineDataLoader::LoadHierarchyTree(GameScene& scene, std::string path, HierarchyTemplate::HierarchyTreeT* treePtr, bool keepVertsData)
	{
		GameManager& gameHandle = *scene.GetGameHandle();
		if (path.empty())
		{
			if (!treePtr || treePtr->GetName().empty())
				return nullptr;

			path = treePtr->GetName();
		}

		RenderEngineManager& renderHandle = *gameHandle.GetRenderEngineHandle();

		if (HierarchyTemplate::HierarchyTreeT* found = gameHandle.FindHierarchyTree(path, treePtr))
		{
			if (PrimitiveDebugger::bDebugMeshTrees)
				std::cout << "Found " << path << ".\n";
			return found;
		}
		if (!treePtr)
			treePtr = &scene.CreateHierarchyTree(path);

		Assimp::Importer importer;
		const aiScene* assimpScene;
		MaterialLoadingData matLoadingData;

		assimpScene = importer.ReadFile(path, aiProcess_GenUVCoords | aiProcess_TransformUVCoords | aiProcess_OptimizeMeshes | aiProcess_SplitLargeMeshes | aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenSmoothNormals | aiProcess_CalcTangentSpace | aiProcess_LimitBoneWeights | aiProcess_JoinIdenticalVertices | aiProcess_ValidateDataStructure);
		if (!assimpScene || assimpScene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !assimpScene->mRootNode)
		{
			std::cerr << "Can't load mesh scene " << path << ".\n";
			std::cerr << "Assimp error " << importer.GetErrorString() << '\n';
			return nullptr;
		}

		if (assimpScene->mFlags & AI_SCENE_FLAGS_VALIDATION_WARNING)
			std::cout << "WARNING! A validation problem occured while loading MeshTree " + path + "\n";


		std::string directory = extractDirectory(path);
		std::vector<ModelComponent*> modelsPtr;

		std::cout << "ROOT: " << &treePtr->GetRoot() << '\n';

		LoadHierarchyNodeFromAi(gameHandle, assimpScene, directory, &matLoadingData, *treePtr, (assimpScene->mRootNode->mNumMeshes > 0) ? (treePtr->GetRoot().CreateChild<ModelComponent>(treePtr->GetRoot().GetCompBaseType().GetName() + "RootMeshes")) : (treePtr->GetRoot()), assimpScene->mRootNode, treePtr->GetBoneMapping(), nullptr, Transform(), keepVertsData);

		for (int i = 0; i < static_cast<int>(assimpScene->mNumAnimations); i++)
		{
			treePtr->AddAnimation(Animation(*treePtr, assimpScene->mAnimations[i]));
			int animIndex = assimpScene->mNumAnimations - 1;
			std::cout << assimpScene->mAnimations[animIndex]->mDuration / assimpScene->mAnimations[animIndex]->mTicksPerSecond << "<- czas; " << assimpScene->mAnimations[animIndex]->mTicksPerSecond << "<- tps\n";
		}

		for (int j = 0; j < static_cast<int>(matLoadingData.LoadedMaterials.size()); j++)
			renderHandle.AddMaterial(matLoadingData.LoadedMaterials[j]);

		for (unsigned int i = 0; i < matLoadingData.LoadedMaterials.size(); i++)
			if (matLoadingData.LoadedMaterials[i]->GetRenderShaderName().empty())
				matLoadingData.LoadedMaterials[i]->SetRenderShaderName("Geometry");

		return treePtr;
	}

	void EngineDataLoader::InstantiateTree(Component& comp, HierarchyTemplate::HierarchyTreeT& tree, Material* overrideMaterial)
	{
		SkeletonInfo& skelInfo = *comp.GetScene().GetRenderData()->AddSkeletonInfo();
		LoadComponentsFromHierarchyTree(comp, tree, tree.GetRoot(), skelInfo, overrideMaterial);
		skelInfo.SetGlobalInverseTransformCompPtr(&comp);
		skelInfo.SortBones();
		if (tree.GetAnimationCount() > 0)
		{
			AnimationManagerComponent& animManager = comp.CreateComponent<AnimationManagerComponent>("An AnimationManagerComponent");
			for (int i = 0; i < tree.GetAnimationCount(); i++)
				animManager.AddAnimationInstance(AnimationInstance(tree.GetAnimation(i), comp));

			//comp.QueueAnimationAll(&tree.GetAnimation(0));
			if (DUPA::AnimTime == 9999.0f)
				DUPA::AnimTime = 0.0f;
		}
		skelInfo.GetBatchPtr()->RecalculateBoneCount();
	}

	SharedPtr<Font> EngineDataLoader::LoadFont(GameManager& gameHandle, const std::string& path)
	{
		if (!FTLib)
		{
			FTLib = new FT_Library;
			if (FT_Init_FreeType(FTLib))
			{
				std::cerr << "ERROR! Cannot init freetype library.\n";
				delete FTLib;
				FTLib = nullptr;
				return nullptr;
			}
		}

		if (auto found = gameHandle.FindFont(path))
			return found;

		FT_Face face;
		if (FT_New_Face(*FTLib, path.c_str(), 0, &face))
		{
			std::cerr << "ERROR! Cannot load font " + path + ".\n";
			return nullptr;
		}

		FT_Set_Pixel_Sizes(face, 0, 48);

		SharedPtr<Font> font = MakeShared<Font>(Font(path));
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		const float advanceUnit = 1.0f / 64.0f;
		const float pixelScale = 1.0f / 64.0f;

		font->SetBaselineHeight(static_cast<float>(face->ascender) * pixelScale * advanceUnit);

		for (int i = 0; i < 128; i++)
		{
			if (FT_Load_Char(face, i, FT_LOAD_RENDER))
			{
				std::cerr << "Can't load glyph " << char(i) << ".\n";
				continue;
			}

			font->GetBitmapsArray().Bind(0);
			glTexSubImage3D(font->GetBitmapsArray().GetType(), 0, 0, 0, i, face->glyph->bitmap.width, face->glyph->bitmap.rows, 1, GL_RED, GL_UNSIGNED_BYTE, face->glyph->bitmap.buffer);
			Character character = Character();
			character.ID = i;
			character.Size = Vec2f(face->glyph->bitmap.width, face->glyph->bitmap.rows) * pixelScale;
			character.Bearing = Vec2f(face->glyph->bitmap_left, face->glyph->bitmap_top) * pixelScale;
			character.Advance = static_cast<float>(face->glyph->advance.x) * pixelScale * advanceUnit;

			font->AddCharacter(character);
		}

		font->GetBitmapsArray().GenerateMipmap();

		return font;
	}

	template <class T> T EngineDataLoader::LoadSettingsFromFile(std::string path)
	{
		T settings = T();
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

	Mat4f toGlm(const aiMatrix4x4& aiMat)
	{
		return Mat4f(aiMat.a1, aiMat.b1, aiMat.c1, aiMat.d1,
			aiMat.a2, aiMat.b2, aiMat.c2, aiMat.d2,
			aiMat.a3, aiMat.b3, aiMat.c3, aiMat.d3,
			aiMat.a4, aiMat.b4, aiMat.c4, aiMat.d4);
	}

	template GameSettings EngineDataLoader::LoadSettingsFromFile<GameSettings>(std::string path);
}
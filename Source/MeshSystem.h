#pragma once
#include <vector>
#include <string>
#include <memory>
#include "CollisionObject.h"
class Mesh;
class CollisionObject;
class CollisionShape;
class Material;

/*
	Meshes loaded from files are placed in a tree structure - 1 tree per 1 file.
	I made it this way because that's the way some file format store their meshes (Assimp, which we use in the project does too).
	Thanks to using the same structure we get an easy access to previously loaded meshes to reuse them (without name conflicts).

	A MeshNode can be instantiated to ModelComponent.
	A ModelComponent is an instance of only one MeshNode, but a MeshNode can be instantiated by multiple ModelComponents.
	Instantiating a MeshNode is associated with instantiating its Meshes and CollisionObject (if there is one).

	In the future you will be able to create your own MeshTree from existing MeshNodes and create your own MeshNodes for the MeshTree from existing Meshes.
*/

namespace MeshSystem
{

	class MeshNode
	{
		std::vector<std::shared_ptr<Mesh>> Meshes;
		std::unique_ptr<CollisionObject> CollisionObjTemplate;
		std::string Name;	//name from mesh file
		std::vector <std::unique_ptr<MeshNode>> Children;
		Transform TemplateTransform;
		Material* OverrideMaterial;

	public:
		MeshNode(std::string name);
		MeshNode(const MeshNode&, bool copyChildren = false);
		MeshNode(MeshNode&&);
		std::string GetName() const;

		const MeshNode* GetChild(int index) const;
		int GetChildCount() const;
		int GetMeshCount() const;
		Mesh* GetMesh(int index) const;
		const Transform& GetTemplateTransform() const;
		Material* GetOverrideMaterial() const;

		MeshNode& AddChild(std::string name);
		MeshNode& AddChild(const MeshSystem::MeshNode&);
		MeshNode& AddChild(MeshSystem::MeshNode&&);
		Mesh& AddMesh(std::string name);
		void AddMesh(std::shared_ptr<Mesh>);
		std::unique_ptr<CollisionObject> InstantiateCollisionObj() const;

		Mesh* FindMesh(std::string name);
		Material* FindMaterial(std::string name);
		MeshNode* FindNode(std::string name);
		MeshNode* FindNode(int number, int&& currentNumber = 0);

		void SetCollisionObject(std::unique_ptr<CollisionObject>&);
		void SetTemplateTransform(Transform&);
		void SetOverrideMaterial(Material*);

		MeshNode& operator=(const MeshNode&);
		MeshNode& operator=(MeshNode&&);
	};

	class MeshTree
	{
		MeshNode Root;	//root of the tree. Its meshes will probably be ignored in loading
		std::string FilePath;	//This path is also serving a purpose for naming the tree. Trying to load a tree when there's an already loaded tree with the same path (name) should return a reference to the already loaded one.

	public:
		MeshTree(std::string path);
		MeshTree(const MeshTree&, std::string path = std::string());
		MeshTree(MeshTree&&, std::string path = std::string());
		std::string GetFilePath() const;
		MeshNode& GetRoot();
		bool IsEmpty();

		void SetFilePath(std::string);	//DOES NOT load from the path automatically. It's useful when you want to refer to this tree from the new name/path (f.e. used for engine objects, to keep their path as ENG_NAME instead of long file paths)

		Mesh* FindMesh(std::string name);
		Material* FindMaterial(std::string name);
		MeshNode* FindNode(std::string name);
		MeshNode* FindNode(int number);
	};
}

bool isInteger(std::string);	//works for int() >= 0
#pragma once
#include <vector>
#include <string>
#include <memory>
#include "CollisionObject.h"
#include "BoneComponent.h"
class Mesh;
struct CollisionObject;
struct CollisionShape;
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
	class MeshNode;

	class TemplateNode
	{
	protected:
		std::string Name;
		Transform TemplateTransform;
		std::vector <std::unique_ptr<TemplateNode>> Children;
		std::unique_ptr<CollisionObject> CollisionObjTemplate;
	public:
		TemplateNode(std::string name, const Transform& transform = Transform());
		TemplateNode(const TemplateNode&, bool copyChildren = false);
		TemplateNode(TemplateNode&&);
		std::string GetName() const;
		const Transform& GetTemplateTransform() const;
		const TemplateNode* GetChild(int index) const;
		int GetChildCount() const;
		std::unique_ptr<CollisionObject> InstantiateCollisionObj() const;

		void SetCollisionObject(std::unique_ptr<CollisionObject>&);
		void SetTemplateTransform(Transform&);

		template<typename T> T* AddChild(std::string name);
		template<typename T> T* AddChild(const T&);
		template<typename T> T* AddChild(T&&);

		TemplateNode* FindNode(std::string name);
		TemplateNode* FindNode(int number, int&& currentNumber = 0);

		virtual Mesh* FindMesh(std::string name);
		virtual const std::shared_ptr<Mesh> FindMeshPtr(std::string name);
		virtual Material* FindMaterial(std::string name);

		virtual void DebugPrint(int nrTabs = 0);

		virtual ~TemplateNode() {}
	};

	class BoneNode: public TemplateNode
	{
		glm::mat4 BoneOffset;
		unsigned int BoneID;
	public:
		BoneNode(std::string name, const Transform& transform = Transform());
		BoneNode(const BoneNode&, bool copyChildren = false);
		BoneNode(BoneNode&&);

		glm::mat4 GetBoneOffset() const;

		void SetBoneOffset(const glm::mat4&);
		void SetBoneID(unsigned int id);

		virtual void DebugPrint(int nrTabs = 0) override;
	};

	class MeshNode: public TemplateNode
	{
		std::vector<std::shared_ptr<Mesh>> Meshes;
		Material* OverrideMaterial;

	public:
		explicit MeshNode(std::string name, std::shared_ptr<Mesh> optionalMesh = nullptr);
		MeshNode(std::shared_ptr<Mesh> optionalMesh = nullptr);
		MeshNode(const MeshNode&, bool copyChildren = false);
		MeshNode(MeshNode&&);

		int GetMeshCount() const;
		Mesh* GetMesh(int index) const;
		Material* GetOverrideMaterial() const;

		Mesh& AddMesh(std::string name);
		void AddMesh(std::shared_ptr<Mesh>);

		virtual Mesh* FindMesh(std::string name) override;
		virtual const std::shared_ptr<Mesh> FindMeshPtr(std::string name) override;
		virtual Material* FindMaterial(std::string name) override;

		void SetOverrideMaterial(Material*);

		MeshNode& operator=(const MeshNode&);
		MeshNode& operator=(MeshNode&&);
	};

	class MeshTree
	{
		MeshNode Root;	//root of the tree. Its meshes will probably be ignored in loading
		std::string Path;	//This path is also serving a purpose for naming the tree. Trying to load a tree when there's an already loaded tree with the same path (name) should return a reference to the already loaded one.
		std::unique_ptr<BoneMapping> TreeBoneMapping;
	public: mutable std::shared_ptr<Animation> animation;

	public:
		MeshTree(std::string path);
		MeshTree(const MeshTree&, std::string path = std::string());
		MeshTree(MeshTree&&, std::string path = std::string());
		std::string GetPath() const;
		MeshNode& GetRoot();
		BoneMapping* GetBoneMapping() const;
		bool IsEmpty();

		void SetPath(std::string);	//DOES NOT load from the path automatically. It's useful when you want to refer to this tree from the new name/path (f.e. used for engine objects, to keep their path as ENG_NAME instead of long file paths)

		Mesh* FindMesh(std::string name);
		const std::shared_ptr<Mesh> FindMeshPtr(std::string name);
		Material* FindMaterial(std::string name);
		TemplateNode* FindNode(std::string name);
		TemplateNode* FindNode(int number);
	};
}

bool isInteger(std::string);	//works for int() >= 0
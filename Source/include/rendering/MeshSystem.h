#pragma once
#include <vector>
#include <string>
#include <memory>
#include <physics/CollisionObject.h>
#include <scene/BoneComponent.h>
#include <utility/Utility.h>
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

namespace HierarchyTemplate
{
	template <typename CompType> class HierarchyNode;
	class HierarchyNodeBase
	{
	public:
		virtual Component& GetCompBaseType() = 0;
		virtual unsigned int GetChildCount() const = 0;
		virtual HierarchyNodeBase* GetChild(unsigned int index) const = 0;
		virtual HierarchyNodeBase& AddChild(std::unique_ptr<HierarchyNodeBase> child) = 0;
		virtual HierarchyNodeBase* FindNode(const std::string& name) = 0;
		template <typename CompType> HierarchyNode<CompType>& CreateChild(HierarchyNode<CompType>&& node)
		{
			std::unique_ptr<HierarchyNode<CompType>> nodeSmartPtr = std::make_unique<HierarchyNode<CompType>>(std::move(node));
			HierarchyNode<CompType>& nodeRef = *nodeSmartPtr;
			AddChild(std::move(nodeSmartPtr));
			
			return nodeRef;
		}

		virtual void InstantiateToComp(Component& comp) const = 0;

		virtual std::unique_ptr<HierarchyNodeBase> Copy(bool copyChildren = false) const = 0;
	};

	template <typename CompType = Component> class HierarchyNode : public HierarchyNodeBase
	{
	public:
		HierarchyNode(CompType& compT) :
			CompT(compT) {}
		HierarchyNode(const HierarchyNode<CompType>& node, bool copyChildren = false):
			CompT(node.CompT)
		{
			if (copyChildren)
			{
				Children.reserve(node.Children.size());
				std::transform(node.Children.begin(), node.Children.end(), std::back_inserter(Children), [copyChildren](const std::unique_ptr<HierarchyNodeBase>& child) { return child->Copy(copyChildren); });
			}
		}
		std::unique_ptr<CompType> Instantiate(Component& parent, const std::string& name) const
		{
			InstantiateToComp(parent.CreateComponent<CompType>(CompType(parent.GetScene(), name)));
		}
		virtual	void InstantiateToComp(Component& comp) const override
		{
			comp = CompT;
		}
		virtual HierarchyNodeBase* GetChild(unsigned int index) const
		{
			if (index > Children.size() - 1)
				return nullptr;

			return Children[index].get();
		}
		virtual unsigned int GetChildCount() const
		{
			return Children.size();
		}
		CompType& GetCompT() const
		{
			return CompT;
		}
		virtual Component& GetCompBaseType() override
		{
			return CompT;
		}
		virtual HierarchyNodeBase& AddChild(std::unique_ptr<HierarchyNodeBase> child) override
		{
			Children.push_back(std::move(child));
			return *Children.back().get();
		}
		virtual HierarchyNodeBase* FindNode(const std::string& name)
		{
			if (GetCompT().GetName() == name)
				return this;

			for (auto& it : Children)
				if (auto found = it->FindNode(name))
					return found;

			return nullptr;
		}
		virtual std::unique_ptr<HierarchyNodeBase> Copy(bool copyChildren = false) const override
		{
			return static_unique_pointer_cast<HierarchyNodeBase>(std::make_unique<HierarchyNode<CompType>>(std::move(HierarchyNode<CompType>(*this, copyChildren))));
		}
	private:
		//friend CompType& CompType::operator=(const ComponentTemplate<CompType>&);
		std::vector<std::unique_ptr<HierarchyNodeBase>> Children;
		CompType& CompT;
	};

	class HierarchyTreeT
	{
	public:
		HierarchyTreeT(GameScene& scene, const std::string& name):
			Scene(scene),
			Name(name),
			Root(nullptr),
			TreeBoneMapping(nullptr) {}
		const std::string& GetName() const
		{
			return Name;
		}
		HierarchyNode<Component>& GetRoot()
		{
			return *Root;
		}
		BoneMapping& GetBoneMapping() const
		{
			return *TreeBoneMapping;
		}
		Animation& GetAnimation(unsigned int index)
		{
			return *TreeAnimations[index];
		}
		unsigned int GetAnimationCount()
		{
			return TreeAnimations.size();
		}
		void AddAnimation(const Animation& anim)
		{
			TreeAnimations.push_back(std::make_unique<Animation>(Animation(anim)));
		}
	private:
		std::string Name;	//(Path)
		GameScene& Scene;
		std::unique_ptr<HierarchyNode<Component>> Root;

		std::vector<std::unique_ptr<Animation>> TreeAnimations;
		std::unique_ptr<BoneMapping> TreeBoneMapping;
	};

	template <typename ActorType> class ActorTemplate : public ActorType
	{

	};
}

namespace MeshSystem
{
	class MeshNode;


	class TemplateNode
	{
	protected:
	public:
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

		void AddCollisionShape(std::shared_ptr<CollisionShape> shape);

		template<typename T> T* AddChild(std::string name);
		TemplateNode& AddChild(const TemplateNode&);
		TemplateNode& AddChild(TemplateNode&&);

		TemplateNode* FindNode(std::string name);
		TemplateNode* FindNode(int number, int&& currentNumber = 0);

		virtual Mesh* FindMesh(std::string name);
		virtual const std::shared_ptr<Mesh> FindMeshPtr(std::string name) const;
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
		virtual const std::shared_ptr<Mesh> FindMeshPtr(std::string name) const override;
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
		std::vector<std::unique_ptr<Animation>> TreeAnimations;
		mutable std::shared_ptr<Animation> animation;

	public:
		MeshTree(std::string path);
		MeshTree(const MeshTree&, std::string path = std::string());
		MeshTree(MeshTree&&, std::string path = std::string());
		std::string GetPath() const;
		MeshNode& GetRoot();
		BoneMapping* GetBoneMapping() const;
		int GetAnimationCount() const;
		Animation& GetAnimation(int index);
		bool IsEmpty();
		void AddAnimation(const Animation&);

		void SetPath(std::string);	//DOES NOT load from the path automatically. It's useful when you want to refer to this tree from the new name/path (f.e. used for engine objects, to keep their path as ENG_NAME instead of long file paths)

		Mesh* FindMesh(std::string name);
		const std::shared_ptr<Mesh> FindMeshPtr(std::string name) const;
		Material* FindMaterial(std::string name);
		TemplateNode* FindNode(std::string name);
		TemplateNode* FindNode(int number);
	};
}

bool isInteger(std::string);	//works for int() >= 0
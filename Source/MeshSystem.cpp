#include "MeshSystem.h"
#include "CollisionObject.h"
#include "Mesh.h"

using namespace MeshSystem;


MeshNode::MeshNode(std::string name) :
	Name(name),
	OverrideMaterial(nullptr)
{
}

MeshNode::MeshNode(const MeshNode& node, bool copyChildren) :
	Name(node.Name),
	Meshes(node.Meshes),
	TemplateTransform(node.TemplateTransform),
	OverrideMaterial(node.OverrideMaterial)
{
	CollisionObjTemplate = (node.CollisionObjTemplate) ? (std::make_unique<CollisionObject>(*node.CollisionObjTemplate)) : (nullptr);
	
	if (copyChildren)
	{
		for (int i = 0; i < node.Children.size(); i++)
			Children.push_back(std::make_unique<MeshNode>(MeshNode(*node.Children[i], true)));
		//Children.reserve(node.Children.size());
		//std::transform(node.Children.begin(), node.Children.end(), Children.begin(), [](const std::unique_ptr<MeshNode>& child) { return (child) ? (std::make_unique<MeshNode>(*child)) : (nullptr); });
	}
}

MeshNode::MeshNode(MeshNode&& node):
	Name(node.Name),
	Children(std::move(node.Children)),
	Meshes(node.Meshes),
	TemplateTransform(node.TemplateTransform),
	OverrideMaterial(node.OverrideMaterial)
{
	CollisionObjTemplate = std::unique_ptr<CollisionObject>((node.CollisionObjTemplate) ? (node.CollisionObjTemplate.release()) : (nullptr));

}

std::string MeshNode::GetName() const
{
	return Name;
}

const MeshNode* MeshNode::GetChild(int index) const
{
	if (index >= Children.size())
	{
		std::cout << "ERROR! Tried to get child nr " << index << ", but " + Name + " has only " << Children.size() << " children.\n";
		return nullptr;
	}

	return Children[index].get();
}

int MeshNode::GetChildCount() const
{
	return Children.size();
}

int MeshNode::GetMeshCount() const
{
	return Meshes.size();
}

Mesh* MeshNode::GetMesh(int index) const
{
	if (index >= Meshes.size())
		return nullptr;

	return Meshes[index].get();
}

const Transform& MeshNode::GetTemplateTransform() const
{
	return TemplateTransform;
}

Material* MeshNode::GetOverrideMaterial() const
{
	return OverrideMaterial;
}

MeshNode& MeshNode::AddChild(std::string name)
{
	Children.push_back(std::make_unique<MeshNode>(MeshNode(name)));
	return *Children.back().get();
}

MeshNode& MeshNode::AddChild(const MeshNode& node)
{
	Children.push_back(std::make_unique<MeshNode>(MeshNode(node)));
	return *Children.back();
}

MeshNode& MeshNode::AddChild(MeshNode&& node)
{
	Children.push_back(std::make_unique<MeshNode>(MeshNode(node)));
	return *Children.back();
}

Mesh& MeshNode::AddMesh(std::string name)
{
	Meshes.push_back(std::make_shared<Mesh>(Mesh(name)));
	return *Meshes.back();
}

void MeshNode::AddMesh(std::shared_ptr<Mesh> mesh)
{
	Meshes.push_back(mesh);
}

std::unique_ptr<CollisionObject> MeshSystem::MeshNode::InstantiateCollisionObj() const
{
	if (!CollisionObjTemplate)
		return nullptr;

	return std::make_unique<CollisionObject>(CollisionObject(*CollisionObjTemplate));
}

Mesh* MeshNode::FindMesh(std::string name)
{
	auto found = std::find_if(Meshes.begin(), Meshes.end(), [name](const std::shared_ptr<Mesh>& mesh) { return mesh->GetName() == name; });
	if (found != Meshes.end())
		return found->get();

	for (int i = 0; i < static_cast<int>(Children.size()); i++)
		if (Mesh* found = Children[i]->FindMesh(name))
			return found;

	return nullptr;
}

Material* MeshSystem::MeshNode::FindMaterial(std::string name)
{
	auto found = std::find_if(Meshes.begin(), Meshes.end(), [name](const std::shared_ptr<Mesh>& mesh) { return mesh->GetMaterial()->GetName() == name; });
	if (found != Meshes.end())
		return found->get()->GetMaterial();

	for (int i = 0; i < static_cast<int>(Children.size()); i++)
		if (Material* found = Children[i]->FindMaterial(name))
			return found;

	return nullptr;
}

MeshNode* MeshNode::FindNode(std::string name)
{
	if (Name == name)
		return this;

	for (int i = 0; i < static_cast<int>(Children.size()); i++)
		if (MeshNode* node = Children[i]->FindNode(name))
			return node;

	return nullptr;
}

MeshNode* MeshNode::FindNode(int number, int&& currentNumber)
{
	if (number == currentNumber)
		return this;

	for (int i = 0; i < static_cast<int>(Children.size()); i++)
	{
		currentNumber++;
		if (Children[i]->FindNode(number, (int&&)currentNumber))
			return Children[i].get();
	}

	return nullptr;
}

void MeshNode::SetCollisionObject(std::unique_ptr<CollisionObject>& obj)
{
	obj.swap(CollisionObjTemplate);
}

void MeshSystem::MeshNode::SetTemplateTransform(Transform& t)
{
	TemplateTransform = t;
}

void MeshNode::SetOverrideMaterial(Material* material)
{
	OverrideMaterial = material;
}

MeshNode& MeshSystem::MeshNode::operator=(const MeshNode& node)
{
	Name = node.Name;
	Meshes.insert(Meshes.end(), node.Meshes.begin(), node.Meshes.end());
	CollisionObjTemplate = (node.CollisionObjTemplate) ? (std::make_unique<CollisionObject>(*node.CollisionObjTemplate)) : (nullptr);
	TemplateTransform = node.TemplateTransform;
	//Children = std::move(node.Children);

	return *this;
}

MeshNode& MeshSystem::MeshNode::operator=(MeshNode&& node)
{
	Name = node.Name;
	Meshes.insert(Meshes.end(), node.Meshes.begin(), node.Meshes.end());
	CollisionObjTemplate = (node.CollisionObjTemplate) ? (std::make_unique<CollisionObject>(*node.CollisionObjTemplate)) : (nullptr);
	Children = std::move(node.Children);
	TemplateTransform = node.TemplateTransform;

	return *this;
}

/*
==================================================================================================================================================================
==================================================================================================================================================================
==================================================================================================================================================================
*/

MeshTree::MeshTree(std::string path):
	Root(path),
	Path(path)
{
}

MeshTree::MeshTree(const MeshTree& tree, std::string path) :
	Path((path.empty()) ? (tree.Path) : (path)),
	Root(tree.Root, true)
{

}

MeshTree::MeshTree(MeshTree&& tree, std::string path) :
	Path((path.empty()) ? (tree.Path) : (path)),
	Root(tree.Root, true)
{

}

std::string MeshTree::GetPath() const
{
	return Path;
}

MeshNode& MeshTree::GetRoot()
{
	return Root;
}

bool MeshTree::IsEmpty()
{
	return Root.GetChildCount() > 0;
}

void MeshTree::SetPath(std::string path)
{
	Path = path;
}

Mesh* MeshTree::FindMesh(std::string name)
{
	return Root.FindMesh(name);
}

Material* MeshSystem::MeshTree::FindMaterial(std::string name)
{
	return Root.FindMaterial(name);
}

MeshNode* MeshTree::FindNode(std::string name)
{
	if (isInteger(name))
		FindNode(std::stoi(name));

	return Root.FindNode(name);
}

MeshNode* MeshTree::FindNode(int number)
{
	return Root.FindNode(number);
}

bool isInteger(std::string str)
{
	return (!str.empty() && std::find_if(str.begin(), str.end(), [](unsigned char c) { return !std::isdigit(c); }) == str.end());
}
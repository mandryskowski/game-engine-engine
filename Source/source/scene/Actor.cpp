#include <scene/Actor.h>
#include <scene/Component.h>
#include <rendering/Mesh.h>
#include <rendering/RenderInfo.h>
#include <vector>

Actor::Actor(GameScene& scene, const std::string& name) :
	Scene(scene),
	Name(name),
	GameHandle(scene.GetGameHandle()),
	ParentActor(nullptr),
	bKillingProcessStarted(false),
	SetupStream(nullptr)
{
	RootComponent = std::make_unique<Component>(scene, name + "'s root", Transform());
	if (GameHandle->HasStarted())
		RootComponent->OnStartAll();
}

Actor::Actor(Actor&& moved):
	RootComponent(std::move(moved.RootComponent)),
	Children(std::move(moved.Children)),
	ParentActor(moved.ParentActor),
	Name(moved.Name),
	SetupStream(moved.SetupStream),
	Scene(moved.Scene),
	GameHandle(moved.GameHandle),
	bKillingProcessStarted(moved.bKillingProcessStarted)
{
}

void Actor::OnStart()
{
	Setup();
//	if (!GameHandle->HasStarted())
		RootComponent->OnStartAll();
}

void Actor::OnStartAll()
{
	OnStart();
	for (int i = 0; i < static_cast<int>(Children.size()); i++)
		Children[i]->OnStartAll();
}

Component* Actor::GetRoot() const
{
	return RootComponent.get();
}

std::string Actor::GetName() const
{
	return Name;
}

Transform* Actor::GetTransform() const
{
	return (RootComponent) ? (&RootComponent->GetTransform()) : (nullptr);
}

Actor* Actor::GetChild(unsigned int index) const
{
	return Children[index].get();
}

std::vector<Actor*> Actor::GetChildren()
{
	std::vector<Actor*> children;
	children.reserve(Children.size());
	std::transform(Children.begin(), Children.end(), std::back_inserter(children), [](std::unique_ptr<Actor>& child) -> Actor*{ return child.get(); });

	return children;
}

GameScene& Actor::GetScene()
{
	return Scene;
}

bool Actor::IsBeingKilled()
{
	return bKillingProcessStarted;
}

void Actor::SetName(const std::string& name)
{
	Name = name;
}

void Actor::DebugHierarchy(int nrTabs)
{
	RootComponent->DebugHierarchy(nrTabs);
	for (int i = 0; i < static_cast<int>(Children.size()); i++)
		Children[i]->DebugHierarchy(nrTabs + 1);
}

void Actor::DebugActorHierarchy(int nrTabs)
{
	std::string tabs;
	for (int i = 0; i < nrTabs; i++)
		tabs += "	";

	std::cout << tabs + "-" + Name + "  ";
	if (GetTransform())
	{
		std::cout << GetTransform()->PositionRef.x << " " << GetTransform()->PositionRef.y << "       ";
		std::cout << GetTransform()->ScaleRef.x << " " << GetTransform()->ScaleRef.y;
	}
	std::cout << "\n";

	for (int i = 0; i < static_cast<int>(Children.size()); i++)
		Children[i]->DebugActorHierarchy(nrTabs + 1);
}

void Actor::ReplaceRoot(std::unique_ptr<Component> component)
{
	RootComponent->MoveChildren(*component);
	component->GetTransform().SetParentTransform(RootComponent->GetTransform().GetParentTransform());
	std::cout << "liczba dzieci: " << RootComponent->Children.size() << "\n";
	RootComponent = std::move(component);
	std::cout << RootComponent << "\n";
}

void Actor::MoveCompToRoot(Component& comp)
{
	ReplaceRoot(std::move(RootComponent->DetachChild(comp)));
}

void Actor::SetTransform(Transform transform)
{
	RootComponent->SetTransform(transform);
}

void Actor::AddComponent(std::unique_ptr<Component> component)
{
	RootComponent->AddComponent(std::move(component));
}

Actor& Actor::AddChild(std::unique_ptr<Actor> child)
{
	Children.push_back(std::move(child));
	Children.back()->GetTransform()->SetParentTransform(&RootComponent->GetTransform());
	Children.back()->ParentActor = this;

	return *Children.back();
}

void Actor::SetSetupStream(std::stringstream* stream)
{
	SetupStream = stream;
	std::cout << "Adding " << SetupStream << " to actor " + Name + ".\n";
}

void Actor::Setup()
{
	if (SetupStream)
	{
		delete SetupStream;
		SetupStream = nullptr;
	}

	for (unsigned int i = 0; i < Children.size(); i++)
		Children[i]->Setup();
}

void Actor::HandleEventAll(const Event& ev)
{
	HandleEvent(ev);

	int childrenCount = static_cast<int>(Children.size());
	for (int i = 0; i < childrenCount; i++)	//This type of loop is put here on purpose. The children might add other children, which would case iterators to become invalid.
		Children[i]->HandleEventAll(ev);
}

void Actor::Update(float deltaTime)
{
	if (IsBeingKilled())
	{
		Delete();
		return;
	}

	RootComponent->UpdateAll(deltaTime);
	if (Name == "CubeActor")
		;//RootComponent->GetTransform().SetScale(glm::vec3(1.0f + glfwGetTime() * 0.05f));
}

void Actor::UpdateAll(float deltaTime)
{
	Update(deltaTime);
	for (unsigned int i = 0; i < Children.size(); i++)
		Children[i]->UpdateAll(deltaTime);
}

void Actor::MarkAsKilled()
{
	bKillingProcessStarted = true;
	for (auto& it : Children)
		it->MarkAsKilled();	
}

void Actor::Delete()
{
	if (RootComponent)
		RootComponent = nullptr;

	ParentActor->Children.erase(std::remove_if(ParentActor->Children.begin(), ParentActor->Children.end(), [this](std::unique_ptr<Actor>& child) { return child.get() == this; }), ParentActor->Children.end());
}

void Actor::DebugRender(RenderInfo info, Shader* shader) const
{
	RootComponent->DebugRenderAll(info, shader);
}

void Actor::DebugRenderAll(RenderInfo info, Shader* shader) const
{
	DebugRender(info, shader);
	for (int i = 0; i < static_cast<int>(Children.size()); i++)
		Children[i]->DebugRenderAll(info, shader);
}

Actor* Actor::FindActor(const std::string& name)
{
	if (Name == name)
		return this;

	for (int i = 0; i < static_cast<int>(Children.size()); i++)
		if (Actor* found = Children[i]->FindActor(name))
			if (!found->IsBeingKilled())
				return found;

	return nullptr;
}

const Actor* Actor::FindActor(const std::string& name) const
{
	return const_cast<Actor*>(this)->FindActor(name);
}

#include <UI/UICanvasActor.h>
#include <scene/UIInputBoxActor.h>
void Actor::GetEditorDescription(UIActor& editorParent, GameScene& editorScene)
{
	UIInputBoxActor& textActor = editorParent.CreateChild(UIInputBoxActor(editorScene, "ComponentsNameActor"));
	textActor.SetOnInputFunc([this](const std::string& content) { SetName(content); }, [this]() -> std::string { return GetName(); });
	//textActor.DeleteButtonModel();
	textActor.SetTransform(Transform(glm::vec2(0.0f, 3.0f), glm::vec2(0.125f, 0.5f / 0.04f * 0.125f)));
}

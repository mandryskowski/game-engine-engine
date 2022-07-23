#include <scene/Actor.h>
#include <scene/Component.h>
#include <game/GameScene.h>
#include <rendering/RenderInfo.h>
#include <vector>

#include <UI/UICanvasActor.h>
#include <scene/UIInputBoxActor.h>
#include <UI/UICanvasField.h>

GEE::GameScene* GEE::CerealActorSerializationData::ScenePtr = nullptr;
namespace GEE
{

	Actor::Actor(GameScene& scene, Actor* parentActor, const std::string& name, const Transform& t) :
		Scene(scene),
		Name(name),
		RootComponent(nullptr),
		GameHandle(scene.GetGameHandle()),
		ParentActor(parentActor),
		KillingProcessFrame(0),
		SetupStream(nullptr)
	{
		RootComponent = MakeUnique<Component>(*this, nullptr, Name + "'s root", t);
	}

	/*Actor::Actor(const Actor& rhs) :
		RootComponent(MakeUnique<Component>(*rhs.RootComponent)),
		ParentActor(nullptr),
		Name(rhs.Name),
		SetupStream(rhs.SetupStream),
		Scene(rhs.Scene),
		GameHandle(rhs.GameHandle)
	{
	}*/

	Actor::Actor(Actor&& moved) :
		RootComponent(std::move(moved.RootComponent)),
		Children(std::move(moved.Children)),
		ParentActor(moved.ParentActor),
		Name(moved.Name),
		SetupStream(moved.SetupStream),
		Scene(moved.Scene),
		GameHandle(moved.GameHandle),
		KillingProcessFrame(moved.KillingProcessFrame)
	{
		std::cout << "UWAGA: MOVE CONSTRUCTOR NIE DZIALA. (" + Name + ") (" + Scene.GetName() + ")\n";
		RootComponent = MakeUnique<Component>(*this, nullptr, Name + "'s root", Transform());
		//exit(-1);
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

	std::vector<Actor*> Actor::GetChildren()
	{
		std::vector<Actor*> children;
		children.reserve(Children.size());
		std::transform(Children.begin(), Children.end(), std::back_inserter(children), [](UniquePtr<Actor>& child) -> Actor* { return child.get(); });

		return children;
	}

	bool Actor::IsBeingKilled() const
	{
		return KillingProcessFrame != 0;
	}

	void Actor::SetName(const std::string& name)
	{
		Name = name;
	}

	void Actor::DebugHierarchy(int nrTabs)
	{
		std::string tabs;
		for (int i = 0; i < nrTabs; i++)
			tabs += "	";

		std::cout << tabs + Name + " (Actor)\n";
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
			std::cout << GetTransform()->GetPos().x << " " << GetTransform()->GetPos().y << "       ";
			std::cout << GetTransform()->GetScale().x << " " << GetTransform()->GetScale().y;
		}
		std::cout << "\n";

		for (int i = 0; i < static_cast<int>(Children.size()); i++)
			Children[i]->DebugActorHierarchy(nrTabs + 1);
	}

	void Actor::ReplaceRoot(UniquePtr<Component> component)
	{
		RootComponent->MoveChildren(*component);
		component->GetTransform().SetParentTransform(RootComponent->GetTransform().GetParentTransform());
		std::cout << "liczba dzieci: " << RootComponent->Children.size() << "\n";
		RootComponent = std::move(component);
		std::cout << RootComponent << "\n";
	}

	void Actor::MoveCompToRoot(Component& comp)
	{
		std::cout << "Moveuje compa " + comp.GetName() + " do roota. ActorRef do tej pory: " << &RootComponent->ActorRef << ". ActorRef teraz: " << &comp.ActorRef << '\n';
		ReplaceRoot(std::move(RootComponent->DetachChild(comp)));
		RootComponent->ParentComponent = nullptr;
	}

	void Actor::SetTransform(Transform transform)
	{
		RootComponent->SetTransform(transform);
	}

	Component& Actor::AddComponent(UniquePtr<Component> component)
	{
		return RootComponent->AddComponent(std::move(component));
	}

	Actor& Actor::AddChild(UniquePtr<Actor> child)
	{
		Children.push_back(std::move(child));
		Children.back()->GetTransform()->SetParentTransform(&RootComponent->GetTransform());
		Children.back()->ParentActor = this;

		if (Scene.HasStarted())
			Children.back()->OnStartAll();

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

	void Actor::HandleEvent(const Event& ev)
	{
		RootComponent->HandleEventAll(ev);
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
		RootComponent->UpdateAll(deltaTime);
	}

	void Actor::UpdateAll(float deltaTime)
	{
		if (IsBeingKilled() && GameHandle->GetTotalFrameCount() != KillingProcessFrame)
		{
			Delete();
			return;
		}

		Update(deltaTime);
		for (unsigned int i = 0; i < Children.size(); i++)
			Children[i]->UpdateAll(deltaTime);
	}

	void Actor::MarkAsKilled()
	{
		KillingProcessFrame = GameHandle->GetTotalFrameCount();
		if (!RootComponent->IsBeingKilled())	//Check if the root is not being killed to avoid infinite recursion (killing the root kills the actor which kills the root which kills the actor...)
			RootComponent->MarkAsKilled();

		for (auto& it : Children)
			it->MarkAsKilled();
	}

	void Actor::Delete()
	{
		while (!Children.empty())
			Children.front()->Delete();

		if (RootComponent)
		{
			RootComponent->Delete();
			RootComponent = nullptr;
		}

		if (!ParentActor)
			std::cout << "INFO: Cannot delete actor " + GetName() + " because it doesn't have a parent (it is probably the root of a scene). Hopefully it will be deleted when the scene gets deleted.\n";
		else
			ParentActor->Children.erase(std::remove_if(ParentActor->Children.begin(), ParentActor->Children.end(), [this](UniquePtr<Actor>& child) { return child.get() == this; }), ParentActor->Children.end());
	}

	void Actor::DebugRender(SceneMatrixInfo info, Shader* shader) const
	{
		RootComponent->DebugRenderAll(info, *shader);
	}

	void Actor::DebugRenderAll(SceneMatrixInfo info, Shader* shader) const
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

	void Actor::GetEditorDescription(EditorDescriptionBuilder descBuilder)
	{
		UIInputBoxActor& textActor = descBuilder.CreateActor<UIInputBoxActor>("ActorsNameActor");
		textActor.SetTransform(Transform(Vec2f(0.0f, 1.5f), Vec2f(5.0f, 1.25f)));
		textActor.SetOnInputFunc([this, descBuilder](const std::string& content) mutable
		{
			auto prevName = GetName();

			if (Scene.GetUniqueActorName(content) == content)
				SetName(content);

			if (Actor* foundSceneCanvas = descBuilder.GetEditorScene().GetRootActor()->FindActor("GEE_E_Scene_Actors_Canvas"))
				if (UIActivableButtonActor* foundButtonActor = foundSceneCanvas->GetActor<UIActivableButtonActor>(prevName))
				{
					foundButtonActor->SetName(GetName());
					if (auto foundTextComp = foundButtonActor->GetRoot()->GetComponent<TextComponent>("ButtonText"))
						foundTextComp->SetContent(GetName());
				}
		}
		, [this]() -> std::string { return GetName(); });
		textActor.DeleteButtonModel();
		textActor.GetContentTextComp()->SetFontStyle(FontStyle::Bold);


		UICanvasField& deleteField = descBuilder.AddField("Delete");
		UIButtonActor& deleteButton = deleteField.CreateChild<UIButtonActor>("DeleteButton", "Delete", [this, descBuilder]() mutable {
			MarkAsKilled();
			descBuilder.RefreshScene();	//Actor will be deselected automatically - SelectActor(nullptr) is always called in SelectScene(...).
			});

		if (Scene.GetRootActor() == this)	//Disallow deleting the root of a scene
			deleteButton.SetDisableInput(true);

		descBuilder.AddField("Set parent to").GetTemplates().ObjectInput<Actor, Actor>(*Scene.GetRootActor(), [this](Actor* newParent) { if (newParent) PassToDifferentParent(*newParent); });
	}

	template <typename Archive> void Actor::Save(Archive& archive) const
	{
		archive(CEREAL_NVP(Name), CEREAL_NVP(RootComponent), CEREAL_NVP(Children));
	}
	template <typename Archive> void Actor::Load(Archive& archive)
	{
		CerealComponentSerializationData::ActorRef = this;	//For constructing the root component and its children
		CerealComponentSerializationData::ParentComp = nullptr;	//The root component doesn't have a parent
		RootComponent = nullptr;
		archive(CEREAL_NVP(Name), CEREAL_NVP(RootComponent));
		std::cout << "Serializing actor " << Name << '\n';

		if (Scene.HasStarted())
			OnStartAll();

		Children.clear();
		archive(CEREAL_NVP(Children));

		for (auto& it : Children)
		{
			it->ParentActor = this;
			it->GetTransform()->SetParentTransform(GetTransform());
		}
	}

	template void Actor::Save<cereal::JSONOutputArchive>(cereal::JSONOutputArchive&) const;
	template void Actor::Load<cereal::JSONInputArchive>(cereal::JSONInputArchive&);
}
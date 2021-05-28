#define STB_IMAGE_IMPLEMENTATION
#define CEREAL_LOAD_FUNCTION_NAME Load
#define CEREAL_SAVE_FUNCTION_NAME Save
#define CEREAL_SERIALIZE_FUNCTION_NAME Serialize
#include <editor/GameEngineEngineEditor.h>
#include <scene/TextComponent.h>
#include <scene/CameraComponent.h>
#include <scene/UIWindowActor.h>
#include <scene/ModelComponent.h>
#include <scene/LightComponent.h>
#include <scene/LightProbeComponent.h>
#include <scene/UIInputBoxActor.h>
#include <UI/UICanvasField.h>
#include <UI/UIListActor.h>
#include <scene/GunActor.h>
#include <scene/PawnActor.h>
#include <scene/Controller.h>
#include <input/InputDevicesStateRetriever.h>
#include <whereami.h>
#include <map>



//Po 11 minutach i 7 sekundach crashuje przez C:\Users\48511\Desktop\PhysX-4.1\physx\source\foundation\include\PsBroadcast.h (199) : abort : User allocator returned NULL. Czemu?
//ODPOWIEDZ MEMORY LEAKS!!!!! we leak about 3 MB/s


extern "C"
{
	__declspec(dllexport) unsigned long NvOptimusEnablement = 0x00000001;
}

extern "C"
{
	__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}

CEREAL_REGISTER_TYPE(GunActor)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Actor, GunActor)
CEREAL_REGISTER_TYPE(PawnActor)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Actor, PawnActor)
CEREAL_REGISTER_TYPE(Controller)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Actor, Controller)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Controller, ShootingController)
CEREAL_REGISTER_TYPE(ShootingController)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Actor, ShootingController)
CEREAL_REGISTER_TYPE(CameraComponent)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Component, CameraComponent)
CEREAL_REGISTER_TYPE(ModelComponent)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Component, ModelComponent)
CEREAL_REGISTER_TYPE(TextComponent)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Component, TextComponent)
CEREAL_REGISTER_TYPE(BoneComponent)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Component, BoneComponent)
CEREAL_REGISTER_TYPE(LightComponent)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Component, LightComponent)
CEREAL_REGISTER_TYPE(SoundSourceComponent)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Component, SoundSourceComponent)
CEREAL_REGISTER_TYPE(AnimationManagerComponent)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Component, AnimationManagerComponent)
CEREAL_REGISTER_TYPE(LightProbeComponent)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Component, LightProbeComponent)

CEREAL_REGISTER_TYPE(Material)
CEREAL_REGISTER_TYPE(AtlasMaterial)

EditorManager* EditorEventProcessor::EditorHandle = nullptr;

class Base
{
public:
	int Pierwsza = 5;
	static std::unique_ptr<Base> Get()
	{
		return std::make_unique<Base>();
	}
	virtual void AA() { std::cout << Pierwsza << "|!!!!!\n"; };
};

class Inheriting : public Base
{
public:
	float Druga = 3.0f;
	static std::unique_ptr<Base> Get()
	{
		return static_unique_pointer_cast<Base>(std::make_unique<Inheriting>());
	}
	virtual void AA() override { std::cout << Druga << "|!!!!!\n"; };
};

class Different
{
public:
	Different(std::unique_ptr<Base>&& base):
		BaseObj(std::move(base))
	{
		BaseObj->AA();
	}

private:
	std::unique_ptr<Base> BaseObj;
};

class BaseAndInheritingFactory
{
	typedef std::unique_ptr<Base>(*CreateObjFun)(void);
	typedef std::map<std::string, CreateObjFun> FactoryMap;
	std::map<std::string, CreateObjFun> CreateMap;
public:
	BaseAndInheritingFactory()
	{
		Register("Base", Base::Get);
		Register("Inheriting", Inheriting::Get);
	}
	static BaseAndInheritingFactory& Get()
	{
		static BaseAndInheritingFactory factory;
		return factory;
	}
	std::unique_ptr<Base> CreateBase(const std::string& name)
	{
		FactoryMap::iterator it = CreateMap.find(name);
		if (it != CreateMap.end())
			return (*it->second)();
		return nullptr;
	}
	void Register(std::string name, CreateObjFun fun)
	{
		CreateMap[name] = fun;
	}
private:
};


class Klasa1 : public UICanvasElement
{
public:
	virtual Boxf<Vec2f> GetBoundingBox(bool world) override
	{
		return Boxf<Vec2f>(Vec2f(2.69f, 4.20f), Vec2f(1.337f, 6.9f));
	}
};

class Klasa2 : public UICanvasElement
{
public:
	virtual Boxf<Vec2f> GetBoundingBox(bool world) override
	{
		return Boxf<Vec2f>(Vec2f(3.19f, 0.32f), Vec2f(99.0f, 0.1f));
	}
};

class Ponadklasa : public Klasa1, public Klasa2
{
public:
	virtual Boxf<Vec2f> GetBoundingBox(bool world) override
	{
		return Boxf<Vec2f>(Vec2f(483.8f, 827.9f), Vec2f(371.0f, 0.0283f));
	}
};

int main(int argc, char** argv)
{
	Ponadklasa ponadobjekt;
	std::cout << ponadobjekt.GetBoundingBox(false).Position.x<< "\n";
	std::string programFilepath;
	std::string projectFilepathArgument;
	for (int i = 0; i < argc; i++)
	{
		std::cout << argv[i] << '\n';
		switch (i)
		{
		case 0: programFilepath = argv[i]; break;
		case 1: projectFilepathArgument = argv[i]; break;
		}
	}
	Different(BaseAndInheritingFactory::Get().CreateBase("Inheriting"));
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);

	glm::dvec3 front(-0.80989, - 0.15385, - 0.56604);
	glm::dvec3 rotationVec = glm::cross(front, glm::dvec3(0.0f, 0.0f, -1.0f));
	double angle = std::atan2((double)glm::dot(front, glm::dvec3(0.0f, 1.0f, 0.0f)), 0.0);
	glm::quat rot(glm::angleAxis(angle, rotationVec));
	printVector(rot, "quick maths");

	GLFWwindow* programWindow = glfwCreateWindow(800, 600, "c00lki", nullptr, nullptr);

	if (!programWindow)
	{
		std::cerr << "nie dziala glfw :('\n";
		return -1;
	}

	glfwMakeContextCurrent(programWindow);
	
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))	//zaladuj glad (openGL); jesli sie nie udalo, to konczymy program
	{
		std::cerr << "zjebalo sie.";
		return -1;
	}

	int max_v_uniforms;
	glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &max_v_uniforms);
	std::cout << "max: " << max_v_uniforms << "\n";
	glGetIntegerv(GL_MAX_VERTEX_UNIFORM_COMPONENTS, &max_v_uniforms);
	std::cout << "max2: " << max_v_uniforms << "\n";

	GameSettings settings;
	//settings.LoadFromFile("Settings.ini");
	
	//glm::vec2 res = static_cast<glm::vec2>(settings.WindowSize);
	//settings.ViewportData = glm::uvec4(res.x * 0.3f, res.y * 0.4f, res.x * 0.4, res.y * 0.6f);
	//settings.Video.Resolution = glm::vec2(res.x * 0.4, res.y * 0.6f);

	GameEngineEngineEditor editor(programWindow, settings);
	editor.SetupMainMenu();

	editor.SetActiveScene(editor.GetScene("GEE_Main_Menu"));
	editor.PassMouseControl(nullptr);

	if (!projectFilepathArgument.empty())
		editor.LoadProject(projectFilepathArgument);

	/*std::ifstream deserializationStream("zjebany.json");

	std::cout << "Serializing...\n";
	if (deserializationStream.good() && editor.GetMainScene())
	{
		try
		{
			cereal::JSONInputArchive archive(deserializationStream);
			cereal::LoadAndConstruct<Actor>::ScenePtr = editor.GetMainScene();
			editor.GetMainScene()->GetRenderData()->LoadSkeletonBatches(archive);
			const_cast<Actor*>(editor.GetMainScene()->GetRootActor())->Load(archive);

			archive.serializeDeferments();

			editor.GetMainScene()->Load();
		}
		catch (cereal::Exception& ex)
		{
			std::cout << "ERROR: While loading scene: " << ex.what() << '\n';
		}
		const_cast<Actor*>(editor.GetMainScene()->GetRootActor())->DebugHierarchy();
	}
	deserializationStream.close();*/


	editor.PreGameLoop();

	float deltaTime = 0.0f;
	float lastUpdateTime = glfwGetTime();
	bool endGame = false;

	do
	{
		deltaTime = glfwGetTime() - lastUpdateTime;
		lastUpdateTime = glfwGetTime();

		if (Material* found = editor.GetRenderEngineHandle()->FindMaterial("GEE_Engine_Title").get())
			found->SetColor(hsvToRgb(Vec3f(glm::mod((float)glfwGetTime() * 10.0f, 360.0f), 1.0f, 1.0f)));

		endGame = editor.GameLoopIteration(1.0f / 60.0f, deltaTime);
	} while (!endGame);
	//game.Run();

	editor.SaveProject();

	return 0;
}

/*
	ENGINE TEXTURE UNITS
	0-9 free
	10 2D shadow map array
	11 3D shadow map array
	12 irradiance map array
	13 prefilter map array
	14 BRDF lookup texture
	15+ free
	TODO: Environment map jest mipmapowany; czy te mipmapy sa wgl uzyte???
*/
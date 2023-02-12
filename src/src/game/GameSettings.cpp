#include <game/GameSettings.h>
#include <fstream>
#include <sstream>
#include <utility/CerealNames.h>
#include <cereal/archives/json.hpp>

namespace GEE
{
	GameSettings::GameSettings()
	{
		bWindowFullscreen = false;
		WindowTitle = "GEE Window";
	}

	GameSettings::GameSettings(const std::string& path) :
		GameSettings()
	{
		LoadFromFile(path);
	}

	void GameSettings::LoadFromFile(const std::string& path)	//TODO: zangielszczyc
	{
		/* FORMAT PLIKOW:
		rodzajdanych dane
	np.	windowsize 800 600
		fullscreen 0
		*/

		std::fstream file;	//wczytaj plik inicjalizujacy
		file.open(path);
		std::stringstream filestr;
		filestr << file.rdbuf();

		std::string settingName;

		while (filestr >> settingName)	//wczytuj kolejne wyrazy w pliku inicjalzujacym
		{								//jesli napotkasz na wyraz, ktory sygnalizuje rodzaj danych to wczytaj te dane (sposob jest rozny w przypadku roznych danych)
			LoadSetting(filestr, settingName);
		}

		//prosta, lecz stosunkowo odporna na bledy i szybka implementacja; moze sie wywalic kiedy podamy zle dane do okreslonego rodzaju, np po zasygnalizowaniu windowsize podany zostaje string
	}

	bool GameSettings::LoadSetting(std::stringstream& filestr, const std::string& settingName)
	{
		if (settingName == "windowsize")
			filestr >> Video.Resolution.x >> Video.Resolution.y;	//rozmiar okna jest dwuwymiarowy, wiec wczytaj 2 liczby (2 wymiary)
		else if (settingName == "fullscreen")
			filestr >> bWindowFullscreen;				//bool wczytujemy tak jak int - 0 jest falszywe a wieksza wartosc (1) prawdziwa
		else if (settingName == "windowtitle")
			getline(filestr.ignore(), WindowTitle);	//tytul moze skladac sie z wielu wyrazow, wczytaj wiec cala linie do konca oraz pomin jeden znak, gdyz jest to spacja
		else
			return Video.LoadSetting(filestr, settingName);

		return true;
	}

	template <class T> void LoadEnum(std::stringstream& filestr, T& var)
	{
		int varNr;
		filestr >> varNr;
		var = static_cast<T>(varNr);
	}

	template void LoadEnum<SettingLevel>(std::stringstream& filestr, SettingLevel& var);

	template<typename Archive>
	void GameSettings::Serialize(Archive& archive)
	{
		archive(CEREAL_NVP(bWindowFullscreen), CEREAL_NVP(WindowTitle), CEREAL_NVP(Video));
	}
	template void GameSettings::Serialize<cereal::JSONOutputArchive>(cereal::JSONOutputArchive&);
	template void GameSettings::Serialize<cereal::JSONInputArchive>(cereal::JSONInputArchive&);

	GameSettings::VideoSettings::VideoSettings():
		Resolution(0.0f),
		AmbientOcclusionSamples(0),
		bVSync(false),
		bBloom(true),
		bForceForwardRendering(false),
		bForceWireframeRendering(false),
		bDrawToWindowFBO(false),
		AAType(AntiAliasingType::AA_NONE),
		AALevel(SettingLevel::SETTING_NONE),
		MonitorGamma(2.2f),
		POMLevel(SettingLevel::SETTING_NONE),
		ShadowLevel(SettingLevel::SETTING_LOW),
		Max2DShadows(16),
		Max3DShadows(16),
		Shading(ShadingAlgorithm::SHADING_FULL_LIT),
		TMType(ToneMappingType::TM_REINHARD)
	{

	}

	std::string GameSettings::VideoSettings::GetShaderDefines(Vec2u resolution) const
	{
		if (resolution == Vec2u(0))
			resolution = Vec2u(Resolution);

		//Cast width and height to float so the macro in shader is also float.
		std::string shaderDefines = "#define SCR_WIDTH " + std::to_string(static_cast<float>(resolution.x)) +
			"\n#define SCR_HEIGHT " + std::to_string(static_cast<float>(resolution.y)) + "\n";

		if (bBloom)
			shaderDefines += "#define ENABLE_BLOOM 1\n";
		if (AmbientOcclusionSamples > 0)
		{
			shaderDefines += "#define ENABLE_SSAO 1\n";
			shaderDefines += "#define SSAO_SAMPLES " + std::to_string(AmbientOcclusionSamples) + "\n";
		}
		if (IsVelocityBufferNeeded())
			shaderDefines += "#define CALC_VELOCITY_BUFFER 1\n";
		if (POMLevel != SETTING_NONE)
		{
			shaderDefines += "#define ENABLE_POM 1\n";
			switch (POMLevel)
			{
			case SETTING_LOW: shaderDefines += "#define POM_PRESET_LOW 1\n"; break;
			case SETTING_MEDIUM: shaderDefines += "#define POM_PRESET_MEDIUM 1\n"; break;
			case SETTING_HIGH: shaderDefines += "#define POM_PRESET_HIGH 1\n"; break;
			case SETTING_ULTRA: shaderDefines += "#define POM_PRESET_ULTRA 1\n"; break;
			}
		}

		return shaderDefines;
	}

	bool GameSettings::VideoSettings::IsVelocityBufferNeeded() const
	{
		return AAType == AntiAliasingType::AA_SMAAT2X;
	}

	bool GameSettings::VideoSettings::IsTemporalReprojectionEnabled() const
	{
		return AAType == AntiAliasingType::AA_SMAAT2X;
	}

	bool GameSettings::VideoSettings::LoadSetting(std::stringstream& filestr, std::string settingName)
	{
		if (settingName == "ssaosamples")
			filestr >> AmbientOcclusionSamples;
		else if (settingName == "vsync")
			filestr >> bVSync;
		else if (settingName == "bloom")
			filestr >> bBloom;
		else if (settingName == "aa")
		{
			int nrAA, levelAA;
			filestr >> nrAA >> levelAA;
			AAType = static_cast<AntiAliasingType>(nrAA);
			AALevel = static_cast<SettingLevel>(levelAA);
		}
		else if (settingName == "gamma")
			filestr >> MonitorGamma;
		else if (settingName == "pom")
		{
			LoadEnum<SettingLevel>(filestr, POMLevel);
		}
		else if (settingName == "shadow")
		{
			LoadEnum<SettingLevel>(filestr, ShadowLevel);
		}
		else
			return false;

		return true;
	}

	template<typename Archive>
	void GameSettings::VideoSettings::Serialize(Archive& archive)
	{
		archive(CEREAL_NVP(AmbientOcclusionSamples), CEREAL_NVP(bVSync), CEREAL_NVP(bBloom), CEREAL_NVP(AAType), CEREAL_NVP(AALevel),
				CEREAL_NVP(MonitorGamma), CEREAL_NVP(POMLevel), CEREAL_NVP(ShadowLevel), CEREAL_NVP(TMType),
				CEREAL_NVP(Max2DShadows), CEREAL_NVP(Max3DShadows));
	}
	template void GameSettings::VideoSettings::Serialize<cereal::JSONOutputArchive>(cereal::JSONOutputArchive&);
	template void GameSettings::VideoSettings::Serialize<cereal::JSONInputArchive>(cereal::JSONInputArchive&);
}
#include "GameSettings.h"

GameSettings::GameSettings()
{
	WindowSize = glm::uvec2(800, 600);
	bWindowFullscreen = false;
	WindowTitle = "kulki";
	AmbientOcclusionSamples = 0;
	bVSync = false;
	bBloom = true;
	AAType = AntiAliasingType::AA_NONE;
	AALevel = SettingLevel::SETTING_NONE;
	MonitorGamma = 2.2f;
	POMLevel = SettingLevel::SETTING_NONE;
	ShadowLevel = SettingLevel::SETTING_LOW;
}

GameSettings::GameSettings(std::string path) :
	GameSettings()	//zainicjalizuj wszystkie zmienne - plik moze byc uszkodzony
{
	LoadFromFile(path);
}

bool GameSettings::IsVelocityBufferNeeded() const
{
	return AAType == AntiAliasingType::AA_SMAAT2X;
}

bool GameSettings::IsTemporalReprojectionEnabled() const
{
	return AAType == AntiAliasingType::AA_SMAAT2X;
}

std::string GameSettings::GetShaderDefines(glm::uvec2 resolution) const
{
	if (resolution == glm::uvec2(0))
		resolution = WindowSize;
	std::string shaderDefines = "#define SCR_WIDTH " + std::to_string(resolution.x) + 
							  "\n#define SCR_HEIGHT " + std::to_string(resolution.y) + "\n";

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

void GameSettings::LoadFromFile(std::string path)	//TODO: zangielszczyc
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

void GameSettings::LoadSetting(std::stringstream& filestr, std::string settingName)
{
	if (settingName == "windowsize")
		filestr >> WindowSize.x >> WindowSize.y;	//rozmiar okna jest dwuwymiarowy, wiec wczytaj 2 liczby (2 wymiary)
	else if (settingName == "fullscreen")
		filestr >> bWindowFullscreen;				//bool wczytujemy tak jak int - 0 jest falszywe a wieksza wartosc (1) prawdziwa
	else if (settingName == "windowtitle")
		getline(filestr.ignore(), WindowTitle);	//tytul moze skladac sie z wielu wyrazow, wczytaj wiec cala linie do konca oraz pomin jeden znak, gdyz jest to spacja
	else if (settingName == "ssaosamples")
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
}

template <class T> void LoadEnum(std::stringstream& filestr, T& var)
{
	int varNr;
	filestr >> varNr;
	var = static_cast<T>(varNr);
}

template void LoadEnum<SettingLevel>(std::stringstream& filestr, SettingLevel& var);
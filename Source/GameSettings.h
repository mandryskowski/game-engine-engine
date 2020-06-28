#pragma once
#include "Utility.h"
enum AntiAliasingType
{
	AA_NONE,
	AA_SMAA
};

struct GameSettings
{
	glm::uvec2 WindowSize;
	bool bWindowFullscreen;
	std::string WindowTitle;

	unsigned int AmbientOcclusionSamples;
	bool bVSync;
	bool bBloom;
	AntiAliasingType AAType;

	float MonitorGamma;

	/////////////////////////////

	GameSettings()
	{
		WindowSize = glm::uvec2(800, 600);
		bWindowFullscreen = false;
		WindowTitle = "kulki";
		AmbientOcclusionSamples = 0;
		bVSync = false;
		bBloom = true;
		AAType = AntiAliasingType::AA_NONE;
		MonitorGamma = 2.2f;
	}

	GameSettings(std::string path) :
		GameSettings()	//zainicjalizuj wszystkie zmienne - plik moze byc uszkodzony
	{
		LoadFromFile(path);
	}

	std::string GetShaderDefines() const
	{
		std::string shaderDefines;

		if (bBloom)
			shaderDefines += "#define ENABLE_BLOOM\n";
		if (AmbientOcclusionSamples > 0)
			shaderDefines += "#define ENABLE_SSAO\n";

		return shaderDefines;
	}

	void LoadFromFile(std::string path)	//TODO: zangielszczyc
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

	virtual void LoadSetting(std::stringstream& filestr, std::string settingName)
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
			int nrAA;
			filestr >> nrAA;
			AAType = static_cast<AntiAliasingType>(nrAA);
		}
		else if (settingName == "gamma")
			filestr >> MonitorGamma;
	}
};
#pragma once
#include "Utility.h"
struct GameSettings
{
	glm::uvec2 WindowSize;
	bool bWindowFullscreen;
	std::string WindowTitle;

	unsigned int AmbientOcclusionSamples;
	bool bVSync;

	float MonitorGamma;

	/////////////////////////////

	GameSettings()
	{
		WindowSize = glm::uvec2(800, 600);
		bWindowFullscreen = false;
		WindowTitle = "kulki";
		bVSync = false;
		AmbientOcclusionSamples = 0;
		MonitorGamma = 2.2f;
	}

	GameSettings(std::string path) :
		GameSettings()	//zainicjalizuj wszystkie zmienne - plik moze byc uszkodzony
	{
		/* FORMAT PLIKOW:
		rodzajdanych dane
	np.	windowsize 800 600
		fullscreen 0
		*/
		std::fstream file;	//wczytaj plik inicjalizujacy
		file.open(path);

		std::string settingName;

		while (file >> settingName)	//wczytuj kolejne wyrazy w pliku inicjalzujacym
		{							//jesli napotkasz na wyraz, ktory sygnalizuje rodzaj danych to wczytaj te dane (sposob jest rozny w przypadku roznych danych), np:
			if (settingName == "windowsize")
				file >> WindowSize.x >> WindowSize.y;	//rozmiar okna jest dwuwymiarowy, wiec wczytaj 2 liczby (2 wymiary)
			else if (settingName == "fullscreen")
				file >> bWindowFullscreen;				//bool wczytujemy tak jak int - 0 jest falszywe a wieksza wartosc (1) prawdziwa
			else if (settingName == "windowtitle")
				getline(file.ignore(), WindowTitle);	//tytul moze skladac sie z wielu wyrazow, wczytaj wiec cala linie do konca oraz pomin jeden znak, gdyz jest to spacja
			else if (settingName == "ssaosamples")
				file >> AmbientOcclusionSamples;
			else if (settingName == "vsync")
				file >> bVSync;
			else if (settingName == "gamma")
				file >> MonitorGamma;
		}

		//prosta, lecz stosunkowo odporna na bledy i szybka implementacja; moze sie wywalic kiedy podamy zle dane do okreslonego rodzaju, np po zasygnalizowaniu windowsize podany zostaje string
	}
};
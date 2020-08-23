#pragma once
#include "Utility.h"

enum AntiAliasingType
{
	AA_NONE,
	AA_SMAA1X,
	AA_SMAAT2X
};

enum SettingLevel
{
	SETTING_NONE,
	SETTING_LOW,
	SETTING_MEDIUM,
	SETTING_HIGH,
	SETTING_ULTRA
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
	SettingLevel AALevel;
	SettingLevel POMLevel;
	SettingLevel ShadowLevel;

	float MonitorGamma;

	/////////////////////////////

	GameSettings();
	GameSettings(std::string path);

	bool IsVelocityBufferNeeded() const;
	bool IsTemporalReprojectionEnabled() const;
	std::string GetShaderDefines(glm::uvec2 resolution = glm::uvec2(0)) const;
	void LoadFromFile(std::string path);

	virtual void LoadSetting(std::stringstream& filestr, std::string settingName);
};

template <class T> void LoadEnum(std::stringstream& filestr, T& var);
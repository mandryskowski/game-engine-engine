#pragma once
#include "Shader.h"
#include <vector>

enum EngineObjectTypes
{
	QUAD,
	CUBE,
	SPHERE,
	CONE
};

struct UniformBuffer
{
	unsigned int UBO;
	size_t offsetCache;

	UniformBuffer();
	void Generate(unsigned int, size_t, float* = nullptr, GLenum = GL_DYNAMIC_DRAW);
	void SubData1i(int, size_t);
	void SubData1f(float, size_t);
	void SubData(size_t, float*, size_t);
	void SubData4fv(glm::vec3, size_t);
	void SubData4fv(std::vector<glm::vec3>, size_t);
	void SubData4fv(glm::vec4, size_t);
	void SubData4fv(std::vector<glm::vec4>, size_t);
	void SubDataMatrix4fv(glm::mat4, size_t);
	void PadOffset();
};

std::string getNextWord(std::stringstream&);			//checks the next word in stream without moving the pointer
bool isNextWordEqual(std::stringstream&, std::string);	//works just like the previous one, but moves the pointer if the word is equal to a passed string
bool toBool(std::string);
std::string framebufferStatusToString(GLenum);
void printVector(glm::vec3);

float lerp(float, float, float);
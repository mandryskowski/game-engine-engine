#pragma once
#include <rendering/Shader.h>
#include <vector>
#include <memory>

enum InterpolationType
{
	CONSTANT,
	LINEAR,
	QUADRATIC,
	CUBIC,
	QUARTIC,
	QUINTIC
};

/*
==========================================
==========================================
==========================================
*/

enum class EngineBasicShape
{
	QUAD,
	CUBE,
	SPHERE,
	CONE
};

/*
==========================================
==========================================
==========================================
*/

struct UniformBuffer
{
	UniformBuffer();
	void Generate(unsigned int blockBindingSlot, size_t size, float* = nullptr, GLenum = GL_DYNAMIC_DRAW);

	void SubData1i(int, size_t offset);
	void SubData1f(float, size_t offset);
	void SubData(size_t size, float* data, size_t offset);
	void SubData4fv(glm::vec3, size_t offset);
	void SubData4fv(std::vector<glm::vec3>, size_t offset);
	void SubData4fv(glm::vec4, size_t offset);
	void SubData4fv(std::vector<glm::vec4>, size_t offset);
	void SubDataMatrix4fv(glm::mat4, size_t offset);
	void PadOffset();

	void Dispose();

	unsigned int UBO;
	unsigned int BlockBindingSlot;
	size_t offsetCache;
};

/*
==========================================
==========================================
==========================================
*/

std::string lookupNextWord(std::stringstream&);			//checks the next word in stream without moving the pointer
std::string multipleWordInput(std::stringstream&);
bool isNextWordEqual(std::stringstream&, std::string);	//works just like the previous one, but moves the pointer if the word is equal to a passed string

bool toBool(std::string);
glm::vec3 toEuler(const glm::quat& quat);
glm::quat toQuat(const glm::vec3& euler);

std::string extractDirectory(std::string path);

void printVector(const glm::vec2&, std::string title = std::string());
void printVector(const glm::vec3&, std::string title = std::string());
void printVector(const glm::vec4&, std::string title = std::string());
void printVector(const glm::quat&, std::string title = std::string());

void printMatrix(const glm::mat4&, std::string title = std::string());

template<typename To, typename From>
std::unique_ptr<To> static_unique_pointer_cast(std::unique_ptr<From>&& old) {
	return std::unique_ptr<To>{static_cast<To*>(old.release())};
	//conversion: unique_ptr<FROM>->FROM*->TO*->unique_ptr<TO>
}

template <typename BaseClass, typename ChildClass>
ChildClass& creator(std::unique_ptr<BaseClass>& objectOwner, ChildClass&& objectConstructor)
{
	objectOwner = static_unique_pointer_cast<BaseClass>(std::make_unique<ChildClass>(std::move(objectConstructor)));
	return dynamic_cast<ChildClass>(*objectOwner);
}
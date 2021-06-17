#pragma once
#include <rendering/Shader.h>
#include <vector>
#include <memory>

namespace GEE
{
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

		void BindToSlot(unsigned int blockBindingSlot, bool isAlreadyBound);

		bool HasBeenGenerated() const;

		void SubData1i(int, size_t offset);
		void SubData1f(float, size_t offset);
		void SubData(size_t size, float* data, size_t offset);
		void SubData4fv(Vec3f, size_t offset);
		void SubData4fv(std::vector<Vec3f>, size_t offset);
		void SubData4fv(Vec4f, size_t offset);
		void SubData4fv(std::vector<Vec4f>, size_t offset);
		void SubDataMatrix4fv(Mat4f, size_t offset);
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
	Vec3f toEuler(const Quatf& quat);
	Quatf toQuat(const Vec3f& euler);

	std::string extractDirectory(std::string path);
	void extractDirectoryAndFilename(const std::string& fullPath, std::string& filename, std::string& directory);

	void printVector(const Vec2f&, std::string title = std::string());
	void printVector(const Vec3f&, std::string title = std::string());
	void printVector(const Vec4f&, std::string title = std::string());
	void printVector(const Quatf&, std::string title = std::string());

	void printMatrix(const Mat4f&, std::string title = std::string());

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

	constexpr int32_t floorConstexpr(float num)
	{
		return static_cast<float>(static_cast<int32_t>(num));
	}

	std::string toValidFilepath(std::string);
	std::string getFilepathExtension(const std::string& filepath);
	std::string getFileName(const std::string& filepath);

	/**
	* @brief Compares two numbers of type float while accounting for floating-point accuracy.
	* @param a: one of the numbers
	* @param b: the other number
	* @param epsilon: the maximum absolute difference between numbers that we consider equal
	* @return a boolean indicating whether the numbers are equal according to our epsilon
	*/
	bool floatComparison(float a, float b, float epsilon);

	template <typename T, typename Pred> typename std::vector<T>::iterator insertSorted(std::vector<T>& vec, const T& element, Pred pred)
	{
		return vec.insert(std::upper_bound(vec.begin(), vec.end(), element, pred), element);
	}

	/*
	template <typename Func> void assertFn(bool expression, Func func)
	{
		if (!expression)
		{
			func();
			abort();
		}
	}

	void assertM(bool expression, const std::string& message)
	{
		assertFn(expression, [message]() { std::cout << "ERROR: ASSERTION FAILURE: " + message + '\n'; });
	}*/
}
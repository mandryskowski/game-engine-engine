#pragma once
#include <math/Vec.h>
#include <vector>
#include <memory>
#include <glad/glad.h>
#include <utility/Asserts.h>

struct GLFWwindow;
namespace GEE
{
	template <typename T> using UniquePtr = std::unique_ptr<T>;
	template <typename T, typename... Args>
	constexpr UniquePtr<T> MakeUnique(Args&&... args)
	{
		return std::make_unique<T>(std::forward<Args>(args)...);
	}

	template <typename T> using SharedPtr = std::shared_ptr<T>;
	template <typename T, typename... Args>
	constexpr SharedPtr<T> MakeShared(Args&&... args)
	{
		return std::make_shared<T>(std::forward<Args>(args)...);
	}

	template <typename T>
	std::string ToStringPrecision(const T& val, const unsigned int precision = 6)
	{
		std::ostringstream str;
		str.precision(precision);
		str << std::fixed << val;
		return str.str();
	}

	template <typename InputIt, typename UnaryFunction>
	std::string GetUniqueName(InputIt first, InputIt last, UnaryFunction&& getNameOfObjectFunc, const std::string& name)
	{
		// If name is not repeated, return the name because it's unique.
		if (std::find_if(first, last, [&](auto& obj) { return getNameOfObjectFunc(obj) == name; }) == last)
			return name;

		int addedIndex = 1;
		std::string currentNameCandidate = name + std::to_string(addedIndex);

		while (std::find_if(first, last, [&](auto& obj) { return getNameOfObjectFunc(obj) == currentNameCandidate; }) != last)
			currentNameCandidate = name + std::to_string(++addedIndex);

		return currentNameCandidate;
	}

	using SystemWindow = GLFWwindow;
	class WindowData
	{
	public:
		WindowData(SystemWindow&);
		Vec2d GetMousePositionPx() const;
		Vec2d GetMousePositionNDC() const;

		Vec2i GetWindowSize() const;
	private:
		SystemWindow& WindowRef;
	};

	namespace pxConversion
	{
		Vec2d PxToNDC(Vec2d pos, Vec2i windowSize);
		Vec2d NDCToPx(Vec2d pos, Vec2i windowSize);
	}

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
		void Generate(unsigned int blockBindingSlot, size_t size, const float* = nullptr, GLenum = GL_DYNAMIC_DRAW);

		void BindToSlot(unsigned int blockBindingSlot, bool isAlreadyBound);

		bool HasBeenGenerated() const;

		void SubData1i(int, size_t offset);
		void SubData1f(float, size_t offset);
		void SubData(size_t size, const float* data, size_t offset);
		void SubData4fv(const Vec3f&, size_t offset);
		void SubData4fv(std::vector<Vec3f>, size_t offset);
		void SubData4fv(const Vec4f, size_t offset);
		void SubData4fv(std::vector<Vec4f>, size_t offset);
		void SubDataMatrix4fv(const Mat4f&, size_t offset);
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
	UniquePtr<To> static_unique_pointer_cast(UniquePtr<From>&& old) {
		return UniquePtr<To>{static_cast<To*>(old.release())};
		//conversion: UniquePtr<FROM>->FROM*->TO*->UniquePtr<TO>
	}

	template <typename BaseClass, typename ChildClass>
	ChildClass& creator(UniquePtr<BaseClass>& objectOwner, ChildClass&& objectConstructor)
	{
		objectOwner = static_unique_pointer_cast<BaseClass>(MakeUnique<ChildClass>(std::move(objectConstructor)));
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
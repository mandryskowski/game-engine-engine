#pragma once
#include <math/Vec.h>
#include <vector>
#include <memory>
#include <sstream>
#include <glad/glad.h>
#include <utility/Asserts.h>
#include <utility>

struct GLFWwindow;


namespace GEE
{
	typedef uint64_t GEEID;

	template <typename T>
	GEEID GetGEEIDPtr(const T* obj)
	{
		return (obj) ? (obj->GetGEEID()) : (0);
	}
}

namespace GEE
{
	using Time = double;

	template <typename T, typename Deleter = std::default_delete<T>> using UniquePtr = std::unique_ptr<T, Deleter>;
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

	using String = std::string;
	template <typename T> using Vector = std::vector<T>;
	template <typename T1, typename T2> using Pair = std::pair<T1, T2>;

	template <typename T> using WeakPtr = std::weak_ptr<T>;

	template <typename T>
	String ToStringPrecision(const T& val, const unsigned int precision = 6)
	{
		std::ostringstream str;
		str.precision(precision);
		str << std::fixed << val;
		return str.str();
	}

	String BoolToString(bool b);

	template <typename InputIt, typename UnaryFunction>
	String GetUniqueName(InputIt first, InputIt last, UnaryFunction&& getNameOfObjectFunc, const String& name)
	{
		// If name is not repeated, return the name because it's unique.
		if (std::find_if(first, last, [&](auto& obj) { return getNameOfObjectFunc(obj) == name; }) == last)
			return name;

		int addedIndex = 1;
		String currentNameCandidate = name + std::to_string(addedIndex);

		while (std::find_if(first, last, [&](auto& obj) { return getNameOfObjectFunc(obj) == currentNameCandidate; }) != last)
			currentNameCandidate = name + std::to_string(++addedIndex);

		return currentNameCandidate;
	}

	class Actor;
	class Component;
	class GameScene;
	struct CerealComponentSerializationData
	{
		static GEE::Actor* ActorRef;			//= dummy actor
		static GEE::Component* ParentComp;		//= always nullptr
	};

	struct CerealTreeSerializationData
	{
		static GEE::GameScene* TreeScene;
	};

	struct CerealNodeSerializationData
	{
		static GEE::Actor* TempActor;
	};

	using SystemWindow = GLFWwindow;
	class WindowData
	{
	public:
		WindowData(SystemWindow&);
		
		/**
		 * \param originBottomLeft If true, then coordinates are from the bottom-left corner. Otherwise, the more popular convention
		 * of using the top-left corner as origin is used. When using GEE, always choose bottom-left as origin.
		 * \return A 2D pixel coordinate of the current mouse position of this WindowData's window.
		 */
		Vec2d GetMousePositionPx(bool originBottomLeft = true) const;
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
		Constant,
		Linear,
		Quadratic,
		Cubic,
		Quartic,
		Quintic
	};

	/*
	==========================================
	==========================================
	==========================================
	*/

	enum class EngineBasicShape
	{
		Quad,
		Cube,
		Sphere,
		Cone
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

		[[nodiscard]] bool HasBeenGenerated() const;

		void SubData1i(int, size_t offset);
		void SubData1f(float, size_t offset);
		void SubData(size_t size, const float* data, size_t offset);
		void SubData4fv(const Vec3f&, size_t offset);
		void SubData4fv(std::vector<Vec3f>, size_t offset);
		void SubData4fv(const Vec4f&, size_t offset);
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

	String lookupNextWord(std::stringstream&);			//checks the next word in stream without moving the pointer
	String multipleWordInput(std::stringstream&);
	bool isNextWordEqual(std::stringstream&, String);	//works just like the previous one, but moves the pointer if the word is equal to a passed string

	bool toBool(String);
	Vec3f toEuler(const Quatf& quat);
	Quatf toQuat(const Vec3f& euler);

	String extractDirectory(String path);
	void extractDirectoryAndFilename(const String& fullPath, String* filenameGet = nullptr, String* directoryGet = nullptr);


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
		return static_cast<int32_t>(num);
	}

	String toValidFilepath(String);
	String getFilepathExtension(const String& filepath);
	String getFileName(const String& filepath, bool keepExtension = true);

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

	void assertM(bool expression, const String& message)
	{
		assertFn(expression, [message]() { std::cout << "ERROR: ASSERTION FAILURE: " + message + '\n'; });
	}*/
}
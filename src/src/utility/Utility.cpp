#include <utility/Utility.h>
#include <functional>
#include <algorithm>
#include <sstream>
#include <glfw/glfw3.h>

namespace GEE
{
	UniformBuffer::UniformBuffer():
		UBO(0),
		BlockBindingSlot(0),
		offsetCache(0)
	{
	}

	void UniformBuffer::Generate(unsigned int blockBindingSlot, size_t size, const float* data, GLenum usage)
	{
		Dispose();
		glGenBuffers(1, &UBO);

		glBindBuffer(GL_UNIFORM_BUFFER, UBO);
		glBufferData(GL_UNIFORM_BUFFER, size, data, usage);
		glBindBufferBase(GL_UNIFORM_BUFFER, blockBindingSlot, UBO);

		offsetCache = 0;
		BlockBindingSlot = blockBindingSlot;
	}

	void UniformBuffer::BindToSlot(unsigned int blockBindingSlot, bool isAlreadyBound)
	{
		if (!isAlreadyBound)
			glBindBuffer(GL_UNIFORM_BUFFER, UBO);
		glBindBufferBase(GL_UNIFORM_BUFFER, blockBindingSlot, UBO);
	}

	bool UniformBuffer::HasBeenGenerated() const
	{
		return UBO != 0;
	}

	void UniformBuffer::SubData1i(int data, size_t offset)
	{
		glBindBuffer(GL_UNIFORM_BUFFER, UBO);
		glBufferSubData(GL_UNIFORM_BUFFER, offset, sizeof(int), &data);
		offsetCache = offset + sizeof(int);
	}

	void UniformBuffer::SubData1f(float data, size_t offset)
	{
		SubData(4, &data, offset);
	}

	void UniformBuffer::SubData(size_t size, const float* data, size_t offset)
	{
		glBindBuffer(GL_UNIFORM_BUFFER, UBO);
		glBufferSubData(GL_UNIFORM_BUFFER, offset, size, data);
		offsetCache = offset + size;
	}

	void UniformBuffer::SubData4fv(const Vec3f& vec, size_t offset)
	{
		Vec4f bufferVector(vec, 0.0f);
		SubData(sizeof(Vec4f), Math::GetDataPtr(bufferVector), offset);
	}

	void UniformBuffer::SubData4fv(std::vector<Vec3f> vecs, size_t offset)
	{
		offsetCache = offset;
		for (const auto& vec: vecs)
			SubData4fv(vec, offsetCache);
	}

	void UniformBuffer::SubData4fv(const Vec4f& vec, size_t offset)
	{
		SubData(sizeof(Vec4f), Math::GetDataPtr(vec), offset);
	}

	void UniformBuffer::SubData4fv(std::vector<Vec4f> vecs, size_t offset)
	{
		offsetCache = offset;
		for (const auto& vec: vecs)
			SubData4fv(vec, offsetCache);
	}

	void UniformBuffer::SubDataMatrix4fv(const Mat4f& mat, size_t offset)
	{
		SubData(64, Math::GetDataPtr(mat), offset);
	}

	void UniformBuffer::PadOffset()
	{
		if (offsetCache % 16 == 0)
			return;
		offsetCache = offsetCache - offsetCache % 16 + 16; //pad to the next multiple of 16
	}

	void UniformBuffer::Dispose()
	{
		if (HasBeenGenerated())
			glDeleteBuffers(1, &UBO); 
	}

	String BoolToString(bool b)
	{
		return (b) ? ("true") : ("false");
	}

	String lookupNextWord(std::stringstream& str)
	{
		String word;
		size_t pos = str.tellg();
		str >> word;
		str.seekg(pos);

		return word;
	}

	String multipleWordInput(std::stringstream& str)
	{
		String word;
		str >> word;

		if (word.empty() || word[0] != '"')
			return word;

		String input = word;
		while (word.back() != '"')
		{
			str >> word;
			input += " " + word;
		}

		return input.substr(1, input.size() - 2); //return the input string without quotation marks
	}

	bool isNextWordEqual(std::stringstream& str, String equalTo)
	{
		String word;
		size_t pos = str.tellg();
		str >> word;

		if (word == equalTo)
			return true;

		str.seekg(pos); //if its not equal, go back to the previous position
		return false;
	}

	bool toBool(String str)
	{
		if (str == "false" || str == "0")
			return false;
		return true;
	}

	Vec3f toEuler(const Quatf& quat)
	{
		return glm::degrees(glm::eulerAngles(quat));
	}

	Quatf toQuat(const Vec3f& euler)
	{
		return Quatf(glm::radians(euler));
	}

	String extractDirectory(String path)
	{
		size_t dirPos = path.find_last_of('/');
		if (dirPos != String::npos)
			return path.substr(0, dirPos + 1);
		dirPos = path.find_last_of(static_cast<char>(92)); //backslash
		if (dirPos != String::npos)
			return path.substr(0, dirPos + 1);

		return String();
	}

	void extractDirectoryAndFilename(const String& fullPath, String* filenameGet, String* directoryGet)
	{
		size_t dirPos = fullPath.find_last_of('/');
		std::string filename, directory;

		std::function<void(size_t)> splitPath = [fullPath, &directory, &filename](size_t dirPos)
		{
			directory = fullPath.substr(0, dirPos + 1);
			if (dirPos + 1 <= fullPath.size())
				filename = fullPath.substr(dirPos + 1);
		};

		if (dirPos != std::string::npos)
			return splitPath(dirPos);

		dirPos = fullPath.find_last_of(static_cast<char>(92)); //backslash

		if (dirPos != std::string::npos)
			return splitPath(dirPos);
		else
		{
			directory.clear();
			filename = fullPath;
		}

		if (filenameGet)
			*filenameGet = filename;
		if (directoryGet)
			*directoryGet = directory;
	}

	std::string toValidFilepath(std::string str)
	{
		std::vector<char> invalidChars = {' ', '\n', '/', 92, ':', '?', '"', '<', '>', '|'}; //92 is backslash ( \ )
		std::transform(str.begin(), str.end(), str.begin(), [&invalidChars](const unsigned char ch) -> unsigned char
		{
			return (std::find(invalidChars.begin(), invalidChars.end(), ch) != invalidChars.end())
				       ? ('-')
				       : (std::tolower(ch));
		});

		return str;
	}

	std::string getFilepathExtension(const std::string& filepath)
	{
		size_t dotPos = filepath.find_last_of('.');
		if (dotPos == std::string::npos)
			return std::string();

		return filepath.substr(dotPos);
	}

	std::string getFileName(const std::string& filepath, bool keepExtension)
	{
		auto lastSlash = filepath.find_last_of('/');
		auto lastDot = filepath.find_last_of('.');

		if (lastSlash == std::string::npos)
			lastSlash = filepath.find_last_of(static_cast<char>(92)); //backslash
		if (lastDot == String::npos)
			lastDot = filepath.length();

		if (lastSlash == String::npos)
			return (keepExtension) ? (filepath) : (filepath.substr(0, lastDot)); //filepath is the filename

		return (filepath.substr(lastSlash + 1, keepExtension ? (filepath.length() - lastSlash - 1) : (lastDot - lastSlash - 1)));
	}

	bool floatComparison(float a, float b, float epsilon)
	{
		return glm::abs(a - b) <= epsilon;
	}

	WindowData::WindowData(SystemWindow& window):
		WindowRef(window)
	{
	}

	Vec2d WindowData::GetMousePositionPx(bool originBottomLeft) const
	{
		Vec2d mousePos;
		glfwGetCursorPos(&WindowRef, &mousePos.x, &mousePos.y);

		if (originBottomLeft)
			mousePos.y = GetWindowSize().y - mousePos.y;

		return mousePos;
	}

	Vec2d WindowData::GetMousePositionNDC() const
	{
		return pxConversion::PxToNDC(GetMousePositionPx(), GetWindowSize());
	}

	Vec2i WindowData::GetWindowSize() const
	{
		Vec2i windowSize;
		glfwGetWindowSize(&WindowRef, &windowSize.x, &windowSize.y);
		return windowSize;
	}


	Vec2d pxConversion::PxToNDC(Vec2d pos, Vec2i windowSize)
	{
		Vec2d mousePos = pos / static_cast<Vec2d>(windowSize);
		return mousePos * 2.0 - 1.0;
	}

	Vec2d pxConversion::NDCToPx(Vec2d pos, Vec2i windowSize)
	{
		Vec2d mousePos = pos * 0.5 + 0.5;
		return mousePos / static_cast<Vec2d>(windowSize);
	}


}
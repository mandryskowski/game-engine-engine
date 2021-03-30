#include <utility/Utility.h>

UniformBuffer::UniformBuffer()
{
	UBO = 0;
	offsetCache = 0;
}
void UniformBuffer::Generate(unsigned int blockBindingSlot, size_t size, float* data, GLenum usage)
{
	Dispose();
	glGenBuffers(1, &UBO);

	glBindBuffer(GL_UNIFORM_BUFFER, UBO);
	glBufferData(GL_UNIFORM_BUFFER, size, data, usage);
	glBindBufferBase(GL_UNIFORM_BUFFER, blockBindingSlot, UBO);

	offsetCache = 0;
	BlockBindingSlot = blockBindingSlot;
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
void UniformBuffer::SubData(size_t size, float* data, size_t offset)
{
	glBindBuffer(GL_UNIFORM_BUFFER, UBO);
	glBufferSubData(GL_UNIFORM_BUFFER, offset, size, data);
	offsetCache = offset + size;
}

void UniformBuffer::SubData4fv(glm::vec3 vec, size_t offset)
{
	glm::vec4 bufferVector(vec, 0.0f);
	SubData(sizeof(glm::vec4), glm::value_ptr(bufferVector), offset);
}
void UniformBuffer::SubData4fv(std::vector<glm::vec3> vecs, size_t offset)
{
	offsetCache = offset;
	for (unsigned int i = 0; i < vecs.size(); i++)
		SubData4fv(vecs[i], offsetCache);
}
void UniformBuffer::SubData4fv(glm::vec4 vec, size_t offset)
{
	SubData(sizeof(glm::vec4), glm::value_ptr(vec), offset);
}
void UniformBuffer::SubData4fv(std::vector<glm::vec4> vecs, size_t offset)
{
	offsetCache = offset;
	for (unsigned int i = 0; i < vecs.size(); i++)
		SubData4fv(vecs[i], offsetCache);
}

void UniformBuffer::SubDataMatrix4fv(glm::mat4 mat, size_t offset)
{
	SubData(64, glm::value_ptr(mat), offset);
}

void UniformBuffer::PadOffset()
{
	if (offsetCache % 16 == 0)
		return;
	offsetCache = offsetCache - offsetCache % 16 + 16;	//pad to the next multiple of 16
}

void UniformBuffer::Dispose()
{
	if (UBO != 0)
		glDeleteBuffers(1, &UBO);
}

std::string lookupNextWord(std::stringstream& str)
{
	std::string word;
	size_t pos = (size_t)str.tellg();
	str >> word;
	str.seekg(pos);

	return word;
}

std::string multipleWordInput(std::stringstream& str)
{
	std::string word;
	str >> word;

	if (word.empty() || word[0] != '"')
		return word;

	std::string input = word;
	while (word.back() != '"')
	{
		str >> word;
		input += " " + word;
	}

	return input.substr(1, input.size() - 2);	//return the input string without quotation marks
}

bool isNextWordEqual(std::stringstream& str, std::string equalTo)
{
	std::string word;
	size_t pos = (size_t)str.tellg();
	str >> word;

	if (word == equalTo)
		return true;

	str.seekg(pos);	//if its not equal, go back to the previous position
	return false;
}

bool toBool(std::string str)
{
	if (str == "false" || str == "0")
		return false;
	return true;
}

glm::vec3 toEuler(const glm::quat& quat)
{
	return glm::degrees(glm::eulerAngles(quat));
}

glm::quat toQuat(const glm::vec3& euler)
{
	return glm::quat(glm::radians(euler));
}

std::string extractDirectory(std::string path)
{
	size_t dirPos = path.find_last_of('/');
	if (dirPos != std::string::npos)
		return path.substr(0, dirPos + 1);
	dirPos = path.find_last_of(static_cast<char>(92));	//backslash
	if (dirPos != std::string::npos)
		return path.substr(0, dirPos + 1);

	return std::string();
}

void printVector(const glm::vec2& vec, std::string title)
{
	if (!title.empty())
		title.append(": ");	//if we want to draw a title, add something after it

	std::cout << title << vec.x << ", " << vec.y << '\n';
}

void printVector(const glm::vec3& vec, std::string title)
{
	if (!title.empty())
		title.append(": ");	//if we want to draw a title, add something after it

	std::cout << title << vec.x << ", " << vec.y << ", " << vec.z << '\n';
}

void printVector(const glm::vec4& vec, std::string title)
{
	if (!title.empty())
		title.append(": ");	//if we want to draw a title, add something after it

	std::cout << title << vec.x << ", " << vec.y << ", " << vec.z << ", " << vec.w << '\n';
}

void printVector(const glm::quat& q, std::string title)
{
	printVector(glm::degrees(glm::eulerAngles(q)), title);
}

void printMatrix(const glm::mat4& mat, std::string title)
{
	if (!title.empty())
		std::cout << "===Matrix " + title + "===\n";
	for (int y = 0; y < 4; y++)
	{
		for (int x = 0; x < 4; x++)
			std::cout << mat[x][y] << "	";
		std::cout << '\n';
	}
}
#include "Utility.h"

UniformBuffer::UniformBuffer()
{
	UBO = 0;
	offsetCache = 0;
}
void UniformBuffer::Generate(unsigned int index, size_t size, float* data, GLenum usage)
{
	glGenBuffers(1, &UBO);

	glBindBuffer(GL_UNIFORM_BUFFER, UBO);
	glBufferData(GL_UNIFORM_BUFFER, size, data, usage);
	glBindBufferBase(GL_UNIFORM_BUFFER, index, UBO);

	offsetCache = 0;
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
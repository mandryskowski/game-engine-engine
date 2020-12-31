#pragma once
#include <glm/glm.hpp>

class NDCViewport;

class Viewport
{
public:
	Viewport(const glm::uvec2& pos, const glm::uvec2& size);
	const glm::uvec2& GetSize() const;
	void SetOpenGLState() const;
	NDCViewport ToNDCViewport(const glm::uvec2& res) const;


private:
	glm::uvec2 Position;
	glm::uvec2 Size;
};

class NDCViewport
{
public:
	NDCViewport(const glm::vec2& pos, const glm::vec2& size);
	const glm::vec2& GetSize() const;
	void SetOpenGLState(const glm::uvec2& res) const;
	Viewport ToPxViewport(const glm::uvec2& res) const;


private:
	glm::vec2 Position;
	glm::vec2 Size;
};
#pragma once

#include "GL.hpp"

#include <glm/glm.hpp>

struct PixelCanvas {
	static constexpr uint32_t Width = 256, Height = 240;
	static constexpr float Aspect = float(Width) / float(Height);

	PixelCanvas() = default;
	PixelCanvas(PixelCanvas const &) = delete;
	~PixelCanvas();

	void begin(glm::vec3 const &background);
	void end(glm::uvec2 const &drawable_size);
	glm::vec2 from_window(glm::vec2 const &position, glm::uvec2 const &window_size) const;
	glm::mat4 world_to_clip() const;

	GLuint framebuffer = 0, texture = 0;
	glm::vec4 rect = glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);
};

#include "PixelCanvas.hpp"

#include <algorithm>
#include <stdexcept>

PixelCanvas::~PixelCanvas() {
	if (framebuffer) glDeleteFramebuffers(1, &framebuffer);
	if (texture) glDeleteTextures(1, &texture);
}

void PixelCanvas::begin(glm::vec3 const &background) {
	if (framebuffer == 0) {
		glGenTextures(1, &texture);
		glBindTexture(GL_TEXTURE_2D, texture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8_ALPHA8, Width, Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glBindTexture(GL_TEXTURE_2D, 0);
		glGenFramebuffers(1, &framebuffer);
		glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) throw std::runtime_error("Pixel canvas framebuffer is incomplete.");
	}
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
	glViewport(0, 0, Width, Height);
	glClearColor(background.r, background.g, background.b, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	glDisable(GL_DEPTH_TEST);
}

void PixelCanvas::end(glm::uvec2 const &drawable_size) {
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, drawable_size.x, drawable_size.y);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	uint32_t scale = std::min(drawable_size.x / Width, drawable_size.y / Height);
	glm::ivec2 size = glm::ivec2(drawable_size);
	if (scale >= 1) size = glm::ivec2(scale * Width, scale * Height);
	glm::ivec2 lower_left = (glm::ivec2(drawable_size) - size) / 2;
	rect = glm::vec4(glm::vec2(lower_left) / glm::vec2(drawable_size), glm::vec2(size) / glm::vec2(drawable_size));

	glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glDisable(GL_FRAMEBUFFER_SRGB);
	glBlitFramebuffer(0, 0, Width, Height,
		lower_left.x, lower_left.y, lower_left.x + size.x, lower_left.y + size.y,
		GL_COLOR_BUFFER_BIT, GL_NEAREST);
	glEnable(GL_FRAMEBUFFER_SRGB);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
}

glm::vec2 PixelCanvas::from_window(glm::vec2 const &position, glm::uvec2 const &window_size) const {
	glm::vec2 f = glm::vec2(position.x / float(window_size.x), 1.0f - position.y / float(window_size.y));
	glm::vec2 uv = (f - glm::vec2(rect.x, rect.y)) / glm::vec2(rect.z, rect.w);
	return glm::vec2((uv.x * 2.0f - 1.0f) * Aspect, uv.y * 2.0f - 1.0f);
}

glm::mat4 PixelCanvas::world_to_clip() const {
	return glm::mat4(
		1.0f / Aspect, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	);
}

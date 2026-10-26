#pragma once

/*
 * Helper class for simple immediate-mode drawing of lines -- mostly intended for
 * DEBUG output and quick tests.
 *
 * Similar usage pattern to DrawSprites.
 *
 */


#include <glm/glm.hpp>

#include <string>
#include <vector>

struct DrawLines {
	//Start drawing; will remember world_to_clip matrix:
	DrawLines(glm::mat4 const &world_to_clip);

	//draw a single line from a to b (in world space):
	void draw(glm::vec3 const &a, glm::vec3 const &b, glm::u8vec4 const &color = glm::u8vec4(0xff));

	//draw a wireframe box corresponding to the [-1,1]^3 cube transformed by mat:
	void draw_box(glm::mat4x3 const &mat, glm::u8vec4 const &color = glm::u8vec4(0xff));

	//draw wireframe text, start at anchor, move in x direction, mat gives x and y directions for text drawing:
	// (default character box is 1 unit high)
	void draw_text(std::string const &text,
		glm::vec3 const &anchor,
		glm::vec3 const &x = glm::vec3(1.0f, 0.0f, 0.0f),
		glm::vec3 const &y = glm::vec3(0.0f, 1.0f, 1.0f),
		glm::u8vec4 const &color = glm::u8vec4(0xff),
		glm::vec3 *anchor_out = nullptr);

	void fill_triangle(glm::vec3 const &a, glm::vec3 const &b, glm::vec3 const &c, glm::u8vec4 const &color = glm::u8vec4(0xff));
	void fill_polygon(std::vector< glm::vec3 > const &points, glm::u8vec4 const &color = glm::u8vec4(0xff));
	void fill_rect(glm::vec2 const &min, glm::vec2 const &max, glm::u8vec4 const &color = glm::u8vec4(0xff));
	void fill_line(glm::vec2 const &a, glm::vec2 const &b, float width, glm::u8vec4 const &color = glm::u8vec4(0xff));
	void fill_circle(glm::vec2 const &center, float radius, glm::u8vec4 const &color = glm::u8vec4(0xff), uint32_t segments = 48);

	//Finish drawing (push attribs to GPU):
	~DrawLines();


	glm::mat4 world_to_clip;
	struct Vertex {
		Vertex(glm::vec3 const &Position_, glm::u8vec4 const &Color_) : Position(Position_), Color(Color_) { }
		glm::vec3 Position;
		glm::u8vec4 Color;
	};
	std::vector< Vertex > attribs;
	std::vector< Vertex > triangle_attribs;

};

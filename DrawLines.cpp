#include "DrawLines.hpp"
#include "PathFont.hpp"
#include "ColorProgram.hpp"

#include "gl_errors.hpp"

#include <glm/gtc/type_ptr.hpp>

//All DrawLines instances share a vertex array object and vertex buffer, initialized at load time:

//n.b. declared static so they don't conflict with similarly named global variables elsewhere:
static GLuint vertex_buffer = 0;
static GLuint vertex_buffer_for_color_program = 0;

static Load< void > setup_buffers(LoadTagDefault, [](){
	//you may recognize this init code from DrawSprites.cpp:

	{ //set up vertex buffer:
		glGenBuffers(1, &vertex_buffer);
		//for now, buffer will be un-filled.
	}

	{ //vertex array mapping buffer for color_program:
		//ask OpenGL to fill vertex_buffer_for_color_program with the name of an unused vertex array object:
		glGenVertexArrays(1, &vertex_buffer_for_color_program);

		//set vertex_buffer_for_color_program as the current vertex array object:
		glBindVertexArray(vertex_buffer_for_color_program);

		//set vertex_buffer as the source of glVertexAttribPointer() commands:
		glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);

		//set up the vertex array object to describe arrays of PongMode::Vertex:
		glVertexAttribPointer(
			color_program->Position_vec4, //attribute
			3, //size
			GL_FLOAT, //type
			GL_FALSE, //normalized
			sizeof(DrawLines::Vertex), //stride
			(GLbyte *)0 + offsetof(DrawLines::Vertex, Position) //offset
		);
		glEnableVertexAttribArray(color_program->Position_vec4);
		//[Note that it is okay to bind a vec3 input to a vec4 attribute -- the w component will be filled with 1.0 automatically]

		glVertexAttribPointer(
			color_program->Color_vec4, //attribute
			4, //size
			GL_UNSIGNED_BYTE, //type
			GL_TRUE, //normalized
			sizeof(DrawLines::Vertex), //stride
			(GLbyte *)0 + offsetof(DrawLines::Vertex, Color) //offset
		);
		glEnableVertexAttribArray(color_program->Color_vec4);

		//done referring to vertex_buffer, so unbind it:
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		//done setting up vertex array object, so unbind it:
		glBindVertexArray(0);
	}

	GL_ERRORS(); //PARANOIA: make sure nothing strange happened during setup
});


DrawLines::DrawLines(glm::mat4 const &world_to_clip_) : world_to_clip(world_to_clip_) {
}

void DrawLines::draw(glm::vec3 const &a, glm::vec3 const &b, glm::u8vec4 const &color) {
	attribs.emplace_back(a, color);
	attribs.emplace_back(b, color);
}

void DrawLines::draw_box(glm::mat4x3 const &mat, glm::u8vec4 const &color) {
	//draw cube as three edge sets:

	draw(mat * glm::vec4(-1.0f,-1.0f,-1.0f, 1.0f), mat * glm::vec4( 1.0f,-1.0f,-1.0f, 1.0f), color);
	draw(mat * glm::vec4(-1.0f, 1.0f,-1.0f, 1.0f), mat * glm::vec4( 1.0f, 1.0f,-1.0f, 1.0f), color);
	draw(mat * glm::vec4(-1.0f,-1.0f, 1.0f, 1.0f), mat * glm::vec4( 1.0f,-1.0f, 1.0f, 1.0f), color);
	draw(mat * glm::vec4(-1.0f, 1.0f, 1.0f, 1.0f), mat * glm::vec4( 1.0f, 1.0f, 1.0f, 1.0f), color);

	draw(mat * glm::vec4(-1.0f,-1.0f,-1.0f, 1.0f), mat * glm::vec4(-1.0f, 1.0f,-1.0f, 1.0f), color);
	draw(mat * glm::vec4( 1.0f,-1.0f,-1.0f, 1.0f), mat * glm::vec4( 1.0f, 1.0f,-1.0f, 1.0f), color);
	draw(mat * glm::vec4(-1.0f,-1.0f, 1.0f, 1.0f), mat * glm::vec4(-1.0f, 1.0f, 1.0f, 1.0f), color);
	draw(mat * glm::vec4( 1.0f,-1.0f, 1.0f, 1.0f), mat * glm::vec4( 1.0f, 1.0f, 1.0f, 1.0f), color);
	
	draw(mat * glm::vec4(-1.0f,-1.0f,-1.0f, 1.0f), mat * glm::vec4(-1.0f,-1.0f, 1.0f, 1.0f), color);
	draw(mat * glm::vec4( 1.0f,-1.0f,-1.0f, 1.0f), mat * glm::vec4( 1.0f,-1.0f, 1.0f, 1.0f), color);
	draw(mat * glm::vec4(-1.0f, 1.0f,-1.0f, 1.0f), mat * glm::vec4(-1.0f, 1.0f, 1.0f, 1.0f), color);
	draw(mat * glm::vec4( 1.0f, 1.0f,-1.0f, 1.0f), mat * glm::vec4( 1.0f, 1.0f, 1.0f, 1.0f), color);
}

void DrawLines::draw_text(std::string const &text, glm::vec3 const &anchor_in, glm::vec3 const &x, glm::vec3 const &y, glm::u8vec4 const &color, glm::vec3 *anchor_out) {

	glm::vec3 anchor = anchor_in;

	uint32_t start = 0;
	while (start < text.size()) {
		uint32_t end = start;
		uint32_t glyph = -1U;
		while (end < text.size()) {
			end += 1;
			auto f = PathFont::font.glyph_map.find(text.substr(start, end-start));
			if (f == PathFont::font.glyph_map.end()) {
				end -= 1;
				break;
			}
			glyph = f->second;
		}
		if (glyph == -1U) {
			assert(start == end);
			end += 1;
			//missing! draw a tofu:
			for (const auto &pt : {
				glm::vec2(0.1f, 0.1f), glm::vec2(0.6f, 0.1f),
				glm::vec2(0.6f, 0.1f), glm::vec2(0.6f, 0.9f),
				glm::vec2(0.9f, 0.6f), glm::vec2(0.1f, 0.9f),
				glm::vec2(0.1f, 0.9f), glm::vec2(0.1f, 0.1f)
			}) {
				attribs.emplace_back(anchor + pt.x * x + pt.y * y, color);
			}
			anchor += x * 0.6f;
		} else {
			for (uint32_t c = PathFont::font.glyph_coord_starts[glyph]; c + 1 < PathFont::font.glyph_coord_starts[glyph+1]; c += 2) {
				attribs.emplace_back(anchor + x * PathFont::font.coords[c] + y * PathFont::font.coords[c+1], color);
			}
			anchor += x * PathFont::font.glyph_widths[glyph];
		}
		start = end;
	}

	if (anchor_out) *anchor_out = anchor;
}

void DrawLines::fill_triangle(glm::vec3 const &a, glm::vec3 const &b, glm::vec3 const &c, glm::u8vec4 const &color) {
	triangle_attribs.emplace_back(a, color);
	triangle_attribs.emplace_back(b, color);
	triangle_attribs.emplace_back(c, color);
}

void DrawLines::fill_polygon(std::vector< glm::vec3 > const &points, glm::u8vec4 const &color) {
	for (size_t i = 1; i + 1 < points.size(); ++i) {
		fill_triangle(points[0], points[i], points[i+1], color);
	}
}

void DrawLines::fill_rect(glm::vec2 const &min, glm::vec2 const &max, glm::u8vec4 const &color) {
	fill_polygon({
		glm::vec3(min.x, min.y, 0.0f), glm::vec3(max.x, min.y, 0.0f),
		glm::vec3(max.x, max.y, 0.0f), glm::vec3(min.x, max.y, 0.0f)
	}, color);
}

void DrawLines::fill_line(glm::vec2 const &a, glm::vec2 const &b, float width, glm::u8vec4 const &color) {
	if (a == b) return;
	glm::vec2 n = 0.5f * width * glm::normalize(glm::vec2(a.y - b.y, b.x - a.x));
	fill_polygon({
		glm::vec3(a - n, 0.0f), glm::vec3(b - n, 0.0f),
		glm::vec3(b + n, 0.0f), glm::vec3(a + n, 0.0f)
	}, color);
}

void DrawLines::fill_circle(glm::vec2 const &center, float radius, glm::u8vec4 const &color, uint32_t segments) {
	std::vector< glm::vec3 > points;
	for (uint32_t i = 0; i < segments; ++i) {
		float a = i / float(segments) * 2.0f * float(M_PI);
		points.emplace_back(center.x + radius * std::cos(a), center.y + radius * std::sin(a), 0.0f);
	}
	fill_polygon(points, color);
}

DrawLines::~DrawLines() {
	if (attribs.empty() && triangle_attribs.empty()) return;

	//based on DrawSprites.cpp :

	//set color_program as current program:
	glUseProgram(color_program->program);

	//upload OBJECT_TO_CLIP to the proper uniform location:
	glUniformMatrix4fv(color_program->OBJECT_TO_CLIP_mat4, 1, GL_FALSE, glm::value_ptr(world_to_clip));

	//use the mapping vertex_buffer_for_color_program to fetch vertex data:
	glBindVertexArray(vertex_buffer_for_color_program);

	auto draw_array = [](std::vector< Vertex > const &verts, GLenum mode) {
		if (verts.empty()) return;
		//upload vertices to vertex_buffer:
		glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer); //set vertex_buffer as current
		glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(verts[0]), verts.data(), GL_STREAM_DRAW); //upload attribs array
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		//run the OpenGL pipeline:
		glDrawArrays(mode, 0, GLsizei(verts.size()));
	};
	draw_array(triangle_attribs, GL_TRIANGLES);
	draw_array(attribs, GL_LINES);

	//reset vertex array to none:
	glBindVertexArray(0);

	//reset current program to none:
	glUseProgram(0);
}



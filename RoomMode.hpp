#pragma once

#include "PPU466.hpp"
#include "Mode.hpp"
#include "PixelCanvas.hpp"
#include "SoulCaptureMode.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <string>

struct RoomMode : Mode {
	RoomMode();
	virtual ~RoomMode();

	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;

	struct Button {
		uint8_t downs = 0;
		uint8_t pressed = 0;
	} left, right, up, down, interact;

	glm::vec2 player_at = glm::vec2(128.0f, 60.0f);

	struct Interactable {
		std::string name;
		glm::vec2 pos;
		float radius;
	};
	std::vector<Interactable> interactables;
	int nearby = -1;

	bool tile_is_solid(int tx, int ty);
	bool can_move_to(glm::vec2 pos);
	void on_interact(Interactable const &thing);

	PPU466 ppu;

	float hover_time = 0.0f;

	PixelCanvas canvas;
	std::string hint = "WASD+E. Read the diary first, or just reap the patient.";
	bool read_diary = false;
	std::shared_ptr< SoulCaptureMode > capture;
};
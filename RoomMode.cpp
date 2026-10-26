#include "RoomMode.hpp"

#include "DrawLines.hpp"

#include <iostream>
#include <cmath>
#include <cstring>

const uint8_t FLOOR_TILE = 1;
const uint8_t WALL_TILE = 2;
const uint8_t BED_TILE = 3;
const uint8_t TABLE_TILE = 4;
const uint8_t DOOR_TILE = 5;
const uint8_t REAPER_TILE = 6;
const uint8_t ALERT_TILE = 7;

const uint8_t ROOM_PALETTE = 0;
const uint8_t REAPER_PALETTE = 1;
const uint8_t ALERT_PALETTE = 2;

static const char *ROOM_MAP[30] = {
	"################################",
	"#..............................#",
	"#..............................#",
	"#.............BBBB.............#",
	"#.............BBBB.............#",
	"#.............BBBB..TT.........#",
	"#.............BBBB..TT.........#",
	"#.............BBBB.............#",
	"#.............BBBB.............#",
	"#...............P....D.........#",
	"#..............................#",
	"#..............................#",
	"#..............................#",
	"#..............................#",
	"#..............................#",
	"#..............................#",
	"#..............................#",
	"#..............................#",
	"#..............................#",
	"#..............................#",
	"#..............................#",
	"#..............................#",
	"#..............................#",
	"#..............................#",
	"#..............................#",
	"#..............................#",
	"#..............................#",
	"#..............................#",
	"#..............................#",
	"###############==###############",
};

static PPU466::Tile solid_tile(uint8_t color) {
	PPU466::Tile t;
	for (int y = 0; y < 8; y++) {
		if (color & 1) {
			t.bit0[y] = 0xff;
		} else {
			t.bit0[y] = 0x00;
		}
		if (color & 2) {
			t.bit1[y] = 0xff;
		} else {
			t.bit1[y] = 0x00;
		}
	}
	return t;
}

RoomMode::RoomMode() {
	//--- palettes ---
	ppu.palette_table[ROOM_PALETTE] = {
		glm::u8vec4(0x00, 0x00, 0x00, 0x00),
		glm::u8vec4(0xd8, 0xe4, 0xe0, 0xff),
		glm::u8vec4(0x60, 0x70, 0x78, 0xff),
		glm::u8vec4(0xff, 0xff, 0xff, 0xff),
	};
	ppu.palette_table[REAPER_PALETTE] = {
		glm::u8vec4(0x00, 0x00, 0x00, 0x00),
		glm::u8vec4(0x20, 0x10, 0x30, 0xff),
		glm::u8vec4(0x00, 0x00, 0x00, 0xff),
		glm::u8vec4(0x00, 0x00, 0x00, 0xff),
	};
	ppu.palette_table[ALERT_PALETTE] = {
		glm::u8vec4(0x00, 0x00, 0x00, 0x00),
		glm::u8vec4(0xff, 0xd0, 0x20, 0xff),
		glm::u8vec4(0x00, 0x00, 0x00, 0xff),
		glm::u8vec4(0x00, 0x00, 0x00, 0xff),
	};

	ppu.tile_table[FLOOR_TILE] = solid_tile(1);
	ppu.tile_table[WALL_TILE] = solid_tile(2);
	ppu.tile_table[BED_TILE] = solid_tile(3);
	ppu.tile_table[TABLE_TILE] = solid_tile(3);
	ppu.tile_table[DOOR_TILE] = solid_tile(2);
	ppu.tile_table[REAPER_TILE] = solid_tile(1);
	ppu.tile_table[ALERT_TILE] = solid_tile(1);

	for (uint32_t i = 0; i < PPU466::BackgroundWidth * PPU466::BackgroundHeight; i++) {
		ppu.background[i] = FLOOR_TILE | (ROOM_PALETTE << 8);
	}

	for (int row = 0; row < 30; row++) {
		if (std::strlen(ROOM_MAP[row]) != 32) {
			throw std::runtime_error("ROOM_MAP row " + std::to_string(row) + " is not 32 characters long.");
		}

		int y = 29 - row;
		for (int x = 0; x < 32; x++) {
			char c = ROOM_MAP[row][x];
			uint8_t tile = FLOOR_TILE;

			if (c == '#') {
				tile = WALL_TILE;
			} else if (c == 'B') {
				tile = BED_TILE;
			} else if (c == 'T') {
				tile = TABLE_TILE;
			} else if (c == '=') {
				tile = DOOR_TILE;
			} else if (c == 'P') {
				Interactable patient;
				patient.name = "patient";
				patient.pos = glm::vec2(x * 8.0f + 4.0f, y * 8.0f + 4.0f);
				patient.radius = 20.0f;
				interactables.push_back(patient);
			} else if (c == 'D') {
				Interactable diary;
				diary.name = "diary";
				diary.pos = glm::vec2(x * 8.0f + 4.0f, y * 8.0f + 4.0f);
				diary.radius = 20.0f;
				interactables.push_back(diary);
			}

			ppu.background[x + PPU466::BackgroundWidth * y] = tile | (ROOM_PALETTE << 8);
		}
	}
}

RoomMode::~RoomMode() {
}

bool RoomMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {
	(void)window_size;

	if (evt.type == SDL_EVENT_KEY_DOWN) {
		if (evt.key.key == SDLK_A) {
			left.downs += 1;
			left.pressed = true;
			return true;
		} else if (evt.key.key == SDLK_D) {
			right.downs += 1;
			right.pressed = true;
			return true;
		} else if (evt.key.key == SDLK_W) {
			up.downs += 1;
			up.pressed = true;
			return true;
		} else if (evt.key.key == SDLK_S) {
			down.downs += 1;
			down.pressed = true;
			return true;
		} else if (evt.key.key == SDLK_E || evt.key.key == SDLK_SPACE) {
			interact.downs += 1;
			interact.pressed = true;
			return true;
		}
	} else if (evt.type == SDL_EVENT_KEY_UP) {
		if (evt.key.key == SDLK_A) {
			left.pressed = false;
			return true;
		} else if (evt.key.key == SDLK_D) {
			right.pressed = false;
			return true;
		} else if (evt.key.key == SDLK_W) {
			up.pressed = false;
			return true;
		} else if (evt.key.key == SDLK_S) {
			down.pressed = false;
			return true;
		} else if (evt.key.key == SDLK_E || evt.key.key == SDLK_SPACE) {
			interact.pressed = false;
			return true;
		}
	}

	return false;
}

bool RoomMode::tile_is_solid(int tx, int ty) {
	if (tx < 0 || ty < 0 || tx >= 32 || ty >= 30) {
		return true;
	}
	uint8_t tile = ppu.background[tx + PPU466::BackgroundWidth * ty] & 0xff;
	if (tile == WALL_TILE || tile == BED_TILE || tile == TABLE_TILE || tile == DOOR_TILE) {
		return true;
	}
	return false;
}

bool RoomMode::can_move_to(glm::vec2 pos) {
	float left_x = pos.x;
	float right_x = pos.x + 7.0f;
	float bottom_y = pos.y;
	float top_y = pos.y + 7.0f;

	if (tile_is_solid(int(left_x) / 8, int(bottom_y) / 8)) {
		return false;
	}
	if (tile_is_solid(int(right_x) / 8, int(bottom_y) / 8)) {
		return false;
	}
	if (tile_is_solid(int(left_x) / 8, int(top_y) / 8)) {
		return false;
	}
	if (tile_is_solid(int(right_x) / 8, int(top_y) / 8)) {
		return false;
	}
	return true;
}

void RoomMode::on_interact(Interactable const &thing) {
	if (thing.name == "patient" && !capture) {
		auto self = shared_from_this();
		capture = std::make_shared< SoulCaptureMode >(read_diary ? SoulCaptureMode::normal() : SoulCaptureMode::hard());
		capture->return_to = self;
		Mode::set_current(capture);
	} else if (thing.name == "diary") {
		read_diary = true;
		hint = "You read the diary. The patient seems calmer now.";
	}
}

void RoomMode::update(float elapsed) {
	const float PlayerSpeed = 60.0f;

	glm::vec2 move = glm::vec2(0.0f);
	if (left.pressed) {
		move.x -= 1.0f;
	}
	if (right.pressed) {
		move.x += 1.0f;
	}
	if (down.pressed) {
		move.y -= 1.0f;
	}
	if (up.pressed) {
		move.y += 1.0f;
	}
	if (move != glm::vec2(0.0f)) {
		move = glm::normalize(move) * PlayerSpeed * elapsed;
	}

	glm::vec2 try_x = glm::vec2(player_at.x + move.x, player_at.y);
	if (can_move_to(try_x)) {
		player_at.x = try_x.x;
	}
	glm::vec2 try_y = glm::vec2(player_at.x, player_at.y + move.y);
	if (can_move_to(try_y)) {
		player_at.y = try_y.y;
	}

	nearby = -1;
	float best_dist = 1e9f;
	glm::vec2 player_center = player_at + glm::vec2(4.0f, 4.0f);
	for (size_t i = 0; i < interactables.size(); i++) {
		float dist = glm::length(interactables[i].pos - player_center);
		if (dist < interactables[i].radius && dist < best_dist) {
			best_dist = dist;
			nearby = int(i);
		}
	}

	if (interact.downs > 0 && nearby != -1) {
		on_interact(interactables[nearby]);
	}

	hover_time = hover_time + elapsed;
	if (hover_time > 1000.0f) {
		hover_time = hover_time - 1000.0f;
	}

	left.downs = 0;
	right.downs = 0;
	up.downs = 0;
	down.downs = 0;
	interact.downs = 0;
}

void RoomMode::draw(glm::uvec2 const &drawable_size) {
	ppu.sprites[0].x = int8_t(player_at.x);
	ppu.sprites[0].y = int8_t(player_at.y);
	ppu.sprites[0].index = REAPER_TILE;
	ppu.sprites[0].attributes = REAPER_PALETTE;

	if (nearby != -1) {
		ppu.sprites[1].x = int8_t(player_at.x);
		ppu.sprites[1].y = int8_t(player_at.y + 10.0f);
		ppu.sprites[1].index = ALERT_TILE;
		ppu.sprites[1].attributes = ALERT_PALETTE;
	} else {
		ppu.sprites[1].y = 240;
	}

	for (uint32_t i = 2; i < 64; i++) {
		ppu.sprites[i].y = 240;
	}

	const float HoverHeight = 2.0f;
	const float HoverSpeed = 3.0f;
	float hover = HoverHeight * std::sin(hover_time * HoverSpeed);

	ppu.sprites[0].x = int8_t(player_at.x);
	ppu.sprites[0].y = int8_t(player_at.y + hover);
	ppu.sprites[0].index = REAPER_TILE;
	ppu.sprites[0].attributes = REAPER_PALETTE;

	if (nearby != -1) {
		ppu.sprites[1].x = int8_t(player_at.x);
		ppu.sprites[1].y = int8_t(player_at.y + 10.0f + hover);
		ppu.sprites[1].index = ALERT_TILE;
		ppu.sprites[1].attributes = ALERT_PALETTE;
	} else {
		ppu.sprites[1].y = 240;
	}

	if (capture) hint = "Soul collected in " + std::to_string(int(capture->timer)) + "s with " + std::to_string(capture->misses) + " misses.";
	canvas.begin(glm::vec3(0.0f));
	glDisable(GL_FRAMEBUFFER_SRGB);
	ppu.draw(glm::uvec2(PixelCanvas::Width, PixelCanvas::Height));
	glEnable(GL_FRAMEBUFFER_SRGB);
	{
		DrawLines lines(canvas.world_to_clip());
		lines.fill_rect(glm::vec2(-PixelCanvas::Aspect, -1.0f), glm::vec2(PixelCanvas::Aspect, -0.86f), glm::u8vec4(0x10, 0x10, 0x18, 0xff));
		lines.draw_text(hint, glm::vec3(-PixelCanvas::Aspect + 0.05f, -0.97f, 0.0f), glm::vec3(0.075f, 0.0f, 0.0f), glm::vec3(0.0f, 0.075f, 0.0f), glm::u8vec4(0xff, 0xff, 0xff, 0xff));
	}
	canvas.end(drawable_size);
}
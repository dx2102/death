#pragma once

#include "Mode.hpp"
#include "PixelCanvas.hpp"

#include <glm/glm.hpp>

#include <cmath>

#include <random>
#include <string>
#include <vector>

struct SoulCaptureMode : Mode {
	struct Difficulty {
		char const *name = "normal";
		float lifeline_spin = 0.0f;
		float wander_radius = 0.05f;
		float wander_speed = 0.6f;
	};
	static Difficulty normal() { return Difficulty{ "normal", 0.0f, 0.05f, 0.6f }; }
	static Difficulty hard() { return Difficulty{ "hard", 1.2f, 0.12f, 1.3f }; }
	static constexpr float AngleTolerance = 0.2618f;
	static constexpr uint32_t HitsNeeded = 5;

	SoulCaptureMode(Difficulty const &difficulty);
	virtual ~SoulCaptureMode();

	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;

	void restart(Difficulty const &difficulty);

	struct SliceCheck {
		bool crosses = false;
		float angle_error = 0.0f;
		bool hit = false;
		glm::vec2 point = glm::vec2(0.0f);
	};
	SliceCheck check_slice(glm::vec2 a, glm::vec2 b) const;

	void slice();

	Difficulty difficulty;
	std::mt19937 mt;

	glm::vec2 soul_center = glm::vec2(0.0f);
	float soul_radius = 0.08f;
	glm::vec2 soul_offset = glm::vec2(0.0f);
	glm::vec2 soul_velocity = glm::vec2(0.0f);
	glm::vec2 wander_phase = glm::vec2(0.0f);
	float clock = 0.0f;
	glm::vec2 wander() const;
	glm::vec2 soul_position() const { return soul_center + soul_offset + wander(); }
	bool move_pending = false;
	static constexpr float ArenaRadius = 0.8f;
	static constexpr float HomeRadius = 0.4f;
	float keep_out_radius = 0.3f;
	float lifeline_angle = 0.0f;
	float lifeline_half_length = 0.25f;
	glm::vec2 lifeline_dir() const { return glm::vec2(std::cos(lifeline_angle), std::sin(lifeline_angle)); }
	glm::vec2 lifeline_end(float side) const { return soul_position() + side * lifeline_half_length * lifeline_dir(); }
	uint32_t hits = 0;
	uint32_t misses = 0;
	float streak_broken_time = 0.0f;

	glm::vec2 mouse = glm::vec2(0.0f, -0.5f);
	glm::vec2 pivot = glm::vec2(0.0f, -0.5f);
	float blade_angle = 0.0f;
	bool rotate_left = false;
	bool rotate_right = false;

	float slice_time = 10.0f;
	static constexpr float SlashSpeed = 4.0f;
	bool slash_reached_lifeline() const {
		return slice_time * SlashSpeed >= glm::length(slice_check.point - slice_a);
	}
	SliceCheck slice_check;
	bool knocked = false;
	glm::vec2 slice_a = glm::vec2(0.0f), slice_b = glm::vec2(0.0f);

	struct Spark {
		glm::vec2 position;
		glm::vec2 velocity;
		float age;
		float radius;
	};
	std::vector< Spark > sparks;
	static constexpr float SparkLifetime = 0.45f;

	struct Particle {
		glm::vec2 position;
		glm::vec2 velocity;
		float age;
		float lifetime;
		float radius;
	};
	std::vector< Particle > particles;
	uint32_t burst_pending = 0;
	void spawn_particles(uint32_t count, glm::vec2 a, glm::vec2 b, float spread, glm::vec2 push);

	std::string message;
	float message_time = 0.0f;

	float cooldown = 0.0f;
	float timer = 0.0f;
	bool captured = false;

	PixelCanvas canvas;
	std::shared_ptr< Mode > return_to;

	float reach = 0.6f;
	float rotate_speed = 3.0f;
	float slice_cooldown = 0.4f;
};

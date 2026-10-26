#include "SoulCaptureMode.hpp"

#include "DrawLines.hpp"
#include "gl_errors.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <string>

static glm::vec3 const Background = glm::vec3(0.02f, 0.02f, 0.03f);
static glm::u8vec4 const Green = glm::u8vec4(0x44, 0xff, 0x44, 0xff);
static glm::u8vec4 const Red = glm::u8vec4(0xff, 0x44, 0x44, 0xff);
static glm::u8vec4 const Orange = glm::u8vec4(0xff, 0x88, 0x22, 0xff);

glm::vec2 SoulCaptureMode::wander() const {
	float t = clock * difficulty.wander_speed;
	glm::vec2 loop = glm::vec2(std::cos(t + wander_phase.x), std::sin(t + wander_phase.x));
	glm::vec2 wobble = glm::vec2(std::cos(2.3f * t + wander_phase.y), std::sin(1.7f * t + wander_phase.y));
	return difficulty.wander_radius * (loop + 0.5f * wobble) / 1.5f;
}

SoulCaptureMode::SoulCaptureMode(Difficulty const &difficulty_) : difficulty(difficulty_), mt(std::random_device()()) {
	std::uniform_real_distribution< float > turn(0.0f, 2.0f * float(M_PI));
	wander_phase = glm::vec2(turn(mt), turn(mt));
	lifeline_angle = 0.5f * turn(mt);
}

SoulCaptureMode::~SoulCaptureMode() {
}

void SoulCaptureMode::restart(Difficulty const &difficulty_) {
	auto next = std::make_shared< SoulCaptureMode >(difficulty_);
	next->mouse = mouse;
	next->blade_angle = blade_angle;
	next->canvas.rect = canvas.rect;
	Mode::set_current(next);
}

static bool segments_cross(glm::vec2 a, glm::vec2 b, glm::vec2 c, glm::vec2 d, float *t) {
	auto side = [](glm::vec2 r, glm::vec2 p, glm::vec2 q) {
		return (q.x - p.x) * (r.y - p.y) - (q.y - p.y) * (r.x - p.x);
	};
	float side_a = side(a, c, d), side_b = side(b, c, d);
	if (!(side_a * side_b < 0.0f && side(c, a, b) * side(d, a, b) < 0.0f)) return false;
	*t = side_a / (side_a - side_b);
	return true;
}

SoulCaptureMode::SliceCheck SoulCaptureMode::check_slice(glm::vec2 a, glm::vec2 b) const {
	SliceCheck ret;
	float t = 0.0f;
	ret.crosses = segments_cross(a, b, lifeline_end(-1.0f), lifeline_end(1.0f), &t);
	if (ret.crosses) ret.point = a + t * (b - a);
	float angle = std::acos(std::min(1.0f, std::abs(glm::dot(glm::normalize(b - a), lifeline_dir()))));
	ret.angle_error = 0.5f * float(M_PI) - angle;
	ret.hit = ret.crosses && ret.angle_error < AngleTolerance;
	return ret;
}

void SoulCaptureMode::slice() {
	if (captured || cooldown > 0.0f) return;
	cooldown = slice_cooldown;

	glm::vec2 blade_dir = glm::vec2(std::cos(blade_angle), std::sin(blade_angle));
	slice_a = pivot;
	slice_b = pivot + reach * blade_dir;

	SliceCheck check = check_slice(slice_a, slice_b);
	slice_check = check;
	slice_time = 0.0f;
	knocked = false;

	{
		constexpr uint32_t Count = 24;
		std::uniform_real_distribution< float > jitter(-0.12f, 0.12f);
		glm::vec2 side_dir = glm::vec2(-blade_dir.y, blade_dir.x);
		for (uint32_t i = 0; i < Count; ++i) {
			Spark s;
			s.position = slice_a;
			s.velocity = SlashSpeed * blade_dir + jitter(mt) * side_dir;
			s.age = -0.004f * i;
			s.radius = glm::mix(0.016f, 0.005f, i / float(Count - 1));
			sparks.emplace_back(s);
		}
	}
	message_time = 2.5f;
	if (!check.crosses) {
		message = "Missed the lifeline.";
	} else if (!check.hit) {
		message = "Off by " + std::to_string(int(glm::degrees(check.angle_error))) + " degrees.";
	}
	if (!check.hit && hits > 0) message += " Streak reset.";

	if (slice_check.hit) {
		hits += 1;
		if (hits >= HitsNeeded) {
			captured = true;
			burst_pending = 80;
			message = return_to ? "Soul captured! Press Space to continue." : "Soul captured!  R: retry   1: normal   2: hard";
		} else {
			burst_pending = 12;
			message = "Hit! " + std::to_string(HitsNeeded - hits) + " more to go.";
		}
	} else {
		misses += 1;
		if (hits > 0) streak_broken_time = 0.6f;
		hits = 0;
	}
	if (!captured) move_pending = true;
}

bool SoulCaptureMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {
	bool go_on = evt.type == SDL_EVENT_MOUSE_BUTTON_DOWN || (evt.type == SDL_EVENT_KEY_DOWN && (evt.key.key == SDLK_SPACE || evt.key.key == SDLK_E));
	if (captured && return_to && go_on && slice_time > 1.0f) {
		auto self = shared_from_this();
		Mode::set_current(std::move(return_to));
		return true;
	}
	if (evt.type == SDL_EVENT_KEY_DOWN) {
		if (evt.key.key == SDLK_A) {
			rotate_left = true;
			return true;
		} else if (evt.key.key == SDLK_D) {
			rotate_right = true;
			return true;
		} else if (evt.key.key == SDLK_SPACE) {
			slice();
			return true;
		} else if (!return_to && (evt.key.key == SDLK_R || evt.key.key == SDLK_1 || evt.key.key == SDLK_2)) {
			auto self = shared_from_this();
			restart(evt.key.key == SDLK_1 ? normal() : evt.key.key == SDLK_2 ? hard() : difficulty);
			return true;
		}
	} else if (evt.type == SDL_EVENT_KEY_UP) {
		if (evt.key.key == SDLK_A) {
			rotate_left = false;
			return true;
		} else if (evt.key.key == SDLK_D) {
			rotate_right = false;
			return true;
		}
	} else if (evt.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
		if (evt.button.button == SDL_BUTTON_LEFT) {
			slice();
			return true;
		}
	} else if (evt.type == SDL_EVENT_MOUSE_MOTION) {
		mouse = canvas.from_window(glm::vec2(evt.motion.x, evt.motion.y), window_size);
		return true;
	}

	return false;
}

void SoulCaptureMode::update(float elapsed) {
	if (rotate_left) blade_angle += rotate_speed * elapsed;
	if (rotate_right) blade_angle -= rotate_speed * elapsed;

	clock += elapsed;

	if (move_pending && slice_time >= reach / SlashSpeed) {
		std::uniform_real_distribution< float > unit(0.0f, 1.0f);
		float angle = unit(mt) * 2.0f * float(M_PI);
		glm::vec2 home = HomeRadius * std::sqrt(unit(mt)) * glm::vec2(std::cos(angle), std::sin(angle));
		soul_offset -= home - soul_center;
		soul_center = home;
		move_pending = false;
	}

	pivot = mouse;
	glm::vec2 away = pivot - soul_position();
	if (glm::length(away) < keep_out_radius) {
		if (glm::length(away) < 1e-4f) away = glm::vec2(0.0f, -1.0f);
		pivot = soul_position() + keep_out_radius * glm::normalize(away);
	}

	cooldown = std::max(0.0f, cooldown - elapsed);
	streak_broken_time = std::max(0.0f, streak_broken_time - elapsed);
	message_time = std::max(0.0f, message_time - elapsed);

	slice_time += elapsed;
	if (burst_pending > 0 && slash_reached_lifeline()) {
		spawn_particles(burst_pending, lifeline_end(-1.0f), lifeline_end(1.0f), 0.01f, glm::normalize(slice_b - slice_a));
		burst_pending = 0;
	}
	{
		bool lifeline_there = !(captured && slash_reached_lifeline());
		glm::vec2 line_dir = lifeline_dir(), line_a = lifeline_end(-1.0f), line_b = lifeline_end(1.0f);
		for (auto &s : sparks) {
			s.age += elapsed;
			if (s.age < 0.0f) continue;
			float step = std::min(elapsed, s.age);
			glm::vec2 next = s.position + s.velocity * step;
			float t = 0.0f;
			if (lifeline_there && !slice_check.hit && segments_cross(s.position, next, line_a, line_b, &t)) {
				glm::vec2 at = s.position + t * (next - s.position);
				s.velocity = 2.0f * glm::dot(s.velocity, line_dir) * line_dir - s.velocity;
				next = at + s.velocity * ((1.0f - t) * step);
				if (!knocked) {
					soul_velocity += 1.2f * glm::normalize(slice_b - slice_a);
					knocked = true;
				}
			}
			s.position = next;
		}
		sparks.erase(std::remove_if(sparks.begin(), sparks.end(), [](Spark const &s) { return s.age >= SparkLifetime; }), sparks.end());
	}
	soul_velocity += (-200.0f * soul_offset - 14.0f * soul_velocity) * elapsed;
	soul_offset += soul_velocity * elapsed;

	for (auto &p : particles) {
		p.age += elapsed;
		p.position += p.velocity * elapsed;
		p.velocity *= std::exp(-2.0f * elapsed);
		p.velocity.y += 0.3f * elapsed;
	}
	particles.erase(std::remove_if(particles.begin(), particles.end(), [](Particle const &p) { return p.age >= p.lifetime; }), particles.end());

	if (!captured) {
		timer += elapsed;
		lifeline_angle += difficulty.lifeline_spin * std::sin(0.9f * clock + wander_phase.x) * elapsed;
	}
}

void SoulCaptureMode::spawn_particles(uint32_t count, glm::vec2 a, glm::vec2 b, float spread, glm::vec2 push) {
	std::uniform_real_distribution< float > unit(0.0f, 1.0f);
	for (uint32_t i = 0; i < count; ++i) {
		float angle = unit(mt) * 2.0f * float(M_PI);
		glm::vec2 dir = glm::vec2(std::cos(angle), std::sin(angle));
		Particle p;
		p.position = glm::mix(a, b, unit(mt)) + dir * spread * std::sqrt(unit(mt));
		p.velocity = dir * glm::mix(0.05f, 0.25f, unit(mt)) + push * glm::mix(0.3f, 1.0f, unit(mt));
		p.age = 0.0f;
		p.lifetime = glm::mix(0.6f, 1.4f, unit(mt));
		p.radius = glm::mix(0.008f, 0.022f, unit(mt));
		particles.emplace_back(p);
	}
}

void SoulCaptureMode::draw(glm::uvec2 const &drawable_size) {
	canvas.begin(Background);
	float aspect = PixelCanvas::Aspect;
	{
		DrawLines lines(canvas.world_to_clip());

		auto circle = [&](glm::vec2 center, float radius, glm::u8vec4 color) {
			for (uint32_t i = 0; i < 48; ++i) {
				float a0 = i / 48.0f * 2.0f * float(M_PI), a1 = (i + 1) / 48.0f * 2.0f * float(M_PI);
				lines.draw(glm::vec3(center + radius * glm::vec2(std::cos(a0), std::sin(a0)), 0.0f), glm::vec3(center + radius * glm::vec2(std::cos(a1), std::sin(a1)), 0.0f), color);
			}
		};

		glm::vec3 const background = Background * 255.0f;
		auto fade = [&](glm::u8vec4 color, float amount) {
			return glm::u8vec4(glm::mix(glm::vec3(color), background, glm::clamp(amount, 0.0f, 1.0f)), 0xff);
		};
		glm::u8vec4 const soul_color = glm::u8vec4(0x99, 0xdd, 0xff, 0xff);

		auto thick_line = [&](glm::vec2 a, glm::vec2 b, float width, glm::u8vec4 color) {
			lines.fill_line(a, b, width, color);
			lines.fill_circle(a, 0.5f * width, color, 12);
			lines.fill_circle(b, 0.5f * width, color, 12);
		};

		circle(glm::vec2(0.0f), ArenaRadius, glm::u8vec4(0x44, 0x44, 0x55, 0xff));
		circle(soul_position(), keep_out_radius, glm::u8vec4(0x2a, 0x2a, 0x36, 0xff));
		bool soul_gone = captured && slash_reached_lifeline();
		if (!soul_gone) {
			lines.fill_circle(soul_position(), soul_radius, soul_color);
		}

		for (auto const &p : particles) {
			float t = p.age / p.lifetime;
			glm::u8vec4 bright = glm::u8vec4(glm::mix(glm::vec3(soul_color), glm::vec3(255.0f), 0.6f * (1.0f - t)), 0xff);
			lines.fill_circle(p.position, p.radius * (1.0f - 0.5f * t), fade(bright, t), 8);
		}

		if (!soul_gone) {
			thick_line(lifeline_end(-1.0f), lifeline_end(1.0f), 0.02f, glm::u8vec4(0xff, 0xee, 0x88, 0xff));
			for (uint32_t i = 0; i < HitsNeeded; ++i) {
				float angle = 0.5f * float(M_PI) - i / float(HitsNeeded) * 2.0f * float(M_PI);
				glm::vec2 at = soul_position() + keep_out_radius * glm::vec2(std::cos(angle), std::sin(angle));
				if (streak_broken_time > 0.0f) {
					lines.fill_circle(at, 0.022f, fade(Red, 1.0f - streak_broken_time / 0.6f), 12);
				} else if (i < hits) {
					lines.fill_circle(at, 0.022f, Green, 12);
				} else {
					circle(at, 0.02f, glm::u8vec4(0x55, 0x55, 0x66, 0xff));
				}
			}
		}

		if (!captured) {
			glm::vec2 d = glm::vec2(std::cos(blade_angle), std::sin(blade_angle));
			SliceCheck check = check_slice(pivot, pivot + reach * d);
			glm::u8vec4 color = glm::u8vec4(0xff, 0xff, 0xff, 0xff);
			if (cooldown > 0.0f) color = glm::u8vec4(0x66, 0x66, 0x66, 0xff);
			else if (check.hit) color = Green;
			else if (check.crosses) {
				float off = (check.angle_error - AngleTolerance) / (0.5f * float(M_PI) - AngleTolerance);
				color = glm::u8vec4(glm::mix(glm::vec3(Orange), glm::vec3(Green), 0.7f * (1.0f - glm::clamp(off, 0.0f, 1.0f))), 0xff);
			}
			thick_line(pivot, pivot + reach * d, 0.017f, color);
			lines.fill_circle(pivot, 0.018f, color, 16);
		}

		for (auto const &s : sparks) {
			if (s.age < 0.0f) continue;
			float t = s.age / SparkLifetime;
			lines.fill_circle(s.position, s.radius * (1.0f - 0.6f * t), fade(slice_check.hit ? Green : Red, t * t), 8);
		}

		constexpr float H = 0.08f;
		auto text = [&](std::string const &str, float y, glm::u8vec4 color) {
			lines.draw_text(str, glm::vec3(-aspect + 0.1f, y, 0.0f), glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f), color);
		};
		char buf[128];
		snprintf(buf, sizeof(buf), "%s soul  misses %u  time %.1fs", difficulty.name, misses, timer);
		text(buf, 1.0f - 1.5f * H, glm::u8vec4(0xff, 0xff, 0xff, 0xff));
		snprintf(buf, sizeof(buf), "hits %u/%u", hits, HitsNeeded);
		lines.draw_text(buf, glm::vec3(aspect - 0.1f - 0.55f * H * std::strlen(buf), 1.0f - 1.5f * H, 0.0f), glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f), streak_broken_time > 0.0f ? Red : Green);
		if (captured || message_time > 0.0f) {
			glm::u8vec4 color = slice_check.hit ? Green : Orange;
			if (!captured) color = fade(color, 1.0f - message_time / 0.5f);
			text(message, 1.0f - 3.0f * H, color);
		}
		//text("Cut ACROSS the yellow lifeline. Mouse moves the scythe, A/D rotate it.", -1.0f + 2.0f * H, glm::u8vec4(0xaa, 0xaa, 0xaa, 0xff));
		//text("The blade turns green when a slice would hit: then press Space or click.", -1.0f + 0.5f * H, glm::u8vec4(0xaa, 0xaa, 0xaa, 0xff));
	}

	canvas.end(drawable_size);
	GL_ERRORS();
}

#pragma once

#include "Mode.hpp"

#include "MeshBuffer.hpp"
#include "WalkMesh.hpp"
#include "GL.hpp"
#include "Scene.hpp"
#include "Sound.hpp"

#include <SDL.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <vector>
#include <deque>
#include <random>

struct GameMode : public Mode {
	GameMode();
	virtual ~GameMode();

	//handle_event is called when new mouse or keyboard events are received:
	// (note that this might be many times per frame or never)
	//The function should return 'true' if it handled the event.
	virtual bool handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) override;

	//update is called at the start of a new frame, after events are handled:
	virtual void update(float elapsed) override;

	//draw is called after update:
	virtual void draw(glm::uvec2 const &drawable_size) override;

	//show voice line from phone or bring up calling menu:
	void activate_phone();

	uint32_t merits = 0;
	uint32_t demerits = 0;
	void add_merit();
	void add_demerit();

	struct {
		bool forward = false;
		bool backward = false;
		bool left = false;
		bool right = false;
	} controls;

	bool mouse_captured = false;

	struct {
		Scene::Transform *transform; //player is at transform's position, looking down y axis with x to the right and z up.
		WalkMesh::WalkPoint walkpoint;
		float elevation = 0.0f; //up/down angle of camera (in radians)
	} player;

	struct Phone {
		Scene::Object *object = nullptr;
		uint32_t index = 0;
		float ring_time = 0.0f;
		std::shared_ptr< Sound::PlayingSample > ring_loop;

		std::deque< Sound::Sample const * > play_queue; //used to assemble voice clips and such.
		std::shared_ptr< Sound::PlayingSample > playing;
	};

	struct Task {
		uint32_t phone = 0;
		uint32_t say = 0;
		bool operator==(Task const &o) {
			return phone == o.phone && say == o.say;
		}
	};

	std::vector< Task > tasks;

	std::vector< Phone > phones;
	Phone *close_phone = nullptr;

	Scene scene;
	Scene::Camera *camera = nullptr;

	float task_timer = 5.0f;

	std::mt19937 mt;
};

#include "GameMode.hpp"

#include "MenuMode.hpp"
#include "Load.hpp"
#include "Sound.hpp"
#include "MeshBuffer.hpp"
#include "gl_errors.hpp" //helper for dumpping OpenGL error messages
#include "read_chunk.hpp" //helper for reading a vector of structures from a file
#include "data_path.hpp" //helper to get paths relative to executable
#include "compile_program.hpp" //helper to compile opengl shader programs
#include "draw_text.hpp" //helper to... um.. draw text
#include "vertex_color_program.hpp"
#include "WalkMesh.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <fstream>
#include <map>
#include <cstddef>
#include <random>
#include <algorithm>
#include <chrono>

Load< Sound::Sample > sample_dot(LoadTagDefault, [](){
	return new Sound::Sample(data_path("dot.wav"));
});
Load< Sound::Sample > sample_loop(LoadTagDefault, [](){
	return new Sound::Sample(data_path("loop.wav"));
});

Load< MeshBuffer > phone_bank_meshes(LoadTagDefault, [](){
	return new MeshBuffer(data_path("phone-bank.pnc"));
});

Load< GLuint > phone_bank_meshes_for_vertex_color_program(LoadTagDefault, [](){
	return new GLuint(phone_bank_meshes->make_vao_for_program(vertex_color_program->program));
});

WalkMesh const *phone_bank_walkmesh = nullptr;

Load< WalkMeshes > phone_bank_walkmeshes(LoadTagDefault, [](){
	WalkMeshes *ret = new WalkMeshes(data_path("phone-bank.w"));
	phone_bank_walkmesh = &ret->lookup("WalkMesh");
	return ret;
});

std::vector< Scene::Object * > phone_bank_scene_phones;

Load< Scene > phone_bank_scene(LoadTagDefault, [](){
	Scene *ret = new Scene();
	ret->load(data_path("phone-bank.scene"), [](Scene &scene, Scene::Transform *transform, std::string const &mesh_name){
		Scene::Object *object = scene.new_object(transform);
		object->program = vertex_color_program->program;
		object->program_mvp_mat4 = vertex_color_program->object_to_clip_mat4;
		object->program_mv_mat4x3 = vertex_color_program->object_to_light_mat4x3;
		object->program_itmv_mat3 = vertex_color_program->normal_to_light_mat3;

		object->vao = *phone_bank_meshes_for_vertex_color_program;
		MeshBuffer::Mesh const &mesh = phone_bank_meshes->lookup(mesh_name);
		object->start = mesh.start;
		object->count = mesh.count;

		if (transform->name.substr(0, 5) == "Phone") {
			phone_bank_scene_phones.emplace_back(object);
		}
	});

	std::cout << "Scene has " << phone_bank_scene_phones.size() << " phones." << std::endl;

	assert(phone_bank_scene_phones.size() == 4);

	//sort phones by name so they end up in the same order [Black, Cyan, Magenta, White] regardless of storage order:
	std::sort(phone_bank_scene_phones.begin(), phone_bank_scene_phones.end(), [](Scene::Object *a,  Scene::Object *b){
		return a->transform->name < b->transform->name;
	});
	//Shuffle phones so they are in White, Black, Cyan, Magenta order (that's the sample order):
	std::swap(phone_bank_scene_phones[0], phone_bank_scene_phones[3]);
	std::swap(phone_bank_scene_phones[1], phone_bank_scene_phones[3]);
	std::swap(phone_bank_scene_phones[2], phone_bank_scene_phones[3]);
	assert(phone_bank_scene_phones[0]->transform->name == "Phone.White");
	assert(phone_bank_scene_phones[1]->transform->name == "Phone.Black");
	assert(phone_bank_scene_phones[2]->transform->name == "Phone.Cyan");
	assert(phone_bank_scene_phones[3]->transform->name == "Phone.Magenta");

	return ret;
});

struct Voice {
	std::vector< Sound::Sample > check;
	std::vector< Sound::Sample > task; //one per phone
	std::vector< std::vector< Sound::Sample > > say; //four per phone
};

std::vector< Voice > voices;


struct Ring {
	Ring(std::string const &n) :
		basic(data_path("samples/ring-" + n + ".wav")),
		strong(data_path("samples/ring-" + n + "-strong.wav")),
		end(data_path("samples/ring-" + n + "-end.wav")),
		click(data_path("samples/click-" + n + ".wav"))
	{ }
	Sound::Sample basic, strong, end, click;
};

std::vector< Ring > rings;

Load< int > load_samples(LoadTagDefault, []() -> int const *{
	voices.reserve(3);
	for (std::string v : {"A", "B", "C"}) {
		voices.emplace_back();
		voices.back().check.reserve(2);
		voices.back().check.emplace_back(data_path("samples/" + v + "-check-1.wav"));
		voices.back().check.emplace_back(data_path("samples/" + v + "-check-2.wav"));
		voices.back().task.reserve(2);
		for (uint32_t i = 0; i < 4; ++i) {
			voices.back().task.emplace_back(data_path("samples/" + v + "-task-" + std::to_string(i+1) + ".wav"));
		}
		voices.back().say.resize(4);
		for (uint32_t i = 0; i < 16; ++i) {
			voices.back().say[i/4].emplace_back(data_path("samples/" + v + "-say-" + std::to_string(i+1) + ".wav"));
		}
	}
	rings.reserve(4);
	for (uint32_t i = 0; i < 4; ++i) {
		rings.emplace_back(std::to_string(i+1));
	}
	return new int;
});

std::vector< std::vector< std::string > > choices = { //corresponding to the voice 'say' clips
	{"CATFISH", "PERCH", "BASS", "SALMON"},
	{"BENCH", "CHAIR", "DESK", "TABLE"},
	{"OAK", "MAPLE", "GINGKO", "SPRUCE"},
	{"KALE", "ROMAINE", "CABBAGE", "SPINACH"}
};


//init idea for mt from http://www.cplusplus.com/reference/random/mersenne_twister_engine/mersenne_twister_engine/
GameMode::GameMode() : mt(uint32_t(std::chrono::system_clock::now().time_since_epoch().count())) {
	//----------------
	//set up scene that holds player + camera (all other objects are in the loaded scene):

	//player starts at origin:
	player.transform = scene.new_transform();
	player.walkpoint = phone_bank_walkmesh->start(player.transform->position);

	{ //Camera is attached to player:
		Scene::Transform *transform = scene.new_transform();
		transform->set_parent(player.transform);
		transform->position = glm::vec3(0.0f, 0.0f, 1.6f); //1.6 units above the groundS
		transform->rotation = glm::angleAxis(0.5f * 3.14159f, glm::vec3(1.0f, 0.0f, 0.0f)); //rotate -z axis up to point along y axis
		camera = scene.new_camera(transform);
	}

	for (auto object : phone_bank_scene_phones) {
		phones.emplace_back();
		phones.back().index = uint32_t(phones.size()) - 1;
		phones.back().object = object;
	}
}

GameMode::~GameMode() {
}

bool GameMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {
	//ignore any keys that are the result of automatic key repeat:
	if (evt.type == SDL_KEYDOWN && evt.key.repeat) {
		return false;
	}
	//handle tracking the state of WSAD for movement control:
	if (evt.type == SDL_KEYDOWN || evt.type == SDL_KEYUP) {
		if (evt.key.keysym.scancode == SDL_SCANCODE_W) {
			controls.forward = (evt.type == SDL_KEYDOWN);
			return true;
		} else if (evt.key.keysym.scancode == SDL_SCANCODE_S) {
			controls.backward = (evt.type == SDL_KEYDOWN);
			return true;
		} else if (evt.key.keysym.scancode == SDL_SCANCODE_A) {
			controls.left = (evt.type == SDL_KEYDOWN);
			return true;
		} else if (evt.key.keysym.scancode == SDL_SCANCODE_D) {
			controls.right = (evt.type == SDL_KEYDOWN);
			return true;
		}
	}
	//handle tracking the mouse for rotation control:
	if (!mouse_captured) {
		if (evt.type == SDL_KEYDOWN && evt.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
			Mode::set_current(nullptr);
			return true;
		}
		if (evt.type == SDL_MOUSEBUTTONDOWN) {
			SDL_SetRelativeMouseMode(SDL_TRUE);
			mouse_captured = true;
			return true;
		}
	} else if (mouse_captured) {
		if (evt.type == SDL_KEYDOWN && evt.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
			SDL_SetRelativeMouseMode(SDL_FALSE);
			mouse_captured = false;
			return true;
		}
		if (evt.type == SDL_MOUSEMOTION) {
			//Note: float(window_size.y) * camera->fovy is a pixels-to-radians conversion factor
			float yaw = evt.motion.xrel / float(window_size.y) * camera->fovy;
			float pitch = evt.motion.yrel / float(window_size.y) * camera->fovy;
			yaw = -yaw;
			pitch = -pitch;

			//update camera elevation:
			const constexpr float PitchLimit = 90.0f / 180.0f * 3.1415926f;
			player.elevation = glm::clamp(player.elevation + pitch, -PitchLimit, PitchLimit);
			camera->transform->rotation = glm::angleAxis(player.elevation + 0.5f * 3.1515926f, glm::vec3(1.0f, 0.0f, 0.0f));

			//update player forward direction by rotation around 'up' direction:
			glm::vec3 up = phone_bank_walkmesh->world_normal(player.walkpoint);
			player.transform->rotation = glm::normalize(
				glm::angleAxis(yaw, up)
				* player.transform->rotation
			);

			return true;
		}
		if (evt.type == SDL_MOUSEBUTTONDOWN) {
			activate_phone();
			return true;
		}
	}
	return false;
}

void GameMode::activate_phone() {
	if (!close_phone) return;
	if (close_phone->ring_time > 0.0f) {
		close_phone->ring_time = 0.0f;
		if (close_phone->ring_loop) {
			close_phone->ring_loop->stop();
			close_phone->ring_loop.reset();
		}
		close_phone->play_queue.emplace_back(&rings[close_phone->index].click);
		//pick a task:
		Voice const &v = voices[mt() % voices.size()];
		if (mt() < mt.max() / 2) {
			//this was it:
			close_phone->play_queue.emplace_back(&v.check[mt() % v.check.size()]);

			add_merit();
		} else {
			//need to go to another phone
			Task task;
			task.phone = mt() % (v.task.size()-1);
			if (task.phone == close_phone->index) {
				task.phone += 1;
			}
			task.say = mt() % v.say[task.phone].size();

			close_phone->play_queue.emplace_back(&v.task[task.phone]);
			close_phone->play_queue.emplace_back(&v.say[task.phone][task.say]);

			tasks.emplace_back(task);
		}
		close_phone->play_queue.emplace_back(&rings[close_phone->index].click);
	} else {
		close_phone->play_queue.emplace_back(&rings[close_phone->index].click);

		std::shared_ptr< MenuMode > menu = std::make_shared< MenuMode >();

		menu->on_escape = [this](){
			Mode::set_current(shared_from_this());
		};
		menu->choices.emplace_back(close_phone->object->transform->name.substr(6) + " PHONE");
		menu->choices.emplace_back("HANG UP", [this](){
			Mode::set_current(shared_from_this());
		});
		for (uint32_t i = 0; i < choices[close_phone->index].size(); ++i) {
			Task t;
			t.phone = close_phone->index;
			t.say = i;
			menu->choices.emplace_back("SAY " + choices[close_phone->index][i], [this,t](){
				Mode::set_current(shared_from_this());
				auto f = std::find(tasks.begin(), tasks.end(), t);
				if (f != tasks.end()) {
					tasks.erase(f);
					add_merit();
				} else {
					add_demerit();
				}
			});
		}
		menu->selected = 1;


		menu->background = shared_from_this();

		Mode::set_current(menu);
	}
}

void GameMode::add_merit() {
	merits += 1;

	if (merits >= 10) {
		std::shared_ptr< MenuMode > menu = std::make_shared< MenuMode >();
		menu->choices.emplace_back("YOU WIN");
		menu->choices.emplace_back("EXIT", [](){
			Mode::set_current(nullptr);
		});
		menu->selected = 1;
		Mode::set_current(menu);
	}
}

void GameMode::add_demerit() {
	demerits += 1;

	if (demerits >= 3) {
		std::shared_ptr< MenuMode > menu = std::make_shared< MenuMode >();
		menu->choices.emplace_back("YOU LOSE");
		menu->choices.emplace_back("EXIT", [](){
			Mode::set_current(nullptr);
		});
		menu->selected = 1;
		Mode::set_current(menu);
	}
}

void GameMode::update(float elapsed) {
	//task spawning:
	task_timer -= elapsed;
	if (task_timer <= 0.0f) {
		//launch a new task:
		Phone &p = phones[mt() % phones.size()];
		if (p.ring_time <= 0.0f && p.play_queue.empty()) {
			p.ring_time = 10.0f + mt() / float(mt.max()) * 3.0f;
		}

		//reset spawn time:
		task_timer += 10.0f + mt() / float(mt.max()) * 5.0f;
	}

	{ //walk:
		glm::mat3 directions = glm::mat3_cast(player.transform->rotation);
		float amt = 5.0f * elapsed;
		glm::vec3 step = glm::vec3(0.0f);
		if (controls.left) step -= amt * directions[0];
		if (controls.right) step += amt * directions[0];
		if (controls.backward) step -= amt * directions[1];
		if (controls.forward) step += amt * directions[1];

		phone_bank_walkmesh->walk(player.walkpoint, step);

		//update position from walkmesh:
		player.transform->position = phone_bank_walkmesh->world_point(player.walkpoint);

		{ //update rotation from walkmesh:
			glm::vec3 old_up = directions[2];
			glm::vec3 new_up = phone_bank_walkmesh->world_normal(player.walkpoint);

			//shortest-arc rotation that takes old_up to new_up:
			//see: https://stackoverflow.com/questions/1171849/finding-quaternion-representing-the-rotation-from-one-vector-to-another
			glm::vec3 xyz = glm::cross(old_up, new_up);
			float w = 1.0f + glm::dot(old_up, new_up);
			glm::quat rot = glm::normalize(glm::quat(w, xyz));

			//apply rotation to 'right' direction:
			glm::vec3 new_right = rot * directions[0];

			//compute forward from up and right:
			glm::vec3 new_forward = glm::cross(new_up, new_right);

			//convert back into quaternion for storage in player transform:
			player.transform->rotation = glm::normalize(glm::quat_cast(glm::mat3(
				new_right,
				new_forward,
				new_up
			)));
		}
	}

	{ //update which phone is interactable (if any):
		close_phone = nullptr;
		glm::vec3 at = glm::vec3(camera->transform->make_local_to_world()[3]);
		glm::vec3 forward = -glm::vec3(camera->transform->make_local_to_world()[2]);
		for (auto &phone : phones) {
			glm::vec3 phone_at = glm::vec3(phone.object->transform->make_local_to_world()[3]);
			if (glm::length(phone_at - at) < 2.0f && glm::dot(phone_at - at, forward) > 0.2f) {
				close_phone = &phone;
			}
		}
	}

	{ //set sound positions:
		glm::mat4 cam_to_world = camera->transform->make_local_to_world();
		Sound::listener.set_position( cam_to_world[3] );
		//camera looks down -z, so right is +x:
		Sound::listener.set_right( glm::normalize(cam_to_world[0]) );
	}

	//update phone sounds:
	for (auto &p : phones) {
		glm::vec3 at = p.object->transform->make_local_to_world()[3];
		if (p.ring_time > 0.0f) {
			if (!p.ring_loop) {
				p.ring_loop = rings[p.index].basic.play(at, 1.0f, Sound::Loop);
			}
			p.ring_time -= elapsed;
			if (p.ring_time <= 4.0f && &p.ring_loop->data != &rings[p.index].strong.data) {
				p.ring_loop->stop();
				p.ring_loop = rings[p.index].strong.play(at, 1.0f, Sound::Loop);
			}
			if (p.ring_time <= 0.0f) {
				p.ring_loop->stop();
				p.ring_loop.reset();

				p.ring_time = 0.0f;

				rings[p.index].end.play(at, 1.0f, Sound::Once);

				add_demerit();
			}
		}
		if (p.playing && p.playing->stopped) p.playing.reset();
		if (!p.playing && !p.play_queue.empty()) {
			p.playing = p.play_queue.front()->play(at, 1.0f, Sound::Once);
			p.play_queue.pop_front();
		}
	}
}

void GameMode::draw(glm::uvec2 const &drawable_size) {
	//set up basic OpenGL state:
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendEquation(GL_FUNC_ADD);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	//set up light position + color:
	glUseProgram(vertex_color_program->program);
	glUniform3fv(vertex_color_program->sun_color_vec3, 1, glm::value_ptr(glm::vec3(0.81f, 0.81f, 0.76f)));
	glUniform3fv(vertex_color_program->sun_direction_vec3, 1, glm::value_ptr(glm::normalize(glm::vec3(-0.2f, 0.2f, 1.0f))));
	glUniform3fv(vertex_color_program->sky_color_vec3, 1, glm::value_ptr(glm::vec3(0.4f, 0.4f, 0.45f)));
	glUniform3fv(vertex_color_program->sky_direction_vec3, 1, glm::value_ptr(glm::vec3(0.0f, 1.0f, 0.0f)));
	glUseProgram(0);

	//fix aspect ratio of camera
	camera->aspect = drawable_size.x / float(drawable_size.y);

	scene.draw(camera);

	phone_bank_scene->draw(camera); //will this work?

	glUseProgram(0);

	glDisable(GL_DEPTH_TEST);
	{ //score messages:
		std::string message1 = "MERITS: ";
		for (uint32_t i = 0; i < 10; ++i) {
			if (i < merits) message1 += '*';
			else message1 += '.';
		}
		std::string message2 = "DEMERITS: ";
		for (uint32_t i = 0; i < 3; ++i) {
			if (i < demerits) message2 += 'X';
			else message2 += '.';
		}
		float height = 0.06f;
		//float width = text_width(message, height);
		float aspect = drawable_size.x / float(drawable_size.y);
		draw_text(message1, glm::vec2(-aspect, 0.99f-height), height, glm::vec4(0.0f, 0.0f, 0.0f, 0.5f));
		draw_text(message1, glm::vec2(-aspect, 1.0f-height), height, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
		draw_text(message2, glm::vec2(-aspect, 0.99f-2.1f*height), height, glm::vec4(0.0f, 0.0f, 0.0f, 0.5f));
		draw_text(message2, glm::vec2(-aspect, 1.0f-2.1f*height), height, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
	}

	if (Mode::current.get() == this) {
		glDisable(GL_DEPTH_TEST);
		{ //mouse captured message:
			std::string message;
			if (mouse_captured) {
				message = "ESCAPE TO UNGRAB MOUSE * WASD MOVE";
			} else {
				message = "CLICK TO GRAB MOUSE * ESCAPE QUIT";
			}
			float height = 0.06f;
			float width = text_width(message, height);
			draw_text(message, glm::vec2(-0.5f * width,-0.99f), height, glm::vec4(0.0f, 0.0f, 0.0f, 0.5f));
			draw_text(message, glm::vec2(-0.5f * width,-1.0f), height, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
		}
		if (close_phone) { //phone interaction message:
			{
				std::string message;
				message = "CLICK FOR";
				float height = 0.06f;
				float width = text_width(message, height);
				draw_text(message, glm::vec2(-0.5f * width, -0.01f), height, glm::vec4(0.0f, 0.0f, 0.0f, 0.5f));
				draw_text(message, glm::vec2(-0.5f * width, 0.0f), height, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
			}

			{
				std::string message;
				message = close_phone->object->transform->name.substr(6) + " PHONE";
				float height = 0.06f;
				float width = text_width(message, height);
				draw_text(message, glm::vec2(-0.5f * width,-height - 0.01f), height, glm::vec4(0.0f, 0.0f, 0.0f, 0.5f));
				draw_text(message, glm::vec2(-0.5f * width,-height), height, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
			}
		}
	}

	GL_ERRORS();
}

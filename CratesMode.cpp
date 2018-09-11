#include "CratesMode.hpp"

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

Load< MeshBuffer > crates_meshes(LoadTagDefault, [](){
	return new MeshBuffer(data_path("crates.pnc"));
});

Load< GLuint > crates_meshes_for_vertex_color_program(LoadTagDefault, [](){
	return new GLuint(crates_meshes->make_vao_for_program(vertex_color_program->program));
});

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
	});
	return ret;
});

CratesMode::CratesMode() {
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

	//TODO: look up phones
}

CratesMode::~CratesMode() {
}

bool CratesMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {
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
	}
	return false;
}

void CratesMode::update(float elapsed) {
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

	{ //set sound positions:
		glm::mat4 cam_to_world = camera->transform->make_local_to_world();
		Sound::listener.set_position( cam_to_world[3] );
		//camera looks down -z, so right is +x:
		Sound::listener.set_right( glm::normalize(cam_to_world[0]) );
	}
}

void CratesMode::draw(glm::uvec2 const &drawable_size) {
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

	if (Mode::current.get() == this) {
		glDisable(GL_DEPTH_TEST);
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

		glUseProgram(0);
	}

	GL_ERRORS();
}


void CratesMode::show_pause_menu() {
	std::shared_ptr< MenuMode > menu = std::make_shared< MenuMode >();

	std::shared_ptr< Mode > game = shared_from_this();
	menu->background = game;

	menu->choices.emplace_back("PAUSED");
	menu->choices.emplace_back("RESUME", [game](){
		Mode::set_current(game);
	});
	menu->choices.emplace_back("QUIT", [](){
		Mode::set_current(nullptr);
	});

	menu->selected = 1;

	Mode::set_current(menu);
}

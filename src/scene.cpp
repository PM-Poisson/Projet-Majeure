#include "scene.hpp"

using namespace cgp;

void scene_structure::initialize()
{
	// Caméra
	camera_control.initialize(inputs, window);
	camera_control.set_rotation_axis_z();
	camera_control.look_at({ 15.0f, 10.0f, 10.0f }, { 0, 0, 0 }, { 0, 0, 1 });
	global_frame.initialize_data_on_gpu(mesh_primitive_frame());

	// --- Chargement du shader custom océan ---
	ocean_shader.load(
		project::path + "shaders/ocean/ocean.vert.glsl",
		project::path + "shaders/ocean/ocean.frag.glsl"
	);

	// --- Génération de la grille océan (CPU, statique) ---
	const int   N    = 50;
	const float size = 20.0f;

	cgp::mesh ocean_mesh;

	// Sommets + UV
	for(int i = 0; i < N; ++i) {
		for(int j = 0; j < N; ++j) {
			float u = i / float(N - 1);
			float v = j / float(N - 1);

			vec3 p;
			p.x = size * (u - 0.5f);
			p.y = size * (v - 0.5f);
			p.z = 0.0f;

			ocean_mesh.position.push_back(p);
			ocean_mesh.uv.push_back({ u, v });
		}
	}

	// Triangles
	for(int i = 0; i < N - 1; ++i) {
		for(int j = 0; j < N - 1; ++j) {
			int k = i * N + j;
			ocean_mesh.connectivity.push_back({ uint(k),     uint(k + 1),     uint(k + N)     });
			ocean_mesh.connectivity.push_back({ uint(k + 1), uint(k + N + 1), uint(k + N)     });
		}
	}

	// Normales + couleurs par défaut
	ocean_mesh.fill_empty_field();

	// Envoi au GPU avec le shader custom
	ocean_drawable.initialize_data_on_gpu(ocean_mesh, ocean_shader);

	// Couleur bleue de base (modulée dans le shader)
	ocean_drawable.material.color = { 0.188f, 0.325f, 0.796f };
	ocean_drawable.material.alpha = 0.9f;
	ocean_drawable.material.phong.ambient   = 0.3f;
	ocean_drawable.material.phong.diffuse   = 0.6f;
	ocean_drawable.material.phong.specular  = 0.3f;
}

void scene_structure::display_frame()
{
	environment.light = camera_control.camera_model.position();

	if(gui.display_frame)
		draw(global_frame, environment);

	// Activer le shader et envoyer le temps avant le draw
	float t = float(glfwGetTime());
	glUseProgram(ocean_shader.id);
	opengl_uniform(ocean_shader, "time", t, true);

	// Dessin de l'océan
	if(gui.display_wireframe)
		draw_wireframe(ocean_drawable, environment);
	else
		draw(ocean_drawable, environment);
}

void scene_structure::display_gui()
{
	ImGui::Checkbox("Frame",     &gui.display_frame);
	ImGui::Checkbox("Wireframe", &gui.display_wireframe);
}

void scene_structure::mouse_move_event()
{
	if(!inputs.keyboard.shift)
		camera_control.action_mouse_move(environment.camera_view);
}
void scene_structure::mouse_click_event()
{
	camera_control.action_mouse_click(environment.camera_view);
}
void scene_structure::keyboard_event()
{
	camera_control.action_keyboard(environment.camera_view);
}
void scene_structure::idle_frame()
{
	camera_control.idle_frame(environment.camera_view);
}
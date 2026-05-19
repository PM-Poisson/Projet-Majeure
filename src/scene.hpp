#pragma once

#include "cgp/cgp.hpp"
#include "environment.hpp"

using cgp::mesh_drawable;

struct gui_parameters {
	bool display_frame     = true;
	bool display_wireframe = false;
};

struct scene_structure : cgp::scene_inputs_generic {

	// Caméra, projection, fenêtre — boilerplate CGP
	camera_controller_orbit_euler camera_control;
	camera_projection_perspective camera_projection;
	window_structure window;

	mesh_drawable      global_frame;
	environment_structure environment;
	input_devices      inputs;
	gui_parameters     gui;

	// --- Océan ---
	cgp::mesh_drawable ocean_drawable; // mesh + shader + GPU
	cgp::opengl_shader_structure ocean_shader; // shader custom

	// --- Fonctions ---
	void initialize();
	void display_frame();
	void display_gui();

	void mouse_move_event();
	void mouse_click_event();
	void keyboard_event();
	void idle_frame();
};
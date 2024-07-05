#pragma once

#include <godot_cpp/godot.hpp>
#include <godot_cpp/classes/camera2d.hpp>
#include <godot_cpp/classes/canvas_layer.hpp>
#include <godot_cpp/classes/node2d.hpp>
#include <godot_cpp/classes/sprite2d.hpp>
#include <godot_cpp/classes/sub_viewport.hpp>
#include <godot_cpp/classes/texture_rect.hpp>
#include <godot_cpp/classes/viewport_texture.hpp>

#include <string>
#include <unordered_map>

namespace godot {

/// @brief This node is made to catch the texture being rendered
/// by the entity drawer and debug it if necessary
/// The texture is used to pick entities
class TextureCatcher : public Node2D {
	GDCLASS(TextureCatcher, Node2D)

public:
	// godot routines
	void _ready() override;
	// void _draw() override;
	// void _physics_process(double delta_p) override;
	void _process(double delta_p) override;

	void set_debug(bool debug_p) { _debug_canvas->set_visible(debug_p); }
	bool is_debug() const { return _debug_canvas->is_visible(); }

	Node2D * get_alt_viewport() { return _alt_viewport; }
	Ref<ViewportTexture> get_texture() const { return _sub_viewport->get_texture(); }

	// signal
	void _on_size_changed();

	// Will be called by Godot when the class is registered
	// Use this to add properties to your class
	static void _bind_methods();

	// getter/setter
	double get_scale_viewport() const { return _scale_viewport; }
	void set_scale_viewport(double const &scale_viewport) { _scale_viewport = scale_viewport; }
	NodePath const & get_ref_camera() const { return _ref_camera_path; }
	void set_ref_camera(NodePath const &ref_camera) { _ref_camera_path = ref_camera; }

private:
	double _scale_viewport = 2.;
	Camera2D * _ref_camera = nullptr;
	NodePath _ref_camera_path;
	SubViewport * _sub_viewport = nullptr;
	Camera2D * _camera = nullptr;
	CanvasLayer *_debug_canvas = nullptr;
	TextureRect *_texture = nullptr;
	Sprite2D *_alt_viewport = nullptr;
};

}

#pragma once

#ifdef GD_EXTENSION_GODOCTOPUS
	#include <godot_cpp/godot.hpp>
	#include <godot_cpp/classes/node2d.hpp>
	#include <godot_cpp/classes/atlas_texture.hpp>
	#include <godot_cpp/classes/shader_material.hpp>
	#include <godot_cpp/classes/sprite_frames.hpp>
#else
	#include "scene/2d/node_2d.h"
	#include "scene/resources/atlas_texture.h"
	#include "scene/resources/material.h"
	#include "scene/resources/sprite_frames.h"
#endif

#include "smart_list/smart_list.h"
#include <mutex>

namespace godot {

struct StringInstance
{
	StringName str = "";
	Vector2 position;
	double spawn_time = 0.;
	Ref<Texture2D> icon;
	Color color;
	bool floating = false;
	bool outline = false;
};

/// @brief This a class that aims at displaying lots of string using backend display
class StringDrawer : public Node2D {
	GDCLASS(StringDrawer, Node2D)

public:
	virtual ~StringDrawer();

	// godot routines
	void _ready();
	void _draw();
	void _process(double delta_p);

	/// @brief Add a string instance
	int add_string_instance(StringName const &str, bool outline, bool floating, Vector2 const &pos, Color const &color, Ref<Texture2D> const &texture);

	// Will be called by Godot when the class is registered
	// Use this to add properties to your class
	static void _bind_methods();

	// getters/setters for basic prop
	double get_oscillation_factor() const { return oscillation_factor; }
	void set_oscillation_factor(double d) { oscillation_factor = d; }
	double get_up_speed() const { return up_speed; }
	void set_up_speed(double d) { up_speed = d; }
	double get_float_time() const { return float_time; }
	void set_float_time(double d) { float_time = d; }
	double get_fade_time() const { return fade_time; }
	void set_fade_time(double d) { fade_time = d; }
	double get_icon_offset_x() const { return icon_offset_x; }
	void set_icon_offset_x(double d) { icon_offset_x = d; }
	double get_icon_offset_y() const { return icon_offset_y; }
	void set_icon_offset_y(double d) { icon_offset_y = d; }
	double get_icone_size() const { return icone_size; }
	void set_icone_size(double d) { icone_size = d; }

protected:
	// godot routine when used internally
	void _notification(int p_notification);

private:
	/// @brief mutex used to avoid data race between text manipulation and draw
	std::mutex mutex;

	/// @brief time since start
	double elapsed_time = 0.;

	/// @brief we use a smart list to iterate fast an reuse memory
	smart_list<StringInstance> instances;

	// floating parameters
	double oscillation_factor = 0.25;
	double up_speed = 2.25;
	double float_time = 0.5;
	double fade_time = 0.25;
	double icon_offset_x = 10.;
	double icon_offset_y = -20.;
	double icone_size = 28.;
};

} // godot
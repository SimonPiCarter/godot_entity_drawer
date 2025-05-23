
#include "StringDrawer.h"


#ifdef GD_EXTENSION_GODOCTOPUS
	#include <godot_cpp/variant/utility_functions.hpp>
	#include <godot_cpp/classes/rendering_server.hpp>
#else
	#include "servers/rendering_server.h"
	#include <scene/theme/theme_db.h>
	#include <scene/main/window.h>
#endif

#include <cmath>

namespace godot
{

	void StringDrawer::_notification(int p_notification)
	{
		switch (p_notification) {
			case NOTIFICATION_PROCESS: {
				_process(get_process_delta_time());
			} break;
			case NOTIFICATION_DRAW: {
				_draw();
			} break;
			case NOTIFICATION_READY: {
				_ready();
				set_process(true);
				set_physics_process(true);
			} break;
		}
	}

	StringDrawer::~StringDrawer() {}

	void StringDrawer::_ready() {}

	void StringDrawer::_draw()
	{
		std::lock_guard<std::mutex> lock(mutex);
		std::vector<size_t> ended_instances;
		instances.for_each_const([this, &ended_instances](StringInstance const &instance, size_t idx) {
			Vector2 pos = instance.position;
			Color color = instance.color;
			if(instance.floating)
			{
				double delta = elapsed_time - instance.spawn_time;
				pos += Vector2(std::cos(delta * 3.14)*oscillation_factor, -delta*up_speed);

				color.a = 1. - std::max(0., (delta - float_time)/fade_time);

				if(color.a < 1e-5)
				{
					ended_instances.push_back(idx);
				}
			}
			if(instance.outline)
			{
				draw_string_outline(
					get_window()->get_theme_default_font() ,
					8*pos,
					instance.str,
					HORIZONTAL_ALIGNMENT_LEFT, -1, Font::DEFAULT_FONT_SIZE, 4,
					Color(0,0,0,color.a)
				);
			}
			draw_string(
				get_window()->get_theme_default_font(),
				8*pos,
				instance.str,
				HORIZONTAL_ALIGNMENT_LEFT, -1, Font::DEFAULT_FONT_SIZE,
				color
			);
			if(instance.icon.is_valid())
			{
				draw_texture_rect(
					instance.icon,
					Rect2(8*pos + Vector2(instance.str.length()*icon_offset_x, icon_offset_y), Vector2(icone_size,icone_size)),
					false,
					Color(1,1,1,color.a)
				);
			}
		});

		for(size_t idx : ended_instances)
		{
			instances.free_instance(idx);
		}
	}

	void StringDrawer::_process(double delta)
	{
		elapsed_time += delta;
	}

	int StringDrawer::add_string_instance(StringName const &str, bool outline, bool floating, Vector2 const &pos,
		Color const &color, Ref<Texture2D> const &texture)
	{
		std::lock_guard<std::mutex> lock(mutex);
		smart_list_handle<StringInstance> handle = instances.new_instance({str, pos, elapsed_time, texture, color, floating, outline});
		return int(handle.handle());
	}

	void StringDrawer::_bind_methods()
	{
		ClassDB::bind_method(D_METHOD("add_string_instance", "str", "outline", "floating", "pos", "color", "icon"), &StringDrawer::add_string_instance);

		ADD_GROUP("StringDrawer", "StringDrawer_");

		// oscillation_factor
		ClassDB::bind_method(D_METHOD("get_oscillation_factor"), &StringDrawer::get_oscillation_factor);
		ClassDB::bind_method(D_METHOD("set_oscillation_factor", "oscillation_factor"), &StringDrawer::set_oscillation_factor);
		ClassDB::add_property("StringDrawer", PropertyInfo(Variant::FLOAT, "oscillation_factor"), "set_oscillation_factor", "get_oscillation_factor");

		// up_speed
		ClassDB::bind_method(D_METHOD("get_up_speed"), &StringDrawer::get_up_speed);
		ClassDB::bind_method(D_METHOD("set_up_speed", "up_speed"), &StringDrawer::set_up_speed);
		ClassDB::add_property("StringDrawer", PropertyInfo(Variant::FLOAT, "up_speed"), "set_up_speed", "get_up_speed");

		// float_time
		ClassDB::bind_method(D_METHOD("get_float_time"), &StringDrawer::get_float_time);
		ClassDB::bind_method(D_METHOD("set_float_time", "float_time"), &StringDrawer::set_float_time);
		ClassDB::add_property("StringDrawer", PropertyInfo(Variant::FLOAT, "float_time"), "set_float_time", "get_float_time");

		// fade_time
		ClassDB::bind_method(D_METHOD("get_fade_time"), &StringDrawer::get_fade_time);
		ClassDB::bind_method(D_METHOD("set_fade_time", "fade_time"), &StringDrawer::set_fade_time);
		ClassDB::add_property("StringDrawer", PropertyInfo(Variant::FLOAT, "fade_time"), "set_fade_time", "get_fade_time");

		// icon_offset_x
		ClassDB::bind_method(D_METHOD("get_icon_offset_x"), &StringDrawer::get_icon_offset_x);
		ClassDB::bind_method(D_METHOD("set_icon_offset_x", "icon_offset_x"), &StringDrawer::set_icon_offset_x);
		ClassDB::add_property("StringDrawer", PropertyInfo(Variant::FLOAT, "icon_offset_x"), "set_icon_offset_x", "get_icon_offset_x");

		// icon_offset_y
		ClassDB::bind_method(D_METHOD("get_icon_offset_y"), &StringDrawer::get_icon_offset_y);
		ClassDB::bind_method(D_METHOD("set_icon_offset_y", "icon_offset_y"), &StringDrawer::set_icon_offset_y);
		ClassDB::add_property("StringDrawer", PropertyInfo(Variant::FLOAT, "icon_offset_y"), "set_icon_offset_y", "get_icon_offset_y");

		// icone_size
		ClassDB::bind_method(D_METHOD("get_icone_size"), &StringDrawer::get_icone_size);
		ClassDB::bind_method(D_METHOD("set_icone_size", "icone_size"), &StringDrawer::set_icone_size);
		ClassDB::add_property("StringDrawer", PropertyInfo(Variant::FLOAT, "icone_size"), "set_icone_size", "get_icone_size");

	}

} // godot

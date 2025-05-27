
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

	void StringDrawer::_ready()
	{
		if(!_ref_camera_path.is_empty())
			_ref_camera = (Camera2D*)get_node(_ref_camera_path);
	}

	Size2i get_size(Viewport const *viewport)
	{
		Window const * wi = dynamic_cast<Window const *>(viewport);
		SubViewport const * sv = dynamic_cast<SubViewport const *>(viewport);
		if(wi)
		{
			return wi->get_size();
		}
		if(sv)
		{
			return sv->get_size();
		}
		return Size2i();
	}

	Vector2 get_cam_top_left(Camera2D * camera)
	{
		Size2i viewport_size = get_size(camera->get_viewport());
		auto cam_size = Vector2(viewport_size.x, viewport_size.y) / camera->get_zoom().x;
		return camera->get_position() - cam_size / 2.;
	}

	Vector2 get_screen_pos(Camera2D * camera, Vector2 const &world_pos)
	{
		auto viewport_pos = (world_pos - get_cam_top_left(camera)) * camera->get_zoom().x;
		auto window_size = DisplayServer::get_singleton()->window_get_size();
		Size2i viewport_size = get_size(camera->get_viewport());
		auto scale_size = Vector2(float(viewport_size.x) / window_size.x, float(viewport_size.y) / window_size.y);

		auto screen_pos = viewport_pos / scale_size;
		return screen_pos;
	}

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
			// adjust from camera if available
			if(_ref_camera)
			{
				pos = get_screen_pos(_ref_camera, pos);
			}
			if(instance.outline)
			{
				draw_string_outline(
					get_window()->get_theme_default_font() ,
					pos,
					instance.str,
					HORIZONTAL_ALIGNMENT_LEFT, -1, Font::DEFAULT_FONT_SIZE * size_ratio, 4 * size_ratio,
					Color(0,0,0,color.a)
				);
			}
			draw_string(
				get_window()->get_theme_default_font(),
				pos,
				instance.str,
				HORIZONTAL_ALIGNMENT_LEFT, -1, Font::DEFAULT_FONT_SIZE * size_ratio,
				color
			);
			if(instance.icon.is_valid())
			{
				draw_texture_rect(
					instance.icon,
					Rect2(pos + Vector2(instance.str.length()*icon_offset_x, icon_offset_y), size_ratio * Vector2(icon_size,icon_size)),
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
		queue_redraw();
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

		ClassDB::bind_method(D_METHOD("get_ref_camera"), &StringDrawer::get_ref_camera);
		ClassDB::bind_method(D_METHOD("set_ref_camera", "ref_camera"), &StringDrawer::set_ref_camera);
		ClassDB::add_property("StringDrawer", PropertyInfo(Variant::NODE_PATH, "ref_camera", PROPERTY_HINT_NODE_PATH_VALID_TYPES, "Camera2D"), "set_ref_camera", "get_ref_camera");

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

		// icon_size
		ClassDB::bind_method(D_METHOD("get_icon_size"), &StringDrawer::get_icon_size);
		ClassDB::bind_method(D_METHOD("set_icon_size", "icon_size"), &StringDrawer::set_icon_size);
		ClassDB::add_property("StringDrawer", PropertyInfo(Variant::FLOAT, "icon_size"), "set_icon_size", "get_icon_size");

		// size_ratio
		ClassDB::bind_method(D_METHOD("get_size_ratio"), &StringDrawer::get_size_ratio);
		ClassDB::bind_method(D_METHOD("set_size_ratio", "size_ratio"), &StringDrawer::set_size_ratio);
		ClassDB::add_property("StringDrawer", PropertyInfo(Variant::FLOAT, "size_ratio"), "set_size_ratio", "get_size_ratio");

	}

} // godot

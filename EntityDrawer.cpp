
#include "EntityDrawer.h"
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/rendering_server.hpp>

#include <algorithm>

namespace godot
{
	Vector3 color_from_idx(int idx_p)
	{
		// Compute the color based on the idx
		int r = idx_p % 256;
		int g = (idx_p/ 256 ) % 256;
		int b = (idx_p/ (256*256) ) % 256;
		return Vector3(r/255.,g/255.,b/255.);
	}

	int idx_from_color(Color const &color_p)
	{
		int r = int(color_p.r * 255);
		int g = int(color_p.g * 255);
		int b = int(color_p.b * 255);
		if(r != 255 || g != 255 || b != 255)
		{
			return r + g *256 + b *256*256;
		}
		return -1;
	}

	Color safe_color(int x, int y, Ref<Image> const &image_p)
	{
		if(x >= 0 && x < image_p->get_width()
		&& y >= 0 && y < image_p->get_height())
		{
			return image_p->get_pixel(x, y);
		}
		return Color(1.f,1.f,1.f,1.f);
	}

	EntityDrawer::~EntityDrawer()
	{
		for(EntityInstance &instance_l : _instances)
		{
			RenderingServer::get_singleton()->free_rid(instance_l._canvas);
		}
	}

	void EntityDrawer::_ready()
	{
		// Custom shader to display a color per idx
		_alt_shader = Ref<Shader>(memnew(Shader));
		_alt_shader->set_code("\n\
			shader_type canvas_item;\n\
			\n\
			uniform vec3 idx_color : source_color = vec3(1.0);\n\
			\n\
			void fragment() {\n\
				COLOR.rgb = idx_color;\n\
				COLOR.a = round(COLOR.a);\n\
			}\n\
			"
		);
	}

	int EntityDrawer::add_instance(Vector2 const &pos_p, Vector2 const &offset_p, Ref<SpriteFrames> const & animation_p,
		StringName const &current_animation_p, StringName const &next_animation_p, bool one_shot_p)
	{
		if(_freeIdx.empty())
		{
			_instances.push_back({offset_p, animation_p, true, _elapsedTime, 0, current_animation_p, next_animation_p, one_shot_p});
			_newPos.push_back(pos_p);
			_oldPos.push_back(pos_p);

			// set up resources
			EntityInstance &instance_l = _instances.back();
			instance_l._canvas = RenderingServer::get_singleton()->canvas_item_create();
			instance_l._material = Ref<ShaderMaterial>(memnew(ShaderMaterial));
			instance_l._material->set_shader(_shader);
			//instance_l._material->set_shader_parameter("uni_enabled", true);

			RenderingServer::get_singleton()->canvas_item_set_parent(instance_l._canvas, get_canvas_item());
			RenderingServer::get_singleton()->canvas_item_set_default_texture_filter(instance_l._canvas, RenderingServer::CANVAS_ITEM_TEXTURE_FILTER_NEAREST);
			RenderingServer::get_singleton()->canvas_item_set_material(instance_l._canvas, instance_l._material->get_rid());

			if(_alt_viewport)
			{
				instance_l._alt_canvas = RenderingServer::get_singleton()->canvas_item_create();

				instance_l._alt_material = Ref<ShaderMaterial>(memnew(ShaderMaterial));
				instance_l._alt_material->set_shader(_alt_shader);
				instance_l._alt_material->set_shader_parameter("idx_color", color_from_idx(_instances.size()-1));

				RenderingServer::get_singleton()->canvas_item_set_parent(instance_l._alt_canvas, _alt_viewport->get_canvas_item());
				RenderingServer::get_singleton()->canvas_item_set_default_texture_filter(instance_l._alt_canvas, RenderingServer::CANVAS_ITEM_TEXTURE_FILTER_NEAREST);
				RenderingServer::get_singleton()->canvas_item_set_material(instance_l._alt_canvas, instance_l._alt_material->get_rid());
			}

			return int(_instances.size()-1);
		}
		else
		{
			size_t idx_l = _freeIdx.front();
			_freeIdx.pop_front();
			_instances[idx_l].offset = offset_p;
			_instances[idx_l].animation = animation_p;
			_instances[idx_l].enabled = true;
			_instances[idx_l].start = _elapsedTime;
			_instances[idx_l].frame_idx = 0;
			_instances[idx_l].current_animation = current_animation_p;
			_instances[idx_l].next_animation = next_animation_p;
			_instances[idx_l].one_shot = one_shot_p;
			_instances[idx_l].handler = -1;
			_newPos[idx_l] = pos_p;
			_oldPos[idx_l] = pos_p;

			return int(idx_l);
		}

	}

	void EntityDrawer::update_pos()
	{
		_elapsedTime = 0.;
		// swap positions
		std::swap(_oldPos, _newPos);
	}

	std::vector<Vector2> & EntityDrawer::getNewPos()
	{
		return _newPos;
	}

	void EntityDrawer::_physics_process(double delta_p)
	{
		size_t i = 0;
		for(DirectionHandler &handler_l : _directionHandlers)
		{
			if(!handler_l.enabled)
			{
				++i;
				continue;
			}
			Vector2 dir_l = handler_l.direction;
			int new_type = DirectionHandler::NONE;
			if(dir_l.length_squared() < 0.1)
			{
				dir_l = _newPos[handler_l.instance] - _oldPos[handler_l.instance];
			}
			if(std::abs(dir_l.x) > 0.01 || std::abs(dir_l.y) > 0.01)
			{
				if(std::abs(dir_l.x) > std::abs(dir_l.y) || !handler_l.has_up_down)
				{
					if(dir_l.x > 0)
					{
						new_type = DirectionHandler::RIGHT;
					}
					else
					{
						new_type = DirectionHandler::LEFT;
					}
				}
				else
				{
					if(dir_l.y > 0)
					{
						new_type = DirectionHandler::DOWN;
					}
					else
					{
						new_type = DirectionHandler::UP;
					}
				}
			}
			if(new_type != DirectionHandler::NONE)
			{
				if(new_type != handler_l.count_type)
				{
					handler_l.count = 0;
					handler_l.count_type = new_type;
				}
				if(handler_l.count < 100)
				{
					++handler_l.count;
				}
			}
			else
			{
				handler_l.count = 0;
				handler_l.count_type = DirectionHandler::NONE;
			}

			if((handler_l.count > 15
			&& handler_l.count_type != DirectionHandler::NONE
			&& handler_l.count_type != handler_l.type))
			{
				handler_l.type = handler_l.count_type;
			}
			++i;
		}
	}

	StringName const & get_anim(EntityInstance const &instance_p, DirectionHandler const &handler_p)
	{
		if(handler_p.type < 0)
		{
			return handler_p.names[2];
		}
		return handler_p.names[handler_p.type];
	}

	StringName const & get_anim(EntityInstance const &instance_p, std::vector<DirectionHandler> const &directionHandlers_p)
	{
		if(instance_p.handler < 0)
		{
			return instance_p.current_animation;
		}
		return get_anim(instance_p, directionHandlers_p[instance_p.handler]);
	}

	void init_handler(EntityInstance const &instance_p, std::vector<DirectionHandler> &directionHandlers_p)
	{
		if(instance_p.handler < 0)
		{
			return;
		}
		DirectionHandler &handler_l = directionHandlers_p[instance_p.handler];
		if(handler_l.base_name == instance_p.current_animation)
		{
			return;
		}
		handler_l.base_name = instance_p.current_animation;
		handler_l.names[DirectionHandler::UP] = StringName("up_"+instance_p.current_animation);
		handler_l.names[DirectionHandler::DOWN] = StringName("down_"+instance_p.current_animation);
		handler_l.names[DirectionHandler::LEFT] = StringName("left_"+instance_p.current_animation);
		handler_l.names[DirectionHandler::RIGHT] = StringName("right_"+instance_p.current_animation);
		handler_l.direction = Vector2(0,0);
	}

	StringName const & EntityDrawer::get_animation(int idx_p) const
	{
		EntityInstance const &instance_l = _instances[idx_p];
		return instance_l.current_animation;
	}

	void EntityDrawer::set_animation(int idx_p, StringName const &current_animation_p, StringName const &next_animation_p)
	{
		EntityInstance &instance_l = _instances[idx_p];

		instance_l.current_animation = current_animation_p;
		instance_l.next_animation = next_animation_p;
		instance_l.frame_idx = 0;
		instance_l.start = _elapsedAllTime;
		instance_l.one_shot = false;

		init_handler(instance_l, _directionHandlers);
	}

	void EntityDrawer::set_animation_one_shot(int idx_p, StringName const &current_animation_p)
	{
		EntityInstance &instance_l = _instances[idx_p];

		instance_l.current_animation = current_animation_p;
		instance_l.frame_idx = 0;
		instance_l.start = _elapsedAllTime;
		instance_l.one_shot = true;

		init_handler(instance_l, _directionHandlers);
	}

	void EntityDrawer::set_direction(int idx_p, Vector2 const &direction_p)
	{
		EntityInstance &instance_l = _instances[idx_p];
		if(instance_l.handler < 0)
		{
			return;
		}
		_directionHandlers[instance_l.handler].direction = direction_p;
	}

	void EntityDrawer::add_direction_handler(int idx_p, bool has_up_down_p)
	{
		EntityInstance &instance_l = _instances[idx_p];
		int idxHandler_l = -1;
		if(_freeHandlersIdx.empty())
		{
			idxHandler_l = int(_directionHandlers.size());
			_directionHandlers.push_back(DirectionHandler());
			_directionHandlers.back().instance = idx_p;
			_directionHandlers.back().has_up_down = has_up_down_p;

		}
		else
		{
			size_t idx_l = _freeHandlersIdx.front();
			_freeHandlersIdx.pop_front();
			_directionHandlers[idx_l].direction = Vector2();
			_directionHandlers[idx_l].type = -1;
			_directionHandlers[idx_l].has_up_down = has_up_down_p;
			_directionHandlers[idx_l].enabled = true;
			_directionHandlers[idx_l].count = 0;
			_directionHandlers[idx_l].instance = idx_p;
			_directionHandlers[idx_l].count_type = -1;
			idxHandler_l = int(idx_l);
		}
		instance_l.handler = idxHandler_l;
		init_handler(instance_l, _directionHandlers);
	}

	void EntityDrawer::remove_direction_handler(int idx_p)
	{
		EntityInstance &instance_l = _instances[idx_p];
		if(instance_l.handler >= 0)
		{
			_directionHandlers[instance_l.handler].enabled = false;
			_freeHandlersIdx.push_back(instance_l.handler);
			instance_l.handler = -1;
		}
	}

	void EntityDrawer::set_new_pos(int idx_p, Vector2 const &pos_p)
	{
		_newPos[idx_p] = pos_p;
	}

	Vector2 const & EntityDrawer::get_old_pos(int idx_p)
	{
		return _oldPos[idx_p];
	}

	Ref<ShaderMaterial> EntityDrawer::get_shader_material(int idx_p)
	{
		return _instances[idx_p]._material;
	}

	TypedArray<int> EntityDrawer::indexes_from_texture(Rect2i const &rect_p, Ref<Texture2D> const &texture_p) const
	{
		Ref<Image> image_l = texture_p->get_image();
		TypedArray<bool> all_added_l = index_array_from_texture(rect_p, texture_p);

		TypedArray<int> indexes_l;
		int idx_l = 0;
		for(int idx_l = 0 ; idx_l < all_added_l.size() ; ++ idx_l)
		{
			if (all_added_l[idx_l])
			{
				indexes_l.append(idx_l);
			}
		}

		return indexes_l;
	}

	TypedArray<bool> EntityDrawer::index_array_from_texture(Rect2i const &rect_p, Ref<Texture2D> const &texture_p) const
	{
		Ref<Image> image_l = texture_p->get_image();
		TypedArray<bool> all_added_l;
		all_added_l.resize(_instances.size());
		all_added_l.fill(false);
		for(int32_t x = rect_p.get_position().x ; x <= rect_p.get_position().x + rect_p.get_size().x ; ++ x)
		{
			for(int32_t y = rect_p.get_position().y ; y <= rect_p.get_position().y + rect_p.get_size().y ; ++ y)
			{
				int idx_l = idx_from_color(safe_color(x, y, image_l));
				if(idx_l >= 0)
				{
					all_added_l[idx_l] = true;
				}
			}
		}
		return all_added_l;
	}

	int EntityDrawer::index_from_texture(Vector2i const &pos_p, Ref<Texture2D> const &texture_p) const
	{
		return idx_from_color(safe_color(pos_p.x, pos_p.y, texture_p->get_image()));
	}

	void EntityDrawer::set_shader_bool_params(String const &param_p, TypedArray<bool> const &values_p)
	{
		for(size_t i = 0 ; i < values_p.size() && i < _instances.size() ; ++ i)
		{
			if(_instances[i]._material.is_valid())
			{
				_instances[i]._material->set_shader_parameter(param_p, values_p[i]);
			}
		}
	}

	void EntityDrawer::set_shader_bool_params_from_indexes(String const &param_p, TypedArray<int> const &indexes_p, bool value_indexes_p, bool value_others_p)
	{
		// set all default values
		for(size_t i = 0 ; i < _instances.size() ; ++ i)
		{
			if(_instances[i]._material.is_valid())
			{
				_instances[i]._material->set_shader_parameter(param_p, value_others_p);
			}
		}
		// set for indexes
		for(size_t i = 0 ; i < indexes_p.size() ; ++ i)
		{
			int idx_l = indexes_p[i];
			if(idx_l < _instances.size() && _instances[idx_l]._material.is_valid())
			{
				_instances[idx_l]._material->set_shader_parameter(param_p, value_indexes_p);
			}
		}
	}

	void EntityDrawer::_process(double delta_p)
	{
		_elapsedTime += delta_p;
		_elapsedAllTime += delta_p;
		queue_redraw();
	}

	void EntityDrawer::_draw()
	{
		size_t i = 0;
		for(EntityInstance &instance_l : _instances)
		{
			StringName const &cur_anim_l = get_anim(instance_l, _directionHandlers);
			if(instance_l.enabled && instance_l.animation.is_valid() && instance_l.animation->get_frame_count(cur_anim_l) > 0)
			{
				double frameTime_l = instance_l.animation->get_frame_duration(cur_anim_l, instance_l.frame_idx) / instance_l.animation->get_animation_speed(cur_anim_l) ;
				double nextFrameTime_l = instance_l.start + frameTime_l;
				if(_elapsedAllTime >= nextFrameTime_l)
				{
					++instance_l.frame_idx;
					instance_l.start = _elapsedAllTime;
				}
				if(instance_l.frame_idx >= instance_l.animation->get_frame_count(cur_anim_l))
				{
					if(instance_l.one_shot)
					{
						// free animation
						RenderingServer::get_singleton()->canvas_item_clear(instance_l._canvas);
						if(instance_l._alt_canvas.is_valid())
							RenderingServer::get_singleton()->canvas_item_clear(instance_l._alt_canvas);
						instance_l.enabled = false;
						_freeIdx.push_back(i);
						if(instance_l.handler >= 0)
						{
							DirectionHandler & handler_l = _directionHandlers[instance_l.handler];
							handler_l.enabled = false;
							_freeHandlersIdx.push_back(i);
						}
					}
					else if(!instance_l.next_animation.is_empty())
					{
						instance_l.current_animation = instance_l.next_animation;
						instance_l.next_animation = StringName("");
						init_handler(instance_l, _directionHandlers);
					}
					instance_l.frame_idx = 0;
				}
				// if still enabled
				if(instance_l.enabled)
				{
					Vector2 diff_l = _newPos[i] - _oldPos[i];
					Vector2 pos_l =  _oldPos[i] + diff_l * std::min<real_t>(1., real_t(_elapsedTime/_timeStep));
					// draw animaton
					Ref<Texture2D> texture_l = instance_l.animation->get_frame_texture(cur_anim_l, instance_l.frame_idx);
					RenderingServer::get_singleton()->canvas_item_set_transform(instance_l._canvas, Transform2D(0., pos_l));
					RenderingServer::get_singleton()->canvas_item_clear(instance_l._canvas);

					// required when empty texture in sprite frame
					if(texture_l.is_valid())
					{
						texture_l->draw(instance_l._canvas, instance_l.offset);
						// alternate rendering
						if(instance_l._alt_canvas.is_valid())
						{
							RenderingServer::get_singleton()->canvas_item_set_transform(instance_l._alt_canvas, Transform2D(0., pos_l));
							RenderingServer::get_singleton()->canvas_item_clear(instance_l._alt_canvas);
							texture_l->draw(instance_l._alt_canvas, instance_l.offset);
						}
					}
				}
			}
			++i;
		}
	}

	void EntityDrawer::_bind_methods()
	{
		ClassDB::bind_method(D_METHOD("add_instance", "position", "offset", "animation", "current_animation", "next_animation", "one_shot"), &EntityDrawer::add_instance);
		ClassDB::bind_method(D_METHOD("update_pos"), &EntityDrawer::update_pos);

		ClassDB::bind_method(D_METHOD("set_animation", "instance", "current_animation", "next_animation"), &EntityDrawer::set_animation);
		ClassDB::bind_method(D_METHOD("set_animation_one_shot", "instance", "current_animation"), &EntityDrawer::set_animation_one_shot);
		ClassDB::bind_method(D_METHOD("set_direction", "instance", "direction"), &EntityDrawer::set_direction);
		ClassDB::bind_method(D_METHOD("add_direction_handler", "instance", "has_up_down"), &EntityDrawer::add_direction_handler);
		ClassDB::bind_method(D_METHOD("remove_direction_handler", "instance"), &EntityDrawer::remove_direction_handler);
		ClassDB::bind_method(D_METHOD("set_new_pos", "instance", "pos"), &EntityDrawer::set_new_pos);
		ClassDB::bind_method(D_METHOD("get_old_pos", "instance"), &EntityDrawer::get_old_pos);
		ClassDB::bind_method(D_METHOD("get_shader_material", "instance"), &EntityDrawer::get_shader_material);
		ClassDB::bind_method(D_METHOD("set_shader_bool_params", "param", "values"), &EntityDrawer::set_shader_bool_params);
		ClassDB::bind_method(D_METHOD("set_shader_bool_params_from_indexes", "param", "indexes", "value_index", "value_other"), &EntityDrawer::set_shader_bool_params_from_indexes);
		ClassDB::bind_method(D_METHOD("set_shader", "material"), &EntityDrawer::set_shader);
		ClassDB::bind_method(D_METHOD("set_alt_viewport", "alt_viewport"), &EntityDrawer::set_alt_viewport);

		ClassDB::bind_method(D_METHOD("set_time_step", "time_step"), &EntityDrawer::set_time_step);

		ClassDB::bind_method(D_METHOD("indexes_from_texture", "rect", "texture"), &EntityDrawer::indexes_from_texture);
		ClassDB::bind_method(D_METHOD("index_array_from_texture", "rect", "texture"), &EntityDrawer::index_array_from_texture);
		ClassDB::bind_method(D_METHOD("index_from_texture", "pos", "texture"), &EntityDrawer::index_from_texture);

		ADD_GROUP("EntityDrawer", "EntityDrawer_");
	}

} // namespace godot


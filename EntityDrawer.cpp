
#include "EntityDrawer.h"
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/rendering_server.hpp>

#include <algorithm>
#include "TextureCatcher.h"

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

	void init_animation(DirectionalAnimation &anim_p, StringName const &base_anim_p)
	{
		anim_p.base_name = base_anim_p;
		anim_p.names[DirectionHandler::UP] = "up_"+base_anim_p;
		anim_p.names[DirectionHandler::DOWN] = "down_"+base_anim_p;
		anim_p.names[DirectionHandler::LEFT] = "left_"+base_anim_p;
		anim_p.names[DirectionHandler::RIGHT] = "right_"+base_anim_p;
	}

	int get_direction(Vector2 const &dir_p, bool has_up_down_p)
	{
		int type_l = DirectionHandler::NONE;
		if(std::abs(dir_p.x) > 0.01 || std::abs(dir_p.y) > 0.01)
		{
			if(std::abs(dir_p.x) > std::abs(dir_p.y) || !has_up_down_p)
			{
				if(dir_p.x > 0)
				{
					type_l = DirectionHandler::RIGHT;
				}
				else
				{
					type_l = DirectionHandler::LEFT;
				}
			}
			else
			{
				if(dir_p.y > 0)
				{
					type_l = DirectionHandler::DOWN;
				}
				else
				{
					type_l = DirectionHandler::UP;
				}
			}
		}
		return type_l;
	}

	EntityDrawer::~EntityDrawer()
	{
		_instances.for_each([&](EntityInstance &, size_t idx_p) {
			free_instance(idx_p);
		});
		delete _payload_handler;
	}

	// helper for animation
	void set_up_animation(smart_list_handle<AnimationInstance> &handle_p,
		double elapsed_time_p, Ref<Shader> const &shader_p, RID const &parent_p,
		Vector2 const &offset_p, Ref<SpriteFrames> const & animation_p,
		StringName const &current_animation_p, StringName const &next_animation_p, bool one_shot_p)
	{
		AnimationInstance &animation_l = handle_p.get();
		animation_l.offset = offset_p;
		animation_l.animation = animation_p;
		animation_l.start = elapsed_time_p;
		animation_l.frame_idx = 0;
		animation_l.current_animation = current_animation_p;
		animation_l.next_animation = next_animation_p;
		animation_l.one_shot = one_shot_p;

		// if fresh new animation we set it up
		if(handle_p.revision() == 0)
		{
			// set up resources
			animation_l.info.rid = RenderingServer::get_singleton()->canvas_item_create();
			animation_l.info.material = Ref<ShaderMaterial>(memnew(ShaderMaterial));
			animation_l.info.material->set_shader(shader_p);

			RenderingServer::get_singleton()->canvas_item_set_parent(animation_l.info.rid, parent_p);
			RenderingServer::get_singleton()->canvas_item_set_default_texture_filter(animation_l.info.rid, RenderingServer::CANVAS_ITEM_TEXTURE_FILTER_NEAREST);
			RenderingServer::get_singleton()->canvas_item_set_material(animation_l.info.rid, animation_l.info.material->get_rid());
		}
	}

	int EntityDrawer::add_instance(Vector2 const &pos_p, Vector2 const &offset_p, Ref<SpriteFrames> const & animation_p,
		StringName const &current_animation_p, StringName const &next_animation_p, bool one_shot_p, bool in_front_p)
	{
		EntityInstance entity_l;

		// animation
		entity_l.animation = animations.recycle_instance();
		set_up_animation(entity_l.animation, _elapsedAllTime, _shader, get_canvas_item(), offset_p, animation_p, current_animation_p, next_animation_p, one_shot_p);
		// reset z_index in case we reuse an instance for a sub instance
		RenderingServer::get_singleton()->canvas_item_set_z_index(entity_l.animation.get().info.rid, in_front_p? 1 : 0);

		// register instance
		smart_list_handle<EntityInstance> handle_l = _instances.new_instance(entity_l);
		// add payload
		_payload_handler->add_payload();

		// position
		handle_l.get().pos_idx = pos_indexes.recycle_instance();
		if(handle_l.get().pos_idx.get().idx < _newPos.size()
		&& handle_l.get().pos_idx.get().idx < _oldPos.size())
		{
			_newPos[handle_l.get().pos_idx.get().idx] = pos_p;
			_oldPos[handle_l.get().pos_idx.get().idx] = pos_p;
		}
		else
		{
			// update index
			handle_l.get().pos_idx.get().idx = _newPos.size();
			_newPos.push_back(pos_p);
			_oldPos.push_back(pos_p);
		}

		return int(handle_l.handle());
	}

	int EntityDrawer::add_sub_instance(int idx_ref_p, Vector2 const &offset_p, Ref<SpriteFrames> const & animation_p,
					StringName const &current_animation_p, StringName const &next_animation_p,
					bool one_shot_p, bool in_front_p, bool use_directions_p)
	{
		if(!_instances.is_valid(idx_ref_p))
		{
			return -1;
		}
		EntityInstance entity_l;

		// animation
		entity_l.animation = animations.recycle_instance();
		set_up_animation(entity_l.animation, _elapsedAllTime, _shader, get_canvas_item(), offset_p, animation_p, current_animation_p, next_animation_p, one_shot_p);
		RenderingServer::get_singleton()->canvas_item_set_z_index(entity_l.animation.get().info.rid, in_front_p ? 1 : -1);

		// copy reference for position and dir_handler
		entity_l.pos_idx = _instances.get(idx_ref_p).pos_idx;
		entity_l.main_instance = _instances.get_handle(idx_ref_p);
		if(use_directions_p)
		{
			entity_l.dir_handler = _instances.get(idx_ref_p).dir_handler;
			if(entity_l.dir_handler.is_valid())
			{
				DirectionalAnimation anim_l;
				init_animation(anim_l, entity_l.animation.get().current_animation);
				entity_l.dir_animation = dir_animations.new_instance(anim_l);
			}
		}

		// register instance
		smart_list_handle<EntityInstance> handle_l = _instances.new_instance(entity_l);
		// add payload
		_payload_handler->add_payload();

		// set up relation for main instance
		entity_l.main_instance.get().sub_instances.push_back(handle_l);

		return int(handle_l.handle());
	}

	void EntityDrawer::free_instance(int idx_p, bool skip_main_free_p)
	{
		EntityInstance &instance_l = _instances.get(idx_p);

		// free all components that cannot be inherited
		if(instance_l.animation.is_valid())
		{
			if(instance_l.animation.get().info.rid.is_valid())
				RenderingServer::get_singleton()->canvas_item_clear(instance_l.animation.get().info.rid);
			animations.free_instance(instance_l.animation);
		}
		if(instance_l.dir_animation.is_valid())
		{
			dir_animations.free_instance(instance_l.dir_animation);
		}
		if(instance_l.dyn_animation.is_valid())
		{
			dyn_animations.free_instance(instance_l.dyn_animation);
		}
		if(instance_l.alt_info.is_valid())
		{
			if(instance_l.alt_info.get().rid.is_valid())
				RenderingServer::get_singleton()->canvas_item_clear(instance_l.alt_info.get().rid);
			alt_infos.free_instance(instance_l.alt_info);
		}

		for(smart_list_handle<EntityInstance> subs_l : instance_l.sub_instances)
		{
			if(subs_l.is_valid())
			{
				free_instance(subs_l.handle(), true);
			}
		}

		// if we are a sub instance we release ourself from our main
		if(instance_l.main_instance.is_valid())
		{
			if(!skip_main_free_p)
			{
				std::list<smart_list_handle<EntityInstance> > & sub_instances_l = instance_l.main_instance.get().sub_instances;
				// remove itself
				for(auto it_l = sub_instances_l.begin() ; it_l != sub_instances_l.end() ; ++it_l )
				{
					if(it_l->handle() == idx_p)
					{
						sub_instances_l.erase(it_l);
						break;
					}
				}
			}
		}
		// else we can clear the direction handler
		else
		{
			pos_indexes.free_instance(instance_l.pos_idx);
			if(instance_l.dir_handler.is_valid())
			{
				dir_handlers.free_instance(instance_l.dir_handler);
			}
		}

		// free payload
		_payload_handler->free_payload(idx_p);
		_instances.free_instance(idx_p);
	}

	void EntityDrawer::set_direction(int idx_p, Vector2 const &direction_p)
	{
		EntityInstance &instance_l = _instances.get(idx_p);
		if(!instance_l.dir_handler.is_valid())
		{
			return;
		}
		DirectionHandler &handler_l = instance_l.dir_handler.get();
		int new_type = get_direction(direction_p, handler_l.has_up_down);
		if(new_type != DirectionHandler::NONE)
		{
			handler_l.type = new_type;
			handler_l.count = 0;
		}

		// instance_l.dir_handler.get().direction = direction_p;
	}

	void EntityDrawer::add_direction_handler(int idx_p, bool has_up_down_p)
	{
		EntityInstance &instance_l = _instances.get(idx_p);
		if(instance_l.dir_handler.is_valid()
		|| !instance_l.animation.is_valid())
		{
			return;
		}

		DirectionHandler handler_l;
		handler_l.has_up_down = has_up_down_p;
		handler_l.pos_idx = instance_l.pos_idx.get().idx;
		instance_l.dir_handler = dir_handlers.new_instance(handler_l);

		DirectionalAnimation anim_l;
		init_animation(anim_l, instance_l.animation.get().current_animation);
		instance_l.dir_animation = dir_animations.new_instance(anim_l);
	}

	void EntityDrawer::remove_direction_handler(int idx_p)
	{
		EntityInstance &instance_l = _instances.get(idx_p);
		if(instance_l.dir_handler.is_valid())
		{
			dir_handlers.free_instance(instance_l.dir_handler);
		}
		if(instance_l.dir_animation.is_valid())
		{
			dir_animations.free_instance(instance_l.dir_animation);
		}
	}

	void EntityDrawer::add_dynamic_animation(int idx_p, StringName const &idle_animation_p, StringName const &moving_animation_p)
	{
		EntityInstance &instance_l = _instances.get(idx_p);
		if(instance_l.dyn_animation.is_valid())
		{
			return;
		}
		DynamicAnimation dyn_l;
		init_animation(dyn_l.idle, idle_animation_p);
		init_animation(dyn_l.moving, moving_animation_p);
		instance_l.dyn_animation = dyn_animations.new_instance(dyn_l);
	}

	void EntityDrawer::add_pickable(int idx_p)
	{
		EntityInstance &instance_l = _instances.get(idx_p);
		if(instance_l.alt_info.is_valid())
		{
			return;
		}
		instance_l.alt_info = alt_infos.recycle_instance();
		RenderingInfo &info_l = instance_l.alt_info.get();

		// if fresh new animation we set it up
		if(instance_l.alt_info.revision() == 0 && _texture_catcher)
		{
			// set up resources
			info_l.rid = RenderingServer::get_singleton()->canvas_item_create();
			info_l.material = Ref<ShaderMaterial>(memnew(ShaderMaterial));
			info_l.material->set_shader(_alt_shader);

			RenderingServer::get_singleton()->canvas_item_set_parent(info_l.rid, _texture_catcher->get_alt_viewport()->get_canvas_item());
			RenderingServer::get_singleton()->canvas_item_set_default_texture_filter(info_l.rid, RenderingServer::CANVAS_ITEM_TEXTURE_FILTER_NEAREST);
			RenderingServer::get_singleton()->canvas_item_set_material(info_l.rid, info_l.material->get_rid());
		}
		info_l.material->set_shader_parameter("idx_color", color_from_idx(idx_p));
	}

	void EntityDrawer::remove_pickable(int idx_p)
	{
		EntityInstance &instance_l = _instances.get(idx_p);
		if(instance_l.alt_info.is_valid())
		{
			alt_infos.free_instance(instance_l.alt_info);
		}
	}

	void EntityDrawer::set_animation(int idx_p, StringName const &current_animation_p, StringName const &next_animation_p)
	{
		EntityInstance &instance_l = _instances.get(idx_p);
		if(!instance_l.animation.is_valid())
		{
			return;
		}
		/// IMPORTANT this has to be done before next_animation = next_animation_p because
		/// current_animation_p is a reference to old next_animation therefore updating it break
		/// the value
		if(instance_l.dir_animation.is_valid())
		{
			init_animation(instance_l.dir_animation.get(), current_animation_p);
		}
		instance_l.animation.get().current_animation = current_animation_p;
		instance_l.animation.get().next_animation = next_animation_p;
		instance_l.animation.get().frame_idx = 0;
		instance_l.animation.get().start = _elapsedAllTime;
		instance_l.animation.get().one_shot = false;
	}

	void EntityDrawer::set_proritary_animation(int idx_p, StringName const &current_animation_p, StringName const &next_animation_p)
	{
		EntityInstance &instance_l = _instances.get(idx_p);
		if(!instance_l.animation.is_valid())
		{
			return;
		}
		/// IMPORTANT this has to be done before next_animation = next_animation_p because
		/// current_animation_p is a reference to old next_animation therefore updating it break
		/// the value
		if(instance_l.dir_animation.is_valid())
		{
			init_animation(instance_l.dir_animation.get(), current_animation_p);
		}
		instance_l.animation.get().current_animation = current_animation_p;
		instance_l.animation.get().next_animation = next_animation_p;
		instance_l.animation.get().frame_idx = 0;
		instance_l.animation.get().start = _elapsedAllTime;
		instance_l.animation.get().one_shot = false;
		instance_l.animation.get().has_priority = true;
	}

	void EntityDrawer::set_animation_one_shot(int idx_p, StringName const &current_animation_p)
	{
		EntityInstance &instance_l = _instances.get(idx_p);
		if(!instance_l.animation.is_valid())
		{
			return;
		}
		instance_l.animation.get().current_animation = current_animation_p;
		instance_l.animation.get().frame_idx = 0;
		instance_l.animation.get().start = _elapsedAllTime;
		instance_l.animation.get().one_shot = true;

		if(instance_l.dir_animation.is_valid())
		{
			init_animation(instance_l.dir_animation.get(), current_animation_p);
		}
	}

	StringName const & EntityDrawer::get_animation(int idx_p) const
	{
		EntityInstance const &instance_l = _instances.get(idx_p);
		if(!instance_l.animation.is_valid())
		{
			static StringName none_l("");
			return none_l;
		}
		return instance_l.animation.get().current_animation;
	}

	void EntityDrawer::set_new_pos(int idx_p, Vector2 const &pos_p)
	{
		size_t const &pos_idx_l = _instances.get(idx_p).pos_idx.get().idx;
		_newPos[pos_idx_l] = pos_p;
	}

	Vector2 const & EntityDrawer::get_old_pos(int idx_p)
	{
		size_t const &pos_idx_l = _instances.get(idx_p).pos_idx.get().idx;
		return _oldPos[pos_idx_l];
	}

	void EntityDrawer::update_pos()
	{
		_elapsedTime = 0.;
		// swap positions
		std::swap(_oldPos, _newPos);
	}

	Ref<ShaderMaterial> EntityDrawer::get_shader_material(int idx_p)
	{
		EntityInstance const &instance_l = _instances.get(idx_p);
		if(!instance_l.animation.is_valid())
		{
			return Ref<ShaderMaterial>();
		}
		return instance_l.animation.get().info.material;
	}

	void EntityDrawer::set_shader_bool_params(String const &param_p, TypedArray<bool> const &values_p)
	{
		_instances.for_each([&](EntityInstance & instance_l, size_t idx_p) {
			if(instance_l.animation.is_valid()
			&& instance_l.animation.get().info.material.is_valid())
			{
				instance_l.animation.get().info.material->set_shader_parameter(param_p, values_p[idx_p]);
			}
		});
	}

	void EntityDrawer::set_shader_bool_params_from_indexes(String const &param_p, TypedArray<int> const &indexes_p, bool value_indexes_p)
	{
		// set for indexes
		for(size_t i = 0 ; i < indexes_p.size() ; ++ i)
		{
			int idx_l = indexes_p[i];
			if(_instances.is_valid(idx_l)
			&& _instances.get(idx_l).animation.is_valid()
			&& _instances.get(idx_l).animation.get().info.material.is_valid())
			{
				_instances.get(idx_l).animation.get().info.material->set_shader_parameter(param_p, value_indexes_p);
			}
		}
	}

	void EntityDrawer::set_all_shader_bool_params_from_indexes(String const &param_p, TypedArray<int> const &indexes_p, bool value_indexes_p, bool value_others_p)
	{
		// set all default values
		_instances.for_each([&](EntityInstance & instance_l, size_t ) {
			if(instance_l.animation.is_valid()
			&& instance_l.animation.get().info.material.is_valid())
			{
				instance_l.animation.get().info.material->set_shader_parameter(param_p, value_others_p);
			}
		});

		// set for indexes
		for(size_t i = 0 ; i < indexes_p.size() ; ++ i)
		{
			int idx_l = indexes_p[i];
			if(_instances.is_valid(idx_l)
			&& _instances.get(idx_l).animation.is_valid()
			&& _instances.get(idx_l).animation.get().info.material.is_valid())
			{
				_instances.get(idx_l).animation.get().info.material->set_shader_parameter(param_p, value_indexes_p);
			}
		}
	}

	TypedArray<int> EntityDrawer::indexes_from_texture(Rect2 const &rect_p) const
	{
		TypedArray<bool> all_added_l = index_array_from_texture(rect_p);

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

	TypedArray<bool> EntityDrawer::index_array_from_texture(Rect2 const &rect_p) const
	{
		if(!_texture_catcher)
		{
			return TypedArray<bool>();
		}
		// scale from texture viewport scale
		double scale_l = _texture_catcher->get_scale_viewport();
		Rect2i scale_rect_l = Rect2i(rect_p.get_position() / scale_l, rect_p.get_size() / scale_l);
		Ref<Image> image_l = _texture_catcher->get_texture()->get_image();
		TypedArray<bool> all_added_l;
		all_added_l.resize(_instances.size());
		all_added_l.fill(false);
		for(int32_t x = scale_rect_l.get_position().x ; x <= scale_rect_l.get_position().x + scale_rect_l.get_size().x ; ++ x)
		{
			for(int32_t y = scale_rect_l.get_position().y ; y <= scale_rect_l.get_position().y + scale_rect_l.get_size().y ; ++ y)
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

	int EntityDrawer::index_from_texture(Vector2 const &pos_p) const
	{
		if(!_texture_catcher)
		{
			return -1;
		}
		// scale from texture viewport scale
		double scale_l = _texture_catcher->get_scale_viewport();
		Vector2 scale_pos_l = pos_p / scale_l;

		return idx_from_color(safe_color(scale_pos_l.x, scale_pos_l.y, _texture_catcher->get_texture()->get_image()));
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

		_texture_catcher = memnew(TextureCatcher);
		_texture_catcher->set_scale_viewport(_scale_viewport);
		if(!_ref_camera_path.is_empty())
		{
			_texture_catcher->set_ref_camera("../"+_ref_camera_path);
		}
		add_child(_texture_catcher);
	}

	StringName const &get_anim(EntityInstance &instance_p)
	{
		if(!instance_p.dir_handler.is_valid())
		{
			return instance_p.animation.get().current_animation;
		}
		DirectionHandler const &handler_l = instance_p.dir_handler.get();
		int type_l = handler_l.type;
		if(type_l == DirectionHandler::NONE)
		{
			type_l = DirectionHandler::LEFT;
		}

		// forced directionl anim
		if(instance_p.dir_animation.is_valid()
		&& instance_p.dir_animation.get().base_name != StringName("")
		&& instance_p.animation.get().has_priority)
		{
			return instance_p.dir_animation.get().names[type_l];
		}

		if(instance_p.dyn_animation.is_valid()
		&& instance_p.dir_handler.is_valid())
		{
			if(handler_l.idle)
			{
				// non-forced directionl anim
				if(instance_p.dir_animation.is_valid()
				&& instance_p.dir_animation.get().base_name != StringName(""))
				{
					return instance_p.dir_animation.get().names[type_l];
				}
				return instance_p.dyn_animation.get().idle.names[type_l];
			}
			return instance_p.dyn_animation.get().moving.names[type_l];
		}
		return instance_p.animation.get().current_animation;
	}

	void EntityDrawer::_draw()
	{
		_instances.for_each([&](EntityInstance &instance_p, size_t idx_p) {
			StringName cur_anim_l = get_anim(instance_p);
			std::string test_l(cur_anim_l.substr(0,-1).utf8().get_data());
			if(!instance_p.animation.is_valid())
			{
				return;
			}
			bool is_over_l = false;
			AnimationInstance & animation_l = instance_p.animation.get();
			if(animation_l.animation.is_valid() && animation_l.animation->get_frame_count(cur_anim_l) > 0)
			{
				double frameTime_l = animation_l.animation->get_frame_duration(cur_anim_l, animation_l.frame_idx) / animation_l.animation->get_animation_speed(cur_anim_l) ;
				double nextFrameTime_l = animation_l.start + frameTime_l;
				if(_elapsedAllTime >= nextFrameTime_l)
				{
					++animation_l.frame_idx;
					animation_l.start = _elapsedAllTime;
				}
				if(animation_l.frame_idx >= animation_l.animation->get_frame_count(cur_anim_l))
				{
					if(animation_l.one_shot)
					{
						free_instance(idx_p);
						is_over_l = true;
					}
					else if(animation_l.next_animation != StringName(""))
					{
						set_animation(idx_p, animation_l.next_animation, StringName(""));
						cur_anim_l = get_anim(instance_p);
					}
					// if dynamic animation and no chaining we reset
					else if(instance_p.dyn_animation.is_valid())
					{
						set_animation(idx_p, StringName(""), StringName(""));
						cur_anim_l = get_anim(instance_p);
					}
					animation_l.frame_idx = 0;
				}
				// if still enabled
				if(!is_over_l)
				{
					size_t pos_idx_l = instance_p.pos_idx.get().idx;
					Vector2 diff_l = _newPos[pos_idx_l] - _oldPos[pos_idx_l];
					Vector2 pos_l =  (_oldPos[pos_idx_l] + diff_l * std::min<real_t>(1., real_t(_elapsedTime/_timeStep))) * _scale;
					// draw animaton
					Ref<Texture2D> texture_l = animation_l.animation->get_frame_texture(cur_anim_l, animation_l.frame_idx);
					RenderingServer::get_singleton()->canvas_item_set_transform(animation_l.info.rid, Transform2D(0., pos_l));
					RenderingServer::get_singleton()->canvas_item_clear(animation_l.info.rid);

					// required when empty texture in sprite frame
					if(texture_l.is_valid())
					{
						// classic rendering
						texture_l->draw(animation_l.info.rid, animation_l.offset);
						// alternate rendering
						if(instance_p.alt_info.is_valid()
						&& instance_p.alt_info.get().rid.is_valid())
						{
							RenderingInfo &alt_info_l = instance_p.alt_info.get();
							RenderingServer::get_singleton()->canvas_item_set_transform(alt_info_l.rid, Transform2D(0., pos_l));
							RenderingServer::get_singleton()->canvas_item_clear(alt_info_l.rid);
							texture_l->draw(alt_info_l.rid, animation_l.offset);
						}
					}
				}
			}
		});
	}

	void EntityDrawer::_physics_process(double delta_p)
	{
		dir_handlers.for_each([&](DirectionHandler &handler_p) {
			Vector2 dir_l = handler_p.direction;
			if(dir_l.length_squared() < 0.1)
			{
				dir_l = _newPos[handler_p.pos_idx] - _oldPos[handler_p.pos_idx];
			}
			int new_type = get_direction(dir_l, handler_p.has_up_down);
			if(new_type != DirectionHandler::NONE)
			{
				handler_p.count_idle = 0;
				handler_p.idle = false;
				if(new_type != handler_p.count_type)
				{
					handler_p.count = 0;
					handler_p.count_type = new_type;
				}
				if(handler_p.count < 100)
				{
					++handler_p.count;
				}
			}
			else
			{
				++handler_p.count_idle;
				handler_p.count = 0;
				handler_p.count_type = DirectionHandler::NONE;
			}

			if(handler_p.count > 1
			&& handler_p.count_type != DirectionHandler::NONE
			&& handler_p.count_type != handler_p.type)
			{
				handler_p.type = handler_p.count_type;
			}
			if(handler_p.count_idle > 1)
			{
				handler_p.idle = true;
			}
		});
	}

	void EntityDrawer::_process(double delta_p)
	{
		_elapsedTime += delta_p;
		_elapsedAllTime += delta_p;
		queue_redraw();
	}

	void EntityDrawer::_bind_methods()
	{
		ClassDB::bind_method(D_METHOD("add_instance", "position", "offset", "animation", "current_animation", "next_animation", "one_shot"), &EntityDrawer::add_instance);
		ClassDB::bind_method(D_METHOD("add_sub_instance", "idx_ref", "offset", "animation", "current_animation", "next_animation", "one_shot", "in_front", "use_directions"), &EntityDrawer::add_sub_instance);
		ClassDB::bind_method(D_METHOD("free_instance", "idx"), &EntityDrawer::free_instance);
		ClassDB::bind_method(D_METHOD("update_pos"), &EntityDrawer::update_pos);

		ClassDB::bind_method(D_METHOD("set_animation", "instance", "current_animation", "next_animation"), &EntityDrawer::set_animation);
		ClassDB::bind_method(D_METHOD("set_proritary_animation", "instance", "current_animation", "next_animation"), &EntityDrawer::set_proritary_animation);
		ClassDB::bind_method(D_METHOD("set_animation_one_shot", "instance", "current_animation"), &EntityDrawer::set_animation_one_shot);
		ClassDB::bind_method(D_METHOD("set_direction", "instance", "direction"), &EntityDrawer::set_direction);
		ClassDB::bind_method(D_METHOD("add_direction_handler", "instance", "has_up_down"), &EntityDrawer::add_direction_handler);
		ClassDB::bind_method(D_METHOD("remove_direction_handler", "instance"), &EntityDrawer::remove_direction_handler);
		ClassDB::bind_method(D_METHOD("add_dynamic_animation", "instance", "idle_animation", "moving_animation"), &EntityDrawer::add_dynamic_animation);
		ClassDB::bind_method(D_METHOD("set_new_pos", "instance", "pos"), &EntityDrawer::set_new_pos);
		ClassDB::bind_method(D_METHOD("get_old_pos", "instance"), &EntityDrawer::get_old_pos);
		ClassDB::bind_method(D_METHOD("get_shader_material", "instance"), &EntityDrawer::get_shader_material);
		ClassDB::bind_method(D_METHOD("set_shader_bool_params", "param", "values"), &EntityDrawer::set_shader_bool_params);
		ClassDB::bind_method(D_METHOD("set_shader_bool_params_from_indexes", "param", "indexes", "value_index"), &EntityDrawer::set_shader_bool_params_from_indexes);
		ClassDB::bind_method(D_METHOD("set_all_shader_bool_params_from_indexes", "param", "indexes", "value_index", "value_other"), &EntityDrawer::set_all_shader_bool_params_from_indexes);
		ClassDB::bind_method(D_METHOD("set_shader", "material"), &EntityDrawer::set_shader);

		ClassDB::bind_method(D_METHOD("set_time_step", "time_step"), &EntityDrawer::set_time_step);

		ClassDB::bind_method(D_METHOD("indexes_from_texture", "rect"), &EntityDrawer::indexes_from_texture);
		ClassDB::bind_method(D_METHOD("index_array_from_texture", "rect"), &EntityDrawer::index_array_from_texture);
		ClassDB::bind_method(D_METHOD("index_from_texture", "pos"), &EntityDrawer::index_from_texture);

		// properties
		ClassDB::bind_method(D_METHOD("get_scale_viewport"), &EntityDrawer::get_scale_viewport);
		ClassDB::bind_method(D_METHOD("set_scale_viewport", "scale_viewport"), &EntityDrawer::set_scale_viewport);
		ClassDB::add_property("EntityDrawer", PropertyInfo(Variant::FLOAT, "scale_viewport"), "set_scale_viewport", "get_scale_viewport");

		ClassDB::bind_method(D_METHOD("get_ref_camera"), &EntityDrawer::get_ref_camera);
		ClassDB::bind_method(D_METHOD("set_ref_camera", "ref_camera"), &EntityDrawer::set_ref_camera);
		ClassDB::add_property("EntityDrawer", PropertyInfo(Variant::NODE_PATH, "ref_camera", PROPERTY_HINT_NODE_PATH_VALID_TYPES, "Camera2D"), "set_ref_camera", "get_ref_camera");

		ClassDB::bind_method(D_METHOD("set_debug", "debug"), &EntityDrawer::set_debug);
		ClassDB::bind_method(D_METHOD("is_debug"), &EntityDrawer::is_debug);
		ClassDB::add_property("EntityDrawer", PropertyInfo(Variant::BOOL, "debug"), "set_debug", "is_debug");

		ADD_GROUP("EntityDrawer", "EntityDrawer_");
	}

	void EntityDrawer::setup_payload(AbstractEntityPayload * payload_hanlder_p)
	{
		if(_instances.size() > 0)
		{
			throw std::logic_error("cannot setup payload with instances.");
		}
		delete _payload_handler;
		_payload_handler = payload_hanlder_p;
	}

	void EntityDrawer::set_debug(bool debug_p) { if(_texture_catcher) _texture_catcher->set_debug(debug_p); }
	bool EntityDrawer::is_debug() const { if(_texture_catcher) return _texture_catcher->is_debug(); else return false; }

} // namespace godot


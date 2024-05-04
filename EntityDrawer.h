#pragma once

#include <godot_cpp/godot.hpp>
#include <godot_cpp/classes/node2d.hpp>
#include <godot_cpp/classes/atlas_texture.hpp>
#include <godot_cpp/classes/shader_material.hpp>
#include <godot_cpp/classes/sprite_frames.hpp>

#include <array>

#include "smart_list/smart_list.h"

namespace godot {

struct PositionIndex
{
	size_t idx = 999999999;
};

struct RenderingInfo
{
	RID rid;
	Ref<ShaderMaterial> material;
};

struct AnimationInstance
{
	/// @brief offset to apply to the texture to display it
	Vector2 offset;
	Ref<SpriteFrames> animation;
	bool enabled = true;
	double start = 0.;
	int frame_idx = 0;
	StringName current_animation;
	StringName next_animation;
	/// @brief will be destroyed after the end of the animation
	bool one_shot = false;

	RenderingInfo info;
};

struct DirectionalAnimation
{
	/// @brief directed names
	std::array<StringName, 4> names;
	/// @brief base name
	StringName base_name;
};

struct DirectionHandler
{
	// static data
	static int const NONE = -1;
	static int const UP = 0;
	static int const DOWN = 1;
	static int const LEFT = 2;
	static int const RIGHT = 3;

	/// @brief can use top down?
	bool has_up_down = true;
	/// @brief position index to be used
	size_t pos_idx = 0;

	// dynamic data
	Vector2 direction;
	// current direction
	int type = -1;

	// tracking of evolution
	char count = 0;
	char count_idle = 0;
	int count_type = -1;

	// moving or idle
	bool idle = false;
};

struct DynamicAnimation
{
	DirectionalAnimation idle;
	DirectionalAnimation moving;
};

struct EntityInstance
{
	/////
	// Basic
	/////

	/// @brief index to use in the position index
	smart_list_handle<PositionIndex> pos_idx;
	smart_list_handle<AnimationInstance> animation;

	/////
	// Directional
	/////
	smart_list_handle<DirectionHandler> dir_handler;
	smart_list_handle<DirectionalAnimation> dir_animation;

	/////
	// Dynamic
	/////
	smart_list_handle<DynamicAnimation> dyn_animation;

	/////
	// Pickable
	/////
	smart_list_handle<RenderingInfo> alt_info;

	// relation links
	std::list<smart_list_handle<EntityInstance> > sub_instances;
	smart_list_handle<EntityInstance> main_instance;
};

class EntityDrawer : public Node2D {
	GDCLASS(EntityDrawer, Node2D)

public:
	~EntityDrawer();

	// creating instances
	int add_instance(Vector2 const &pos_p, Vector2 const &offset_p, Ref<SpriteFrames> const & animation_p,
		StringName const &current_animation_p, StringName const &next_animation_p, bool one_shot_p);
	/// @todo add in front or in back possibility
	int add_sub_instance(int idx_ref_p, Vector2 const &offset_p, Ref<SpriteFrames> const & animation_p,
					StringName const &current_animation_p, StringName const &next_animation_p, bool one_shot_p);
	void free_instance(int idx_p);

	// direction handling
	void set_direction(int idx_p, Vector2 const &direction_p);
	void add_direction_handler(int idx_p, bool has_up_down_p);
	void remove_direction_handler(int idx_p);

	// dynamic animation handling
	void add_dynamic_animation(int idx_p, StringName const &idle_animation_p, StringName const &moving_animation_p);

	// pickable handling
	void add_pickable(int idx_p);
	void remove_pickable(int idx_p);

	// animation getters/setters
	void set_animation(int idx_p, StringName const &current_animation_p, StringName const &next_animation_p);
	void set_animation_one_shot(int idx_p, StringName const &current_animation_p);
	StringName const & get_animation(int idx_p) const;

	// position handling
	void set_new_pos(int idx_p, Vector2 const &pos_p);
	Vector2 const & get_old_pos(int idx_p);
	void update_pos();

	// shader handling
	Ref<ShaderMaterial> get_shader_material(int idx_p);
	void set_shader_bool_params(String const &param_p, TypedArray<bool> const &values_p);
	void set_shader_bool_params_from_indexes(String const &param_p, TypedArray<int> const &indexes_p, bool value_indexes_p, bool value_others_p);

	/// getters for alternative rendering
	TypedArray<int> indexes_from_texture(Rect2i const &rect_p, Ref<Texture2D> const &texture_p) const;
	TypedArray<bool> index_array_from_texture(Rect2i const &rect_p, Ref<Texture2D> const &texture_p) const;
	int index_from_texture(Vector2i const &pos_p, Ref<Texture2D> const &texture_p) const;

	// godot routines
	void _ready() override;
	void _draw() override;
	void _physics_process(double delta_p) override;
	void _process(double delta_p) override;

	// Will be called by Godot when the class is registered
	// Use this to add properties to your class
	static void _bind_methods();

	// set up
	void set_time_step(double timeStep_p) { _timeStep = timeStep_p; }
	void set_shader(Ref<Shader> const &shader_p) { _shader = shader_p; }
	void set_alt_viewport(Node2D *alt_viewport_p) { _alt_viewport = alt_viewport_p; }

private:
	Ref<Shader> _shader;

	smart_list<EntityInstance> _instances;

	// smart list for components
	smart_list<AnimationInstance> animations;
	smart_list<DirectionHandler> dir_handlers;
	smart_list<DirectionalAnimation> dir_animations;
	smart_list<DynamicAnimation> dyn_animations;
	smart_list<RenderingInfo> alt_infos;

	/// @brief last position of instances to lerp
	std::vector<Vector2> _newPos;
	std::vector<Vector2> _oldPos;
	smart_list<PositionIndex> pos_indexes;

	/// @brief expected duration of a timestep
	double _timeStep = 0.01;

	/// @brief time since beginning
	double _elapsedAllTime = 0.;

	/// @brief time since last position update
	double _elapsedTime = 0.;

	/// @brief an alternative rendering layer used to render the entities
	/// differently (used for mouse picking)
	Node2D *_alt_viewport = nullptr;
	Ref<Shader> _alt_shader;

	double const _scale = 1.;
};

}

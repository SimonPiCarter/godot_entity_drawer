#pragma once

#include <godot_cpp/godot.hpp>
#include <godot_cpp/classes/node2d.hpp>
#include <godot_cpp/classes/atlas_texture.hpp>
#include <godot_cpp/classes/shader_material.hpp>
#include <godot_cpp/classes/sprite_frames.hpp>

#include <array>

namespace godot {

struct EntityInstance
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
	bool one_shot = true;
	/// @brief true if h mirrored
	bool mirrored = false;
	/// @brief index to direction handler (if >= 0)
	int handler = -1;

	// GODOT RID used for efficient display
	RID _canvas;
	// rid used for alternate rendering
	RID _alt_canvas;
	Ref<ShaderMaterial> _material;
	Ref<ShaderMaterial> _alt_material;
};

struct DirectionHandler
{
	// static data
	static int const NONE = -1;
	static int const UP = 0;
	static int const DOWN = 1;
	static int const LEFT = 2;
	static int const RIGHT = 3;

	// dynamic data
	Vector2 direction;
	int type = -1;

	char count = 0;
	int count_type = -1;

	int instance = 0;

	/// @brief directed names
	std::array<StringName, 4> names;
	/// @brief base name
	StringName base_name;
	bool has_up_down = true;

	bool enabled = true;
};

class EntityDrawer : public Node2D {
	GDCLASS(EntityDrawer, Node2D)

public:
	~EntityDrawer();

	int add_instance(Vector2 const &pos_p, Vector2 const &offset_p, Ref<SpriteFrames> const & animation_p,
		StringName const &current_animation_p, StringName const &next_animation_p, bool one_shot_p);

	StringName const & get_animation(int idx_p) const;
	void set_animation(int idx_p, StringName const &current_animation_p, StringName const &next_animation_p);
	void set_animation_one_shot(int idx_p, StringName const &current_animation_p);
	void set_direction(int idx_p, Vector2 const &direction_p);
	void add_direction_handler(int idx_p, bool has_up_down_p);
	void remove_direction_handler(int idx_p);
	void set_new_pos(int idx_p, Vector2 const &pos_p);
	Vector2 const & get_old_pos(int idx_p);
	Ref<ShaderMaterial> get_shader_material(int idx_p);

	TypedArray<int> indexes_from_texture(Rect2i const &rect_p, Ref<Texture2D> const &texture_p) const;
	int index_from_texture(Vector2i const &pos_p, Ref<Texture2D> const &texture_p) const;

	void update_pos();
	std::vector<Vector2> & getNewPos();
	std::vector<EntityInstance> &getInstances() { return _instances; }

	void _ready() override;
	void _draw() override;
	void _physics_process(double delta_p) override;
	void _process(double delta_p) override;

	// Will be called by Godot when the class is registered
	// Use this to add properties to your class
	static void _bind_methods();

	void set_time_step(double timeStep_p) { _timeStep = timeStep_p; }
	void set_shader(Ref<Shader> const &shader_p) { _shader = shader_p; }
	void set_alt_viewport(Node2D *alt_viewport_p) { _alt_viewport = alt_viewport_p; }

private:
	Ref<Shader> _shader;

	std::vector<EntityInstance> _instances;
	std::vector<DirectionHandler> _directionHandlers;
	std::list<size_t> _freeIdx;
	std::list<size_t> _freeHandlersIdx;

	/// @brief last position of instances to lerp
	std::vector<Vector2> _newPos;
	std::vector<Vector2> _oldPos;

	/// @brief expected duration of a timestep
	double _timeStep = 0.01;

	/// @brief time since beginning
	double _elapsedAllTime = 0.;

	/// @brief time since last position update
	double _elapsedTime = 0.;

	/// @brief an alternative rendering layer used to redner the entities
	/// differently (used for mouse picking)
	Node2D *_alt_viewport = nullptr;
	Ref<Shader> _alt_shader;
};

}

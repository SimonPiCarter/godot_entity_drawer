#pragma once

#ifdef GD_EXTENSION_GODOCTOPUS
	#include <godot_cpp/godot.hpp>
	#include <godot_cpp/classes/node.hpp>
	#include <godot_cpp/classes/sprite_frames.hpp>
#else
	#include "scene/main/node.h"
	#include "scene/resources/sprite_frames.h"
#endif

#include <string>
#include <unordered_map>

namespace godot {

struct FrameInfo
{
	Ref<SpriteFrames> sprite_frame;
	Vector2 offset;
	bool has_up_down = true;
};

class FramesLibrary : public Node {
	GDCLASS(FramesLibrary, Node)

public:
	void addFrame(String const &name_p, Ref<SpriteFrames> const &frame_p, Vector2 const &offset_p, bool has_up_down_p);

	FrameInfo const & getFrameInfo(std::string const &name_p);
	FrameInfo const * tryGetFrameInfo(std::string const &name_p) const;

	// Will be called by Godot when the class is registered
	// Use this to add properties to your class
	static void _bind_methods();

private:
	std::unordered_map<std::string, FrameInfo > _mapFrames;
};

}

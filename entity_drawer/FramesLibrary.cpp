#include "FramesLibrary.h"

namespace godot {

void FramesLibrary::addFrame(String const &name_p, Ref<SpriteFrames> const &frame_p, Vector2 const &offset_p, bool has_up_down_p)
{
	std::string name_l(name_p.utf8().get_data());
	_mapFrames[name_l] = { frame_p, offset_p, has_up_down_p };
}

FrameInfo const & FramesLibrary::getFrameInfo(std::string const &name_p)
{
	return _mapFrames.at(name_p);
}

void FramesLibrary::_bind_methods()
{
	ClassDB::bind_method(D_METHOD("addFrame", "name", "frame", "offset" "has_up_down"), &FramesLibrary::addFrame);

	ADD_GROUP("FramesLibrary", "FramesLibrary_");
}

}

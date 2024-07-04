#include "TextureCatcher.h"
#include <godot_cpp/variant/utility_functions.hpp>

#include <godot_cpp/classes/color_rect.hpp>
#include <godot_cpp/classes/viewport_texture.hpp>
#include <godot_cpp/classes/window.hpp>

namespace godot {

void TextureCatcher::_ready()
{
	set_texture_filter(CanvasItem::TextureFilter::TEXTURE_FILTER_NEAREST);
	_ref_camera = get_node<Camera2D>(_ref_camera_path);
	// set up basic tree
	// - Subviewport : viewport used to render the picking texture
	//   - CanvasLayer
	//     - ColorRect : white background
	//   - Camera2D : camera of the viewport to mimic the main camera
	//   - Sprite2D : the sprite used to render the picking draw calls
	// - CanvasLayer : visible for debug mode
	//   - TextureRect : used to render the viewport

	// - Subviewport : viewport used to render the picking texture
	_sub_viewport = memnew(SubViewport);
	add_child(_sub_viewport);
	_sub_viewport->set_update_mode(SubViewport::UPDATE_ALWAYS);
	_sub_viewport->set_canvas_cull_mask(2);
	_sub_viewport->set_disable_3d(true);

	//   - CanvasLayer
	CanvasLayer *canvas_layer_sub_l = memnew(CanvasLayer);
	_sub_viewport->add_child(canvas_layer_sub_l);
	canvas_layer_sub_l->set_layer(-1);

	//     - ColorRect : white background
	ColorRect *color_rect_l = memnew(ColorRect);
	canvas_layer_sub_l->add_child(color_rect_l);
	color_rect_l->set_size(Vector2(512,512));
	color_rect_l->set_anchors_preset(Control::LayoutPreset::PRESET_FULL_RECT);
	color_rect_l->set_visibility_layer(2);

	//   - Camera2D : camera of the viewport to mimic the main camera
	_camera = memnew(Camera2D);
	_sub_viewport->add_child(_camera);
	_camera->set_visibility_layer(2);

	//   - Sprite2D : the sprite used to render the picking draw calls
	_alt_viewport = memnew(Sprite2D);
	_sub_viewport->add_child(_alt_viewport);
	_alt_viewport->set_visibility_layer(2);
	_alt_viewport->set_y_sort_enabled(true);

	// - CanvasLayer : visible for debug mode
	_debug_canvas = memnew(CanvasLayer);
	add_child(_debug_canvas);
	_debug_canvas->set_visible(false);

	//   - TextureRect : used to render the viewport
	_texture = memnew(TextureRect);
	_debug_canvas->add_child(_texture);
	_texture->set_stretch_mode(TextureRect::StretchMode::STRETCH_KEEP);
	_texture->set_texture(_sub_viewport->get_texture());

	// Handle ref camera
	if(_ref_camera)
	{
		_ref_camera->get_viewport()->connect("size_changed", Callable(this, "_on_size_changed"));
		_on_size_changed();
	}

	// handle texture scale
	_texture->set_scale(Vector2(_scale_viewport, _scale_viewport));
}

void TextureCatcher::_process(double delta_p)
{
	if(_ref_camera)
	{
		_camera->set_position(_ref_camera->get_position());
		_camera->set_zoom(_ref_camera->get_zoom() / _scale_viewport);
	}
	_texture->set_texture(_sub_viewport->get_texture());
}

void TextureCatcher::_on_size_changed()
{
	SubViewport * sub_l = dynamic_cast<SubViewport *>(_ref_camera->get_viewport());
	Window * window_l = dynamic_cast<Window *>(_ref_camera->get_viewport());
	if(sub_l)
	{
		_sub_viewport->set_size(sub_l->get_size() / _scale_viewport);
	}
	if(window_l)
	{
		_sub_viewport->set_size(window_l->get_size() / _scale_viewport);
	}
}

void TextureCatcher::_bind_methods()
{
	ClassDB::bind_method(D_METHOD("_on_size_changed"), &TextureCatcher::_on_size_changed);
	ClassDB::bind_method(D_METHOD("set_debug", "debug"), &TextureCatcher::set_debug);
	ClassDB::bind_method(D_METHOD("is_debug"), &TextureCatcher::is_debug);
	ClassDB::bind_method(D_METHOD("get_alt_viewport"), &TextureCatcher::get_alt_viewport);

	ClassDB::bind_method(D_METHOD("get_scale_viewport"), &TextureCatcher::get_scale_viewport);
	ClassDB::bind_method(D_METHOD("set_scale_viewport", "scale_viewport"), &TextureCatcher::set_scale_viewport);
	ClassDB::add_property("TextureCatcher", PropertyInfo(Variant::FLOAT, "scale_viewport"), "set_scale_viewport", "get_scale_viewport");

	ClassDB::bind_method(D_METHOD("get_ref_camera"), &TextureCatcher::get_ref_camera);
	ClassDB::bind_method(D_METHOD("set_ref_camera", "ref_camera"), &TextureCatcher::set_ref_camera);
	ClassDB::add_property("TextureCatcher", PropertyInfo(Variant::NODE_PATH, "ref_camera", PROPERTY_HINT_NODE_PATH_VALID_TYPES, "Camera2D"), "set_ref_camera", "get_ref_camera");

	ADD_GROUP("TextureCatcher", "TextureCatcher_");
}

}

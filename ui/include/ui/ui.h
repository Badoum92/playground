#pragma once
#include <cross/buttons.h>
#include <exo/maths/numerics.h>

#include "painter/color.h"
#include "painter/rect.h"

#include <string_view>

struct Font;
struct Painter;

inline constexpr u32 UI_MAX_DEPTH = 128;

namespace ui
{
struct Theme
{
	// Color scheme
	ColorU32 accent_color = ColorU32::from_uints(0x10, 0x75, 0xB2);

	// Widget colors
	ColorU32 button_bg_color         = ColorU32::from_floats(1.0f, 1.0f, 1.0f, 0.3f);
	ColorU32 button_hover_bg_color   = ColorU32::from_uints(0, 0, 0, 0x06);
	ColorU32 button_pressed_bg_color = ColorU32::from_uints(0, 0, 0, 0x09);
	ColorU32 button_label_color      = ColorU32::from_uints(0, 0, 0, 0xFF);

	float    input_thickness          = 10.0f;
	float    splitter_thickness       = 2.0f;
	float    splitter_hover_thickness = 4.0f;
	ColorU32 splitter_color           = ColorU32::from_greyscale(u8(0xE5));
	ColorU32 splitter_hover_color     = ColorU32::from_greyscale(u8(0xD1));

	ColorU32 scroll_area_bg_color  = ColorU32::from_uints(0, 0, 0, 0x44);
	ColorU32 scroll_bar_bg_color   = ColorU32::from_uints(0xDD, 0xDD, 0xFF, 0x22);
	ColorU32 scroll_thumb_bg_color = accent_color;

	Font *main_font = nullptr;
	float font_size = 14.0f;
};

struct Inputs
{
	exo::EnumArray<bool, exo::MouseButton> mouse_buttons_pressed            = {};
	exo::EnumArray<bool, exo::MouseButton> mouse_buttons_pressed_last_frame = {};
	int2                                   mouse_position                   = {0};
};

struct Activation
{
	u64 focused = 0;
	u64 active  = 0;
	u64 gen     = 0;
};

struct State
{
	u32 i_clip_rect              = u32_invalid;
	u32 clip_stack[UI_MAX_DEPTH] = {};
	u32 i_clip_stack             = 0;

	// Stack containing the "starting" rect for each scroll area, the "starting" rect is the inner content position with
	// a max size
	Rect scroll_starting_stack[UI_MAX_DEPTH] = {};
	// Stack containing the "ending" rect for each scroll area, the "ending" rect is the "starting" rect split by the
	// user content Substracting the pos from the "ending" rect to the "starting" rect gives the actual content size
	Rect scroll_ending_stack[UI_MAX_DEPTH] = {};
	// Index of the top of the stack
	u32 i_scroll_stack = 0;

	// custom mouse cursor
	int cursor = 0;

	float2 active_drag_offset = {};
};

struct Ui
{
	Theme      theme;
	Inputs     inputs;
	Activation activation;
	State      state;
	Painter   *painter;
};

struct Button
{
	const char *label;
	Rect        rect;
};

Ui   create(Font *font, float font_size, Painter *painter);
void new_frame(Ui &ui);
void end_frame(Ui &ui);

// helpers
inline float2 mouse_position(const Ui &ui) { return float2(ui.inputs.mouse_position); }
bool          is_hovering(const Ui &ui, const Rect &rect);
u64           make_id(Ui &ui);
float         em(const Ui &ui);

// widget api
bool has_pressed(const Ui &ui, exo::MouseButton button = exo::MouseButton::Left);
bool has_pressed_and_released(const Ui &ui, exo::MouseButton button = exo::MouseButton::Left);
bool has_clicked(const Ui &ui, u64 id, exo::MouseButton button = exo::MouseButton::Left);

const Rect &current_clip_rect(const Ui &ui);
bool        is_clipped(const Ui &ui, const Rect &rect);

u32  register_clip_rect(Ui &ui, const Rect &clip_rect);
void push_clip_rect(Ui &ui, u32 i_clip_rect);
void pop_clip_rect(Ui &ui);

bool button(Ui &ui, const Button &button);
void splitter_x(Ui &ui, const Rect &view_rect, float &value);
void splitter_y(Ui &ui, const Rect &view_rect, float &value);
void rect(Ui &ui, const Rect &rect);

enum struct Alignment
{
	Center
};
void label_in_rect(Ui &ui, const Rect &view_rect, std::string_view label, Alignment alignment = Alignment::Center);
void label_split(Ui &ui, RectSplit &rectsplit, std::string_view label);
bool button_split(Ui &ui, RectSplit &rectsplit, std::string_view label);

} // namespace ui

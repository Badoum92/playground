#pragma once
#include <exo/base/option.h>
#include <exo/maths/vectors.h>
#include <exo/collections/vector.h>
#include <exo/collections/enum_array.h>
#include <exo/cross/prelude.h>

#include <string>
#include <string_view>
#include <variant>

#if defined(CROSS_WINDOWS)
typedef struct HWND__ *HWND;
typedef struct HGLRC__ *HGLRC;
#else
#    include <xcb/xcb.h>
struct xkb_context;
struct xkb_keymap;
struct xkb_state;
#endif

struct ScopeStack;

enum struct MouseButton : uint
{
    Left,
    Right,
    Middle,
    SideForward,
    SideBackward,
    Count
};

inline constexpr EnumArray<const char *, MouseButton> mouse_button_to_string{
    "Left mouse button",
    "Right mouse button",
    "Middle mouse button (wheel)",
    "Side mouse button forward",
    "Side mouse button backward",
};

inline constexpr const char *to_string(MouseButton button) { return mouse_button_to_string[button]; }

enum struct VirtualKey : uint
{
#define X(EnumName, DisplayName, Win32, Xlib) EnumName,
#include "exo/cross/window_keys.def"
#undef X
    Count
};

inline constexpr EnumArray<const char *, VirtualKey> key_to_string{
#define X(EnumName, DisplayName, Win32, Xlib) DisplayName,
#include "exo/cross/window_keys.def"
#undef X
};

inline constexpr const char *to_string(VirtualKey key) { return key_to_string[key]; }

enum class ButtonState
{
    Pressed,
    Released
};

inline constexpr const char *to_string(ButtonState state) { return state == ButtonState::Pressed ? "Pressed" : "Released"; }

namespace cross
{

enum struct Cursor
{
    None,
    Arrow,
    TextInput,
    ResizeAll,
    ResizeEW,
    ResizeNS,
    ResizeNESW,
    ResizeNWSE,
    Hand,
    NotAllowed
};

namespace event
{

struct Key
{
    VirtualKey key;
    ButtonState state;
};

struct MouseClick
{
    MouseButton button;
    ButtonState state;
};

struct Char
{
    std::string char_sequence;
};

struct IMEComposition
{
    std::string composition;
};

struct IMECompositionResult
{
    std::string result;
};

struct Scroll
{
    int dx;
    int dy;
};

struct MouseMove
{
    int x;
    int y;
};

struct Focus
{
    bool focused;
};

struct Resize
{
    uint width;
    uint height;
};

using Event = std::variant<Key, MouseClick, Char, IMEComposition, IMECompositionResult, Scroll, MouseMove, Focus, Resize>;

} // namespace event

struct Caret
{
    int2 position{0};
    int2 size{0};
};

struct Window;

#if defined(CROSS_WINDOWS)
struct Window_Win32
{
    HWND window;
};
#else
struct Window_Xcb
{
    xcb_connection_t *connection;
    xcb_window_t window;

    int device_id;
    xkb_context *kb_ctx;
    xkb_state *kb_state;
    xkb_keymap *keymap;
    xcb_intern_atom_reply_t *close_reply;
};
#endif

struct Window
{
    static Window *create(ScopeStack &scope, u32 width, u32 height, const std::string_view title);
    ~Window();

    void set_title(std::string && new_title);
    void poll_events();

    // caret operations
    void set_caret_pos(int2 pos);
    void set_caret_size(int2 size);
    void remove_caret();

    // cursor operations
    void set_cursor(Cursor cursor);

    [[nodiscard]] inline bool should_close() const { return stop; }

    [[nodiscard]] float2 get_dpi_scale() const;

    [[nodiscard]] inline bool is_key_pressed(VirtualKey key) const { return keys_pressed[key]; }
    [[nodiscard]] inline bool is_mouse_button_pressed(MouseButton button) const
    {
        return mouse_buttons_pressed[button];
    }
    [[nodiscard]] inline float2 get_mouse_position() const { return mouse_position; }

    // push events to the events vector
    template <typename EventType> void push_event(EventType &&event) { events.push_back(std::move(event)); }

    std::string title;
    uint width;
    uint height;
    float2 mouse_position = {0.0f};

    bool stop{false};
    Option<Caret> caret{};

    bool has_focus{false};
    bool minimized{false};
    bool maximized{false};
    Cursor current_cursor;

    Vec<event::Event> events;

    EnumArray<bool, VirtualKey> keys_pressed           = {};
    EnumArray<bool, MouseButton> mouse_buttons_pressed = {};

#if defined(CROSS_WINDOWS)
    Window_Win32 win32;
#else
    Window_Xcb xcb;
#endif
};

} // namespace cross
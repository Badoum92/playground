#pragma once
#include <exo/macros/packed.h>
#include <exo/maths/vectors.h>
#include <exo/collections/vector.h>

namespace exo { struct ScopeStack; }
struct Font;

PACKED(struct Rect
{
    exo::float2 position;
    exo::float2 size;
})

PACKED(struct ColorRect
{
    Rect rect;
    u32 color;
    u32 i_clip_rect;
    u32 padding[2];
})

PACKED(struct TexturedRect
{
    Rect rect;
    Rect uv;
    u32 texture_descriptor;
    u32 i_clip_rect;
    u32 padding[2];
})

enum RectType
{
    RectType_Color = 0,
    RectType_Textured = 1,
    RectType_Clip = 2,
};

union PrimitiveIndex
{
    struct
    {
        u32 index : 24;
        u32 corner : 2;
        u32 type : 6;
    } bits;
    u32 raw;
};
static_assert(sizeof(PrimitiveIndex) == sizeof(u32));

struct FontGlyph
{
    Font *font;
    u32 glyph_index;

    bool operator==(const FontGlyph &other) const = default;
};

struct Painter
{
    u8 *vertices;
    PrimitiveIndex *indices;

    usize vertices_size;
    usize indices_size;
    usize vertex_bytes_offset;
    u32   index_offset;

    exo::Vec<u32> used_textures;
    exo::Vec<FontGlyph> glyphs_to_upload;
};

Painter *painter_allocate(exo::ScopeStack &scope, usize vertex_buffer_size, usize index_buffer_size);
void painter_draw_textured_rect(Painter &painter, const Rect &rect, u32 i_clip_rect, const Rect &uv, u32 texture);
void painter_draw_color_rect(Painter &painter, const Rect &rect, u32 i_clip_rect, u32 AABBGGRR);
exo::int2 measure_label(Font *font, const char *label);
void painter_draw_label(Painter &painter, const Rect &rect, u32 i_clip_rect, Font *font, const char *label);

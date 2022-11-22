#include "painter/painter.h"
#include "exo/collections/span.h"
#include "exo/macros/assert.h"
#include "exo/memory/scope_stack.h"
#include "exo/profile.h"
#include "painter/font.h"
#include "painter/glyph_cache.h"

#include <ft2build.h>
#include FT_FREETYPE_H
#include <cstring> // for std::memset
#include <hb.h>

Painter Painter::create(exo::Span<u8> vbuffer, exo::Span<PrimitiveIndex> ibuffer, int2 glyph_cache_size)
{
	Painter painter = {};
	painter.vertex_buffer = vbuffer;
	painter.index_buffer = ibuffer;
	painter.glyph_cache.allocator.size = glyph_cache_size;
	painter.glyph_cache.rasterizer = freetype_rasterizer;

	std::memset(painter.vertex_buffer.data(), 0, painter.vertex_buffer.size_bytes());
	std::memset(painter.index_buffer.data(), 0, painter.index_buffer.size_bytes());

	painter.shaper.hb_buf = hb_buffer_create();
	return painter;
}

void Painter::draw_textured_rect(const Rect &r, u32 i_clip_rect, const Rect &uv, u32 texture_id)
{
	EXO_PROFILE_SCOPE;
	ASSERT(texture_id != u32_invalid);

	auto misalignment = this->vertex_bytes_offset % sizeof(TexturedRect);
	if (misalignment != 0) {
		this->vertex_bytes_offset += sizeof(TexturedRect) - misalignment;
	}

	ASSERT(this->vertex_bytes_offset % sizeof(TexturedRect) == 0);
	const u32 i_rect = static_cast<u32>(this->vertex_bytes_offset / sizeof(TexturedRect));

	auto vertices = exo::reinterpret_span<TexturedRect>(this->vertex_buffer);
	vertices[i_rect] = {.rect = r, .uv = uv, .texture_descriptor = texture_id, .i_clip_rect = i_clip_rect};

	this->vertex_bytes_offset += sizeof(TexturedRect);

	// 0 - 3
	// |   |
	// 1 - 2
	this->index_buffer[this->index_offset++] = {{.index = i_rect, .corner = 0, .type = RectType_Textured}};
	this->index_buffer[this->index_offset++] = {{.index = i_rect, .corner = 1, .type = RectType_Textured}};
	this->index_buffer[this->index_offset++] = {{.index = i_rect, .corner = 2, .type = RectType_Textured}};
	this->index_buffer[this->index_offset++] = {{.index = i_rect, .corner = 2, .type = RectType_Textured}};
	this->index_buffer[this->index_offset++] = {{.index = i_rect, .corner = 3, .type = RectType_Textured}};
	this->index_buffer[this->index_offset++] = {{.index = i_rect, .corner = 0, .type = RectType_Textured}};
}

void Painter::draw_color_rect(const Rect &r, u32 i_clip_rect, ColorU32 color)
{
	EXO_PROFILE_SCOPE;

	// Don't draw invisible rects
	if (color.comps.a == 0) {
		return;
	}

	auto misalignment = this->vertex_bytes_offset % sizeof(ColorRect);
	if (misalignment != 0) {
		this->vertex_bytes_offset += sizeof(ColorRect) - misalignment;
	}

	const u32 i_rect = u32(this->vertex_bytes_offset / sizeof(ColorRect));
	auto vertices = exo::reinterpret_span<ColorRect>(this->vertex_buffer);
	vertices[i_rect] = {.rect = r, .color = color.raw, .i_clip_rect = i_clip_rect};
	this->vertex_bytes_offset += sizeof(ColorRect);

	// 0 - 3
	// |   |
	// 1 - 2
	this->index_buffer[this->index_offset++] = {{.index = i_rect, .corner = 0, .type = RectType_Color}};
	this->index_buffer[this->index_offset++] = {{.index = i_rect, .corner = 1, .type = RectType_Color}};
	this->index_buffer[this->index_offset++] = {{.index = i_rect, .corner = 2, .type = RectType_Color}};
	this->index_buffer[this->index_offset++] = {{.index = i_rect, .corner = 2, .type = RectType_Color}};
	this->index_buffer[this->index_offset++] = {{.index = i_rect, .corner = 3, .type = RectType_Color}};
	this->index_buffer[this->index_offset++] = {{.index = i_rect, .corner = 0, .type = RectType_Color}};
}

int2 Painter::measure_label(Font &font, exo::StringView label)
{
	EXO_PROFILE_SCOPE;

	auto *buf = this->shaper.hb_buf;
	hb_buffer_clear_contents(buf);
	hb_buffer_add_utf8(buf, label.data(), int(label.len()), 0, -1);
	hb_buffer_set_direction(buf, HB_DIRECTION_LTR);
	hb_buffer_set_script(buf, HB_SCRIPT_LATIN);
	hb_buffer_set_language(buf, hb_language_from_string("en", -1));

	hb_shape(font.hb_font, buf, nullptr, 0);

	u32 glyph_count;
	const i32 line_height = font.metrics.ascender - font.metrics.descender;
	// hb_glyph_info_t     *glyph_info  = hb_buffer_get_glyph_infos(buf, &glyph_count);
	hb_glyph_position_t *glyph_pos = hb_buffer_get_glyph_positions(buf, &glyph_count);

	i32 cursor_x = 0;
	for (u32 i = 0; i < glyph_count; i++) {
		cursor_x += (glyph_pos[i].x_advance >> 6);
	}

	return {cursor_x, line_height};
}

void Painter::draw_label(const Rect &view_rect, u32 i_clip_rect, Font &font, exo::StringView label)
{
	EXO_PROFILE_SCOPE;

	auto *buf = this->shaper.hb_buf;
	hb_buffer_clear_contents(buf);
	hb_buffer_add_utf8(buf, label.data(), int(label.len()), 0, -1);
	hb_buffer_set_direction(buf, HB_DIRECTION_LTR);
	hb_buffer_set_script(buf, HB_SCRIPT_LATIN);
	hb_buffer_set_language(buf, hb_language_from_string("en", -1));
	hb_shape(font.hb_font, buf, nullptr, 0);

	u32 glyph_count;
	const i32 line_height = font.metrics.height;
	hb_glyph_info_t *glyph_infos = hb_buffer_get_glyph_infos(buf, &glyph_count);
	hb_glyph_position_t *glyph_positions = hb_buffer_get_glyph_positions(buf, &glyph_count);

	i32 cursor_x = i32(view_rect.pos.x);
	i32 cursor_y = i32(view_rect.pos.y) + font.metrics.ascender;
	for (u32 i = 0; i < glyph_count; i++) {
		const u32 glyph_index = glyph_infos[i].codepoint;
		const i32 x_advance = glyph_positions[i].x_advance;
		const i32 y_advance = glyph_positions[i].y_advance;

		GlyphImage glyph_image = {};
		auto cache_entry = this->glyph_cache.queue_glyph(font, glyph_index, &glyph_image);
		if (cache_entry.has_value()) {
			const int2 glyph_pos = cache_entry.value();

			const Rect rect = {
				.pos = float2(int2{cursor_x + glyph_image.top_left.x, cursor_y - glyph_image.top_left.y}),
				.size = float2(glyph_image.image_size),
			};
			const Rect uv = {
				.pos = float2(glyph_pos) / float2(this->glyph_cache.allocator.size),
				.size = float2(glyph_image.image_size) / float2(this->glyph_cache.allocator.size),
			};

			this->draw_textured_rect(rect, i_clip_rect, uv, this->glyph_atlas_gpu_idx);
		}

		cursor_x += x_advance >> 6;
		cursor_y += y_advance >> 6;

		if (label[glyph_infos[i].cluster] == '\n') {
			cursor_x = i32(view_rect.pos.x);
			cursor_y += line_height;
		}
	}
}

void Painter::draw_color_round_rect(
	const Rect &r, u32 i_clip_rect, ColorU32 color, ColorU32 border_color, u32 border_thickness)
{
	EXO_PROFILE_SCOPE;

	// Don't draw invisible rects
	if (color.comps.a == 0 && border_color.comps.a == 0) {
		return;
	}

	auto misalignment = this->vertex_bytes_offset % sizeof(SdfRect);
	if (misalignment != 0) {
		this->vertex_bytes_offset += sizeof(SdfRect) - misalignment;
	}

	const u32 i_rect = u32(this->vertex_bytes_offset / sizeof(SdfRect));
	auto vertices = exo::reinterpret_span<SdfRect>(this->vertex_buffer);
	vertices[i_rect] = {
		.rect = r,
		.color = color.raw,
		.i_clip_rect = i_clip_rect,
		.border_color = border_color.raw,
		.border_thickness = border_thickness,
	};
	this->vertex_bytes_offset += sizeof(SdfRect);

	// 0 - 3
	// |   |
	// 1 - 2
	this->index_buffer[this->index_offset++] = {{.index = i_rect, .corner = 0, .type = RectType_Sdf_RoundRectangle}};
	this->index_buffer[this->index_offset++] = {{.index = i_rect, .corner = 1, .type = RectType_Sdf_RoundRectangle}};
	this->index_buffer[this->index_offset++] = {{.index = i_rect, .corner = 2, .type = RectType_Sdf_RoundRectangle}};
	this->index_buffer[this->index_offset++] = {{.index = i_rect, .corner = 2, .type = RectType_Sdf_RoundRectangle}};
	this->index_buffer[this->index_offset++] = {{.index = i_rect, .corner = 3, .type = RectType_Sdf_RoundRectangle}};
	this->index_buffer[this->index_offset++] = {{.index = i_rect, .corner = 0, .type = RectType_Sdf_RoundRectangle}};
}

void Painter::draw_color_circle(
	const Rect &r, u32 i_clip_rect, ColorU32 color, ColorU32 border_color, u32 border_thickness)
{
	EXO_PROFILE_SCOPE;

	// Don't draw invisible rects
	if (color.comps.a == 0 && border_color.comps.a == 0) {
		return;
	}

	auto misalignment = this->vertex_bytes_offset % sizeof(SdfRect);
	if (misalignment != 0) {
		this->vertex_bytes_offset += sizeof(SdfRect) - misalignment;
	}

	const u32 i_rect = u32(this->vertex_bytes_offset / sizeof(SdfRect));
	auto vertices = exo::reinterpret_span<SdfRect>(this->vertex_buffer.subspan(this->vertex_bytes_offset));
	vertices[i_rect] = {
		.rect = r,
		.color = color.raw,
		.i_clip_rect = i_clip_rect,
		.border_color = border_color.raw,
		.border_thickness = border_thickness,
	};
	this->vertex_bytes_offset += sizeof(SdfRect);

	// 0 - 3
	// |   |
	// 1 - 2
	this->index_buffer[this->index_offset++] = {{.index = i_rect, .corner = 0, .type = RectType_Sdf_Circle}};
	this->index_buffer[this->index_offset++] = {{.index = i_rect, .corner = 1, .type = RectType_Sdf_Circle}};
	this->index_buffer[this->index_offset++] = {{.index = i_rect, .corner = 2, .type = RectType_Sdf_Circle}};
	this->index_buffer[this->index_offset++] = {{.index = i_rect, .corner = 2, .type = RectType_Sdf_Circle}};
	this->index_buffer[this->index_offset++] = {{.index = i_rect, .corner = 3, .type = RectType_Sdf_Circle}};
	this->index_buffer[this->index_offset++] = {{.index = i_rect, .corner = 0, .type = RectType_Sdf_Circle}};
}

#include "renderer.h"

#include <exo/macros/packed.h>

#include <exo/profile.h>
#include <render/bindings.h>
#include <render/shader_watcher.h>
#include <render/vulkan/device.h>
#include <render/vulkan/image.h>

Renderer Renderer::create(u64 window_handle, AssetManager *asset_manager)
{
	Renderer renderer;
	renderer.base          = SimpleRenderer::create(window_handle);
	renderer.asset_manager = asset_manager;
	renderer.mesh_renderer = MeshRenderer::create(renderer.base.device);
	renderer.ui_renderer   = UiRenderer::create(renderer.base.device, int2(1024, 1024));
	WATCH_LIB_SHADER(renderer.base.shader_watcher);

	renderer.srgb_pass.program = renderer.base.device.create_program("srgb pass",
		vulkan::ComputeState{
			.shader = renderer.base.device.create_shader(SHADER_PATH("srgb_pass.comp.glsl.spv")),
		});

	return renderer;
}

static void register_srgb_pass(Renderer &renderer, Handle<TextureDesc> input, Handle<TextureDesc> output)
{
	auto &graph = renderer.base.render_graph;

	auto compute_program = renderer.srgb_pass.program;

	graph.raw_pass([compute_program, input, output](RenderGraph &graph, PassApi &api, vulkan::ComputeWork &cmd) {
		PACKED(struct Options {
			u32 linear_input_buffer_texture;
			u32 srgb_output_buffer_image;
			u32 pad00;
			u32 pad01;
		})

		auto input_image  = graph.resources.resolve_image(api.device, input);
		auto output_image = graph.resources.resolve_image(api.device, output);

		ASSERT(graph.image_size(input) == graph.image_size(output));
		exo::uint3 dispatch_size = exo::uint3(graph.image_size(input));
		dispatch_size.x          = (dispatch_size.x / 16) + (dispatch_size.x % 16 != 0);
		dispatch_size.y          = (dispatch_size.y / 16) + (dispatch_size.y % 16 != 0);

		auto *options = reinterpret_cast<Options *>(
			bindings::bind_shader_options(api.device, api.uniform_buffer, cmd, sizeof(Options)));
		options->linear_input_buffer_texture = api.device.get_image_sampled_index(input_image);
		options->srgb_output_buffer_image    = api.device.get_image_storage_index(output_image);

		cmd.barrier(input_image, vulkan::ImageUsage::ComputeShaderRead);
		cmd.barrier(output_image, vulkan::ImageUsage::ComputeShaderReadWrite);
		cmd.bind_pipeline(compute_program);
		cmd.dispatch(dispatch_size);
	});
}

void Renderer::draw(const RenderWorld &world, Painter *painter)
{
	EXO_PROFILE_SCOPE;
	base.start_frame();

	register_upload_nodes(this->base.render_graph,
		this->mesh_renderer,
		this->base.device,
		this->base.upload_buffer,
		this->asset_manager,
		world);

	auto intermediate_buffer = base.render_graph.output(TextureDesc{
		.name = "render buffer desc",
		.size = TextureSize::screen_relative(float2(1.0, 1.0)),
	});
	register_graphics_nodes(this->base.render_graph, this->mesh_renderer, intermediate_buffer);
	if (painter) {
		auto &pass = register_graph(this->base.render_graph, this->ui_renderer, painter, intermediate_buffer);
		pass.clear = false;
	}

	auto srgb_buffer = base.render_graph.output(TextureDesc{
		.name = "srgb buffer desc",
		.size = TextureSize::screen_relative(float2(1.0, 1.0)),
	});

	register_srgb_pass(*this, intermediate_buffer, srgb_buffer);

	base.render(srgb_buffer, 1.0);
}

u32 Renderer::glyph_atlas_index() const
{
	return this->base.device.get_image_sampled_index(this->ui_renderer.glyph_atlas);
}

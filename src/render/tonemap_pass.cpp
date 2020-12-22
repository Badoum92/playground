#include "render/hl_api.hpp"
#include "render/renderer.hpp"

namespace my_app
{

Renderer::TonemappingPass create_tonemapping_pass(vulkan::API &api)
{
    Renderer::TonemappingPass pass;

    vulkan::RasterizationState rasterization = {.culling = false};

    pass.program = api.create_program({
        .vertex_shader   = api.create_shader("shaders/fullscreen_triangle.vert.spv"),
        .fragment_shader = api.create_shader("shaders/hdr_compositing.frag.spv"),
        .rasterization = rasterization,
    });

    pass.params_pos = {};

    return pass;
}

void add_tonemapping_pass(Renderer &r)
{
    auto &api   = r.api;
    auto &graph = r.graph;

    // Make a shader debugging window and its own uniform buffer
    {
        r.tonemapping.params_pos = api.dynamic_uniform_buffer(sizeof(uint) + sizeof(float));
        auto *buffer             = reinterpret_cast<uint *>(r.tonemapping.params_pos.mapped);
        buffer[0]                = r.tonemap_debug.selected;
        auto *floatbuffer        = reinterpret_cast<float *>(buffer + 1);
        floatbuffer[0]           = r.tonemap_debug.exposure;
    }

    auto taa_output = r.history[(r.current_history)%2];

    graph.add_pass({
        .name              = "Tonemapping",
        .type              = PassType::Graphics,
        .sampled_images    = {taa_output, r.average_luminance},
        .color_attachments = {r.ldr_buffer},
        .exec =
            [pass_data       = r.tonemapping,
             default_sampler = r.nearest_sampler](RenderGraph &graph, RenderPass &self, vulkan::API &api) {
                auto hdr_buffer = graph.get_resolved_image(self.sampled_images[0]);
                auto average_luminance       = graph.get_resolved_image(self.sampled_images[1]);
                auto program    = pass_data.program;

                api.bind_combined_image_sampler(program,
                                                hdr_buffer,
                                                default_sampler,
                                                vulkan::SHADER_DESCRIPTOR_SET,
                                                0);

                api.bind_buffer(program, pass_data.params_pos, vulkan::SHADER_DESCRIPTOR_SET, 1);

                api.bind_combined_image_sampler(program,
                                                average_luminance,
                                                default_sampler,
                                                vulkan::SHADER_DESCRIPTOR_SET,
                                                2);
                api.bind_program(program);

                api.draw(3, 1, 0, 0);
            },
    });
}

} // namespace my_app

#include "renderer/renderer.hpp"
#include <imgui.h>
#include <iostream>
#include "window.hpp"
#include "tools.hpp"

namespace my_app
{

Renderer Renderer::create(const Window &window)
{
    Renderer r;
    r.p_window = &window;
    r.api = vulkan::API::create(window);

    /// --- Init ImGui
    {
        ImGui::CreateContext();
        auto &style             = ImGui::GetStyle();
        style.FrameRounding     = 0.f;
        style.GrabRounding      = 0.f;
        style.WindowRounding    = 0.f;
        style.ScrollbarRounding = 0.f;
        style.GrabRounding      = 0.f;
        style.TabRounding       = 0.f;

        ImGuiIO &io = ImGui::GetIO();

        io.DisplaySize.x             = float(r.api.ctx.swapchain.extent.width);
        io.DisplaySize.y             = float(r.api.ctx.swapchain.extent.height);
        io.DisplayFramebufferScale.x = window.get_dpi_scale().x;
        io.DisplayFramebufferScale.y = window.get_dpi_scale().y;
    }

    {
	vulkan::ProgramInfo pinfo{};
	pinfo.vertex_shader   = r.api.create_shader("shaders/gui.vert.spv");
	pinfo.fragment_shader = r.api.create_shader("shaders/gui.frag.spv");
        // clang-format off
	pinfo.push_constant({/*.stages = */ vk::ShaderStageFlagBits::eVertex, /*.offset = */ 0, /*.size = */ 4 * sizeof(float)});
	pinfo.binding({/* .set = */ vulkan::SHADER_DESCRIPTOR_SET, /*.slot = */ 0, /*.stages = */ vk::ShaderStageFlagBits::eFragment, /*.type = */ vk::DescriptorType::eCombinedImageSampler, /*.count = */ 1});
        // clang-format on
	pinfo.vertex_stride(sizeof(ImDrawVert));

	pinfo.vertex_info({vk::Format::eR32G32Sfloat, MEMBER_OFFSET(ImDrawVert, pos)});
	pinfo.vertex_info({vk::Format::eR32G32Sfloat, MEMBER_OFFSET(ImDrawVert, uv)});
	pinfo.vertex_info({vk::Format::eR8G8B8A8Unorm, MEMBER_OFFSET(ImDrawVert, col)});
	pinfo.enable_depth = false;

        r.gui_program = r.api.create_program(std::move(pinfo));
    }

    {
        vulkan::ImageInfo iinfo;
        iinfo.name = "ImGui texture";

        uchar *pixels = nullptr;

        // Get image data
        int w = 0;
        int h = 0;
        ImGui::GetIO().Fonts->GetTexDataAsRGBA32(&pixels, &w, &h);

        iinfo.width  = static_cast<u32>(w);
        iinfo.height = static_cast<u32>(h);
        iinfo.depth  = 1;

        r.gui_texture = r.api.create_image(iinfo);
        r.api.upload_image(r.gui_texture, pixels, iinfo.width * iinfo.height * 4);
    }

    {
        r.model = load_model("../models/Sponza/glTF/Sponza.gltf");
        r.load_model_data();
    }

    {
        vulkan::ImageInfo iinfo;
        iinfo.name    = "Depth texture";
	iinfo.format = vk::Format::eD32Sfloat;
        iinfo.width   = r.api.ctx.swapchain.extent.width;
        iinfo.height  = r.api.ctx.swapchain.extent.height;
        iinfo.depth   = 1;
	iinfo.usages = vk::ImageUsageFlagBits::eDepthStencilAttachment;
        auto depth_h = r.api.create_image(iinfo);

	vulkan::RTInfo dinfo;
	dinfo.is_swapchain = false;
	dinfo.image_h = depth_h;
	r.depth_rt              = r.api.create_rendertarget(dinfo);

	vulkan::RTInfo cinfo;
	cinfo.is_swapchain = true;
	r.color_rt              = r.api.create_rendertarget(cinfo);
    }

    {
        vulkan::ImageInfo iinfo;
        iinfo.name    = "Shadow Map Depth";
	iinfo.format  = vk::Format::eD32Sfloat;
        iinfo.width   = 2048;
        iinfo.height  = 2048;
        iinfo.depth   = 1;
	iinfo.usages  = vk::ImageUsageFlagBits::eDepthStencilAttachment;
        auto depth_h  = r.api.create_image(iinfo);

	vulkan::RTInfo dinfo;
	dinfo.is_swapchain = false;
	dinfo.image_h = depth_h;
	r.shadow_map_depth_rt    = r.api.create_rendertarget(dinfo);
    }

    return r;
}

void Renderer::destroy()
{
    destroy_model();

    {
    auto& depth = api.get_rendertarget(shadow_map_depth_rt);
    api.destroy_image(depth.image_h);
    }

    {
    auto& depth = api.get_rendertarget(depth_rt);
    api.destroy_image(depth.image_h);
    }

    api.destroy_image(gui_texture);
    api.destroy();
}

void Renderer::on_resize(int width, int height)
{
    api.on_resize(width, height);

    auto& depth = api.get_rendertarget(depth_rt);
    api.destroy_image(depth.image_h);

    vulkan::ImageInfo iinfo;
    iinfo.name   = "Depth texture";
    iinfo.format = vk::Format::eD32Sfloat;
    iinfo.width  = api.ctx.swapchain.extent.width;
    iinfo.height = api.ctx.swapchain.extent.height;
    iinfo.depth  = 1;
    iinfo.usages = vk::ImageUsageFlagBits::eDepthStencilAttachment;
    depth.image_h = api.create_image(iinfo);
}

void Renderer::wait_idle() { api.wait_idle(); }

void Renderer::imgui_draw()
{
    ImGui::Render();
    ImDrawData *data = ImGui::GetDrawData();
    if (data == nullptr || data->TotalVtxCount == 0) {
        return;
    }

    u32 vbuffer_len = sizeof(ImDrawVert) * static_cast<u32>(data->TotalVtxCount);
    u32 ibuffer_len = sizeof(ImDrawIdx) * static_cast<u32>(data->TotalIdxCount);

    auto v_pos = api.dynamic_vertex_buffer(vbuffer_len);
    auto i_pos = api.dynamic_index_buffer(ibuffer_len);

    auto *vertices = reinterpret_cast<ImDrawVert *>(v_pos.mapped);
    auto *indices  = reinterpret_cast<ImDrawIdx *>(i_pos.mapped);

    for (int i = 0; i < data->CmdListsCount; i++) {
        const ImDrawList *cmd_list = data->CmdLists[i];

        std::memcpy(vertices, cmd_list->VtxBuffer.Data, sizeof(ImDrawVert) * size_t(cmd_list->VtxBuffer.Size));
        std::memcpy(indices, cmd_list->IdxBuffer.Data, sizeof(ImDrawIdx) * size_t(cmd_list->IdxBuffer.Size));

        vertices += cmd_list->VtxBuffer.Size;
        indices += cmd_list->IdxBuffer.Size;
    }

    vk::Viewport viewport{};
    viewport.width    = data->DisplaySize.x * data->FramebufferScale.x;
    viewport.height   = data->DisplaySize.y * data->FramebufferScale.y;
    viewport.minDepth = 1.0f;
    viewport.maxDepth = 1.0f;
    api.set_viewport(viewport);

    api.bind_image(gui_program, vulkan::SHADER_DESCRIPTOR_SET, 0, gui_texture);
    api.bind_program(gui_program);
    api.bind_vertex_buffer(v_pos);
    api.bind_index_buffer(i_pos);

    float4 scale_and_translation;
    scale_and_translation[0] = 2.0f / data->DisplaySize.x; // X Scale
    scale_and_translation[1] = 2.0f / data->DisplaySize.y; // Y Scale
    scale_and_translation[2] = -1.0f - data->DisplayPos.x * scale_and_translation[0]; // X Translation
    scale_and_translation[3] = -1.0f - data->DisplayPos.y * scale_and_translation[1]; // Y Translation

    api.push_constant(vk::ShaderStageFlagBits::eVertex, 0, sizeof(float4), &scale_and_translation);

    // Will project scissor/clipping rectangles into framebuffer space
    ImVec2 clip_off   = data->DisplayPos;       // (0,0) unless using multi-viewports
    ImVec2 clip_scale = data->FramebufferScale; // (1,1) unless using retina display which are often (2,2)

    // Render GUI
    i32 vertex_offset = 0;
    u32 index_offset  = 0;
    for (int list = 0; list < data->CmdListsCount; list++) {
        const ImDrawList *cmd_list = data->CmdLists[list];

        for (int command_index = 0; command_index < cmd_list->CmdBuffer.Size; command_index++) {
            const ImDrawCmd *draw_command = &cmd_list->CmdBuffer[command_index];

            // Project scissor/clipping rectangles into framebuffer space
            ImVec4 clip_rect;
            clip_rect.x = (draw_command->ClipRect.x - clip_off.x) * clip_scale.x;
            clip_rect.y = (draw_command->ClipRect.y - clip_off.y) * clip_scale.y;
            clip_rect.z = (draw_command->ClipRect.z - clip_off.x) * clip_scale.x;
            clip_rect.w = (draw_command->ClipRect.w - clip_off.y) * clip_scale.y;

            // Apply scissor/clipping rectangle
            // FIXME: We could clamp width/height based on clamped min/max values.
            vk::Rect2D scissor;
            scissor.offset.x = (static_cast<i32>(clip_rect.x) > 0) ? static_cast<i32>(clip_rect.x) : 0;
            scissor.offset.y = (static_cast<i32>(clip_rect.y) > 0) ? static_cast<i32>(clip_rect.y) : 0;
            scissor.extent.width = static_cast<u32>(clip_rect.z - clip_rect.x);
            scissor.extent.height = static_cast<u32>(clip_rect.w - clip_rect.y + 1); // FIXME: Why +1 here?

            api.set_scissor(scissor);

            api.draw_indexed(draw_command->ElemCount, 1, index_offset, vertex_offset, 0);

            index_offset += draw_command->ElemCount;
        }
        vertex_offset += cmd_list->VtxBuffer.Size;
    }
}

static void bind_texture(Renderer& r, uint slot, std::optional<u32> i_texture)
{
    if (i_texture) {
        auto texture = r.model.textures[*i_texture];
        auto image   = r.model.images[texture.image];
        auto sampler = r.model.samplers[texture.sampler];

        r.api.bind_combined_image_sampler(r.model.program, vulkan::DRAW_DESCRIPTOR_SET, slot, image.image_h, sampler.sampler_h);
    }
    else {
        // bind empty texture
    }
}

static void draw_node(Renderer& r, Node& node)
{
    if (node.dirty) {
        node.dirty = false;
        auto translation = glm::translate(glm::mat4(1.0f), node.translation);
        auto rotation = glm::mat4(node.rotation);
        auto scale = glm::scale(glm::mat4(1.0f), node.scale);
        node.cached_transform =  translation * rotation * scale;
    }

    auto u_pos = r.api.dynamic_uniform_buffer(sizeof(float4x4));
    auto* buffer = reinterpret_cast<float4x4*>(u_pos.mapped);
    *buffer = node.cached_transform;
    r.api.bind_buffer(r.model.program, vulkan::DRAW_DESCRIPTOR_SET, 0, u_pos);

    const auto& mesh = r.model.meshes[node.mesh];
    for (const auto& primitive : mesh.primitives) {
        // if program != last program then bind program

        const auto& material = r.model.materials[primitive.material];

        MaterialPushConstant material_pc = MaterialPushConstant::from(material);
        r.api.push_constant(vk::ShaderStageFlagBits::eFragment, 0, sizeof(material_pc), &material_pc);

        bind_texture(r, 1, material.base_color_texture);
        bind_texture(r, 2, material.normal_texture);
        bind_texture(r, 3, material.metallic_roughness_texture);

        r.api.draw_indexed(primitive.index_count, 1, primitive.first_index, static_cast<i32>(primitive.first_vertex), 0);
    }

    // TODO: transform relative to parent
    for (auto child_i : node.children) {
        draw_node(r, r.model.nodes[child_i]);
    }
}

void Renderer::draw_model()
{
    float3 cam_up = float3(0, 1, 0);

    ImGui::Begin("Camera");
    static float s_camera_pos[3] = {2.0f, 0.5f, 0.5f};
    ImGui::SliderFloat3("Camera position", s_camera_pos, -25.0f, 25.0f);
    static float s_target_pos[3] = {1.0f, 0.0f, 0.0f};
    ImGui::SliderFloat3("Camera target", s_target_pos, -1.0f, 1.0f);

    float3 target_pos;
    target_pos.x = s_target_pos[0];
    target_pos.y = s_target_pos[1];
    target_pos.z = s_target_pos[2];

    float3 camera_pos;
    camera_pos.x = s_camera_pos[0];
    camera_pos.y = s_camera_pos[1];
    camera_pos.z = s_camera_pos[2];

    float aspect_ratio = api.ctx.swapchain.extent.width / float(api.ctx.swapchain.extent.height);

    static float fov          = 45.0f;
    ImGui::SliderFloat("FOV", &fov, 30.f, 90.f);

    float4x4 proj      = glm::perspective(glm::radians(fov), aspect_ratio, 0.1f, 5000.0f);

    ImGui::SetCursorPosX(10.0f);
    ImGui::Text("Target position:");
    ImGui::SetCursorPosX(20.0f);
    ImGui::Text("X: %.1f", double(target_pos.x));
    ImGui::SetCursorPosX(20.0f);
    ImGui::Text("Y: %.1f", double(target_pos.y));
    ImGui::SetCursorPosX(20.0f);
    ImGui::Text("Z: %.1f", double(target_pos.z));


    ImGui::End();
    // clang-format off
    float4x4 view = glm::lookAt(
	camera_pos,       // origin of camera
	camera_pos + target_pos,    // where to look
	cam_up);

    // Vulkan clip space has inverted Y and half Z.
    float4x4 clip = glm::mat4(1.0f, 0.0f, 0.0f, 0.0f,
			 0.0f, -1.0f, 0.0f, 0.0f,
			 0.0f, 0.0f, 0.5f, 0.0f,
			 0.0f, 0.0f, 0.5f, 1.0f);

    // clang-format on

    vk::Viewport viewport{};
    viewport.width    = api.ctx.swapchain.extent.width;
    viewport.height   = api.ctx.swapchain.extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    api.set_viewport(viewport);

    vk::Rect2D scissor{};
    scissor.extent = api.ctx.swapchain.extent;
    api.set_scissor(scissor);

    // last program
    // bind program

    {
        auto u_pos = api.dynamic_uniform_buffer(3 * sizeof(float4x4));
        auto* buffer = reinterpret_cast<float4x4*>(u_pos.mapped);
        buffer[0] = view;
        buffer[1] = proj;
        buffer[2] = clip;
        api.bind_buffer(model.program, vulkan::SHADER_DESCRIPTOR_SET, 0, u_pos);
    }

    {
        ImGui::Begin("glTF Shader");
        static const char* options[] = {
            "Nothing",
            "BaseColor",
            "Normal",
            "MetallicRoughness"
            };
        static usize selected = 0;
        tools::imgui_select("Debug output", options, ARRAY_SIZE(options), selected);
        ImGui::End();

        auto u_pos = api.dynamic_uniform_buffer(sizeof(uint));
        auto* buffer = reinterpret_cast<uint*>(u_pos.mapped);
        *buffer = static_cast<uint>(selected);
        api.bind_buffer(model.program, vulkan::SHADER_DESCRIPTOR_SET, 1, u_pos);
    }



    api.bind_program(model.program);
    api.bind_index_buffer(model.index_buffer);
    api.bind_vertex_buffer(model.vertex_buffer);

    for (usize node_i : model.scene) {
        draw_node(*this, model.nodes[node_i]);
    }
}

static void shadow_map(Renderer& r)
{
    vulkan::PassInfo pass;
    pass.present       = false;

    vulkan::AttachmentInfo depth_info;
    depth_info.load_op = vk::AttachmentLoadOp::eClear;
    depth_info.rt      = r.shadow_map_depth_rt;
    pass.depth = std::make_optional(depth_info);

    r.api.begin_pass(std::move(pass));



    r.api.end_pass();
}

void Renderer::draw()
{
    bool is_ok = api.start_frame();
    if (!is_ok) {
	ImGui::EndFrame();
        return;
    }

    shadow_map(*this);

    vulkan::PassInfo pass;
    pass.present       = true;

    vulkan::AttachmentInfo color_info;
    color_info.load_op = vk::AttachmentLoadOp::eClear;
    color_info.rt      = color_rt;
    pass.color = std::make_optional(color_info);

    vulkan::AttachmentInfo depth_info;
    depth_info.load_op = vk::AttachmentLoadOp::eClear;
    depth_info.rt      = depth_rt;
    pass.depth = std::make_optional(depth_info);

    api.begin_pass(std::move(pass));

    draw_model();
    imgui_draw();

    api.end_pass();
    api.end_frame();
}

} // namespace my_app

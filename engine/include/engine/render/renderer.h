#pragma once

#include <exo/collections/handle.h>
#include <exo/collections/pool.h>
#include <exo/os/uuid.h>

#include "engine/render/imgui_pass.h"
#include "engine/render/ring_buffer.h"
#include "engine/render/bvh.h"
#include "engine/render/unified_buffer_storage.h"

#include "engine/render/vulkan/commands.h"
#include "engine/render/vulkan/context.h"
#include "engine/render/vulkan/surface.h"

namespace exo { struct ScopeStack; }
namespace gfx = vulkan;
struct Mesh;
struct Material;
struct Scene;
struct AssetManager;
struct RenderWorld;
struct BaseRenderer;

// -- Assets begin

// Descriptors for a mesh
PACKED (struct RenderMeshGPU
{
    u32 first_position;
    u32 first_index;
    u32 first_submesh;
    u32 bvh_root;
    u32 first_uv;
    u32 pad00;
    u32 pad01;
    u32 pad10;
})

PACKED(struct RenderSubMesh
{
    u32 first_index  = 0;
    u32 first_vertex = 0;
    u32 index_count  = 0;
    u32 i_material   = 0;
})

PACKED(struct RenderMaterialGPU
{
   float4 base_color_factor          = float4(1.0);
   float4 emissive_factor            = float4(0.0);
   float  metallic_factor            = 0.0f;
   float  roughness_factor           = 0.0f;
   u32    base_color_texture         = u32_invalid;
   u32    normal_texture             = u32_invalid;
   u32    metallic_roughness_texture = u32_invalid;
   float  rotation                   = 0.0f;
   float2 offset                     = float2(0.0f);
   float2 scale                      = float2(1.0f);
   float2 pad00                      = 42.0f;
})

struct RenderMesh
{
    BVHNode bvh_root = {};
    Vec<u32> instances;
    Vec<u32> materials;
    u32 first_instance = 0;
    Vec<RenderSubMesh> submeshes;
};

struct RenderTexture
{
    Handle<gfx::Image> gpu;
};

struct RenderMaterial
{
    Handle<RenderTexture> base_color_texture;
};

// -- Assets end

struct Settings
{
    float resolution_scale      = {1.0f};
    int2  render_resolution     = {0};
    bool  resolution_dirty      = {true};
    bool  enable_taa            = {true};
    bool  enable_path_tracing   = {false};
    bool  freeze_camera_culling = {false};
    bool  use_blue_noise        = {true};
    bool enable_hbao = {false};
    u32 hbao_samples_per_dir = 4;
    u32 hbao_dir_count = 4;
    u32 hbao_radius = 32;
};

PACKED (struct GlobalUniform
{
    float4x4 camera_view;
    float4x4 camera_projection;
    float4x4 camera_view_inverse;
    float4x4 camera_projection_inverse;
    float4x4 camera_previous_view;
    float4x4 camera_previous_projection;

    float2 render_resolution;
    float2 jitter_offset;

    float delta_t;
    u32 frame_count;
    u32 enable_taa;
    u32 meshes_data_descriptor;

    u32 instances_data_descriptor;
    u32 instances_offset;
    u32 submesh_instances_data_descriptor;
    u32 submesh_instances_offset;

    u32 tlas_descriptor;
    u32 submesh_instances_count;
    u32 index_buffer_descriptor;
    u32 vertex_positions_descriptor;

    u32 bvh_nodes_descriptor;
    u32 submeshes_descriptor;
    u32 culled_instances_indices_descriptor;
    u32 materials_descriptor;

    u32 vertex_uvs_descriptor;
    u32 enable_hbao;
    u32 hbao_samples_per_dir;
    u32 hbao_dir_count;

    u32 hbao_radius;
    u32 padding0;
    u32 padding1;
    u32 padding2;
})

PACKED (struct PushConstants
{
    u32 draw_id        = u32_invalid;
    u32 gui_texture_id = u32_invalid;
})

// One object in the world
PACKED (struct RenderInstance
{
    float4x4 object_to_world;
    float4x4 world_to_object;
    u32 i_render_mesh;
    u32 pad00;
    u32 pad01;
    u32 pad10;
})

// One drawcall, one material instance
PACKED (struct SubMeshInstance
{
    u32 i_mesh;
    u32 i_submesh;
    u32 i_instance;
    u32 i_draw;
})

struct Renderer
{
    static Renderer *create(exo::ScopeStack &scope, exo::Window *window, AssetManager *_asset_manager);
    ~Renderer();

    void display_ui();
    void update(const RenderWorld &render_world);
    void prepare_geometry(const RenderWorld &render_world);
    void compact_buffer(gfx::ComputeWork &cmd, i32 count, Handle<gfx::ComputeProgram> copy_program, const void *options_data, usize options_len);

    // base_renderer
    void reload_shader(std::string_view shader_name);
    void on_resize();
    bool start_frame();
    bool end_frame(gfx::ComputeWork &cmd);

    /// ---
    BaseRenderer *base_renderer;

    AssetManager *asset_manager;
    Settings settings;

    Handle<gfx::Image> visibility_buffer;
    Handle<gfx::Image> depth_buffer;
    Handle<gfx::Image> hdr_buffer;
    Handle<gfx::Image> ldr_buffer;
    Handle<gfx::Image> history_buffers[2];
    Handle<gfx::Framebuffer> visibility_depth_fb;
    Handle<gfx::Framebuffer> hdr_depth_fb;
    Handle<gfx::Framebuffer> ldr_fb;
    Handle<gfx::Framebuffer> ldr_depth_fb;

    // Geometry
    exo::Map<exo::UUID, Handle<RenderMesh>> uploaded_meshes; // Contains all uploaded meshes in the Pool
    exo::Pool<RenderMesh> render_meshes; // Used to free/allocate mesh and get a free slot to upload it to GPU buffer
    exo::Map<exo::UUID, Vec<u32>> mesh_instances; // all mesh instances
    Handle<gfx::Buffer>        meshes_buffer;

    Handle<gfx::Buffer>  tlas_buffer;
    UnifiedBufferStorage index_buffer;
    UnifiedBufferStorage vertex_positions_buffer;
    UnifiedBufferStorage vertex_uvs_buffer;
    UnifiedBufferStorage bvh_nodes_buffer;
    UnifiedBufferStorage submeshes_buffer;

    // Textures
    exo::Map<exo::UUID, Handle<RenderTexture>> uploaded_textures;
    exo::Pool<RenderTexture>                     render_textures;

    // Materials
    exo::Map<exo::UUID, Handle<RenderMaterial>> uploaded_materials;
    exo::Pool<RenderMaterial>                     render_materials;
    Handle<gfx::Buffer>                      materials_buffer;

    // Scan
    Handle<gfx::Buffer> predicate_buffer;        // hold predicate for each element, (for example in instance culling, 0: not visible, 1: visible)
    Handle<gfx::Buffer> group_sum_reduction;     // count of element that match predicate for each work group
    Handle<gfx::Buffer> scanned_indices;         // scan of culled indices in predicate buffer (0112233 for example if predicate is true every other 2 elements)

    // Draw list data
    Vec<RenderInstance> render_instances;                     // all drawable instances from the scene
    Vec<SubMeshInstance> submesh_instances;                   // instance x submeshes
    RingBuffer submesh_instances_data;                        // for every instance x submesh, corresponding SubMeshInstance
    RingBuffer instances_data;                                // for every instance, corresponding RenderInstance
    Handle<gfx::Buffer> culled_instances_compact_indices;     // above indices but compacted and ready to draw (0123 for above example)
    Handle<gfx::Buffer> draw_arguments;                       // gpu draws, one draw per mesh, instance index refers to compact indices
    Handle<gfx::Buffer> culled_draw_arguments;                // gpu draws without empty vertex_count/instances_count

    // global uniform data
    u32 instances_offset = 0;
    u32 submesh_instances_offset = 0;

    ImGuiPass imgui_pass;

    // Main render passes
    Handle<gfx::GraphicsProgram> opaque_program;
    Handle<gfx::ComputeProgram> tonemap_program;
    Handle<gfx::ComputeProgram> taa_program;
    float2 halton_sequence[16];
    Handle<gfx::ComputeProgram> path_tracer_program;
    Handle<gfx::ComputeProgram> visibility_shading_program;

    // Indirect draw/GPU culling
    u32 draw_count;
    Handle<gfx::ComputeProgram> init_draw_calls_program;
    Handle<gfx::ComputeProgram> instances_culling_program;
    Handle<gfx::ComputeProgram> parallel_prefix_sum_program;
    Handle<gfx::ComputeProgram> copy_culled_instances_index_program;
    Handle<gfx::ComputeProgram> drawcalls_fill_predicate_program;
    Handle<gfx::ComputeProgram> copy_draw_calls_program;

    // Misc Textures
    Handle<gfx::Image> blue_noise;
};

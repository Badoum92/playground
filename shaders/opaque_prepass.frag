#include "types.h"
#include "globals.h"
#include "constants.h"

layout(set = 1, binding = 2) buffer RenderMeshesBuffer {
    RenderMeshData render_meshes[];
};

layout(set = 1, binding = 3) buffer MaterialBuffer {
    Material materials[];
};

layout(location = 0) in float4 i_color;
layout(location = 1) in float2 i_uv;
void main()
{
    RenderMeshData render_mesh = render_meshes[push_constants.render_mesh_idx];
    Material material = materials[render_mesh.i_material];

    float4 base_color = i_color * material.base_color_factor;
    if (material.base_color_texture != u32_invalid)
    {
        base_color *= texture(global_textures[nonuniformEXT(globals.render_texture_offset + material.base_color_texture)], i_uv);
    }

    if (base_color.a < 0.5)
    {
        discard;
    }
}
#include "glb.h"

#include <exo/algorithms.h>
#include <exo/collections/vector.h>
#include <exo/numerics.h>
#include <exo/types.h>
#include <exo/logger.h>

#include <rapidjson/document.h>
#include <rapidjson/error/en.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <meshoptimizer.h>

#include <ktx.h>

namespace gltf
{
enum struct ComponentType : i32
{
    Byte = 5120,
    UnsignedByte = 5121,
    Short = 5122,
    UnsignedShort = 5123,
    UnsignedInt = 5125,
    Float = 5126,
    Invalid
};

inline u32 size_of(ComponentType type)
{
    switch (type)
    {
    case ComponentType::Byte:
    case ComponentType::UnsignedByte:
        return 1;

    case ComponentType::Short:
    case ComponentType::UnsignedShort:
        return 2;

    case ComponentType::UnsignedInt:
    case ComponentType::Float:
        return 4;

    default:
        assert(false);
        return 4;
    }
}
}

namespace glb
{

enum struct ChunkType : u32
{
    Json   = 0x4E4F534A,
    Binary = 0x004E4942,
    Invalid,
};

struct Chunk
{
    u32 length;
    ChunkType type;
    u8  data[0];
};
static_assert(sizeof(Chunk) == 2 * sizeof(u32));

struct Header
{
    u32 magic;
    u32 version;
    u32 length;
    Chunk first_chunk;
};

struct Accessor
{
    gltf::ComponentType component_type = gltf::ComponentType::Invalid;
    int count = 0;
    int nb_component = 0;
    int bufferview_index = 0;
    int byte_offset = 0;
    float min_float;
    float max_float;
};

struct BufferView
{
    int byte_offset = 0;
    int byte_length = 0;
    int byte_stride = -1;
};

static Accessor get_accessor(const rapidjson::Value &object)
{
    const auto &accessor = object.GetObject();

    Accessor res = {};

    // technically not required but it doesn't make sense not to have one
    res.bufferview_index = accessor["bufferView"].GetInt();
    res.byte_offset = 0;

    if (accessor.HasMember("byteOffset")) {
        res.byte_offset = accessor["byteOffset"].GetInt();
    }

    res.component_type = gltf::ComponentType(accessor["componentType"].GetInt());

    res.count = accessor["count"].GetInt();

    auto type = std::string_view(accessor["type"].GetString());
    if (type == "SCALAR") {
        res.nb_component = 1;
    }
    else if (type == "VEC2") {
        res.nb_component = 2;
    }
    else if (type == "VEC3") {
        res.nb_component = 3;
    }
    else if (type == "VEC4") {
        res.nb_component = 4;
    }
    else if (type == "MAT2") {
        res.nb_component = 4;
    }
    else if (type == "MAT3") {
        res.nb_component = 9;
    }
    else if (type == "MAT4") {
        res.nb_component = 16;
    }
    else {
        assert(false);
    }

    return res;
}

static BufferView get_bufferview(const rapidjson::Value &object)
{
    const auto &bufferview = object.GetObject();
    BufferView res = {};
    if (bufferview.HasMember("byteOffset")) {
        res.byte_offset = bufferview["byteOffset"].GetInt();
    }
    res.byte_length = bufferview["byteLength"].GetInt();
    if (bufferview.HasMember("byteStride")) {
        res.byte_stride = bufferview["byteStride"].GetInt();
    }
    return res;
}

static void process_json(Scene &new_scene, rapidjson::Document &document, const Chunk *binary_chunk)
{
    const auto &accessors = document["accessors"].GetArray();
    const auto &bufferviews = document["bufferViews"].GetArray();

    u32 index_acc = 0;

    Vec<u32> mesh_remap;
    if (document.HasMember("meshes"))
    {
        mesh_remap.resize(document["meshes"].GetArray().Size());
        u32 i_current_mesh = 0;

        for (auto &mesh : document["meshes"].GetArray())
        {
            Mesh new_mesh = {};

            for (auto &primitive : mesh["primitives"].GetArray())
            {
                assert(primitive.HasMember("attributes"));
                const auto &attributes = primitive["attributes"].GetObject();
                assert(attributes.HasMember("POSITION"));

                new_mesh.submeshes.emplace_back();
                auto &new_submesh = new_mesh.submeshes.back();

                new_submesh.index_count  = 0;
                new_submesh.first_vertex = static_cast<u32>(new_mesh.positions.size());
                new_submesh.first_index  = static_cast<u32>(new_mesh.indices.size());

                if (primitive.HasMember("material"))
                {
                    new_submesh.i_material   = primitive["material"].GetUint() + 1; // #0 is the default one for primitives without materials
                }

                // -- Attributes
                assert(primitive.HasMember("indices"));
                {
                    auto accessor_json = accessors[primitive["indices"].GetInt()].GetObject();
                    auto accessor = get_accessor(accessor_json);
                    auto bufferview = get_bufferview(bufferviews[accessor.bufferview_index]);
                    usize byte_stride = bufferview.byte_stride > 0 ? bufferview.byte_stride : gltf::size_of(accessor.component_type) * accessor.nb_component;

                    // Copy the data from the binary buffer
                    new_mesh.indices.reserve(accessor.count);
                    for (usize i_index = 0; i_index < usize(accessor.count); i_index += 1)
                    {
                        u32 index;
                        usize offset = bufferview.byte_offset + accessor.byte_offset + i_index * byte_stride;
                        if (accessor.component_type == gltf::ComponentType::UnsignedShort) {
                            index = new_submesh.first_vertex + *reinterpret_cast<const u16*>(ptr_offset(binary_chunk->data, offset));
                        }
                        else if (accessor.component_type == gltf::ComponentType::UnsignedInt) {
                            index = new_submesh.first_vertex + *reinterpret_cast<const u32*>(ptr_offset(binary_chunk->data, offset));
                        }
                        else {
                            assert(false);
                        }
                        new_mesh.indices.push_back(index);
                    }

                    new_submesh.index_count = accessor.count;

                    index_acc += accessor.count;
                }

                u32 vertex_count = 0;
                {
                    auto accessor_json = accessors[attributes["POSITION"].GetInt()].GetObject();
                    auto accessor = get_accessor(accessor_json);
                    vertex_count = accessor.count;
                    auto bufferview = get_bufferview(bufferviews[accessor.bufferview_index]);
                    usize byte_stride = bufferview.byte_stride > 0 ? bufferview.byte_stride : gltf::size_of(accessor.component_type) * accessor.nb_component;

                    // Copy the data from the binary buffer
                    new_mesh.positions.reserve(accessor.count);
                    for (usize i_position = 0; i_position < usize(accessor.count); i_position += 1)
                    {
                        usize offset = bufferview.byte_offset + accessor.byte_offset + i_position * byte_stride;
                        float4 new_position = {};

                        if (accessor.component_type == gltf::ComponentType::UnsignedShort) {
                            const auto *components = reinterpret_cast<const u16*>(ptr_offset(binary_chunk->data, offset));
                            new_position = {float(components[0]), float(components[1]), float(components[2]), 1.0f};
                        }
                        else if (accessor.component_type == gltf::ComponentType::Float) {
                            const auto *components = reinterpret_cast<const float*>(ptr_offset(binary_chunk->data, offset));
                            new_position = {float(components[0]), float(components[1]), float(components[2]), 1.0f};
                        }
                        else {
                            assert(false);
                        }

                        new_mesh.positions.push_back(new_position);
                    }
                }

                if (attributes.HasMember("TEXCOORD_0"))
                {
                    auto accessor_json = accessors[attributes["TEXCOORD_0"].GetInt()].GetObject();
                    auto accessor = get_accessor(accessor_json);
                    assert(accessor.count == vertex_count);
                    auto bufferview = get_bufferview(bufferviews[accessor.bufferview_index]);
                    usize byte_stride = bufferview.byte_stride > 0 ? bufferview.byte_stride : gltf::size_of(accessor.component_type) * accessor.nb_component;

                    // Copy the data from the binary buffer
                    new_mesh.uvs.reserve(accessor.count);
                    for (usize i_uv = 0; i_uv < usize(accessor.count); i_uv += 1)
                    {
                        usize offset = bufferview.byte_offset + accessor.byte_offset + i_uv * byte_stride;
                        float2 new_uv = {};

                        if (accessor.component_type == gltf::ComponentType::UnsignedShort) {
                            const auto *components = reinterpret_cast<const u16*>(ptr_offset(binary_chunk->data, offset));
                            new_uv = {float(components[0]), float(components[1])};
                        }
                        else if (accessor.component_type == gltf::ComponentType::Float) {
                            const auto *components = reinterpret_cast<const float*>(ptr_offset(binary_chunk->data, offset));
                            new_uv = {float(components[0]), float(components[1])};
                        }
                        else {
                            assert(false);
                        }

                        new_mesh.uvs.push_back(new_uv);
                    }
                }
                else
                {
                    new_mesh.uvs.reserve(vertex_count);
                    for (u32 i = 0; i < vertex_count; i += 1)
                    {
                        new_mesh.uvs.push_back(float2(0.0f, 0.0f));
                    }
                }

#if defined(BUILD_MESHLETS)
                const size_t max_vertices  = 64;
                const size_t max_triangles = 124;  // NVidia-recommended 126, rounded down to a multiple of 4
                const float cone_weight    = 0.5f; // note: should be set to 0 unless cone culling is used at runtime!

                size_t max_meshlets = meshopt_buildMeshletsBound(new_submesh.index_count, max_vertices, max_triangles);
                std::vector<meshopt_Meshlet> meshlets(max_meshlets);
                std::vector<unsigned int> meshlet_vertices(max_meshlets * max_vertices);
                std::vector<unsigned char> meshlet_triangles(max_meshlets * max_triangles * 3);

                meshlets.resize(meshopt_buildMeshlets(&meshlets[0],
                                                      &meshlet_vertices[0],
                                                      &meshlet_triangles[0],
                                                      &new_mesh.indices[new_submesh.first_index],
                                                      new_submesh.index_count,
                                                      &new_mesh.positions[new_submesh.first_vertex].x,
                                                      new_submesh.vertex_count,
                                                      sizeof(float3),
                                                      max_vertices,
                                                      max_triangles,
                                                      cone_weight));

                if (meshlets.size())
                {
                    const meshopt_Meshlet &last = meshlets.back();

                    // this is an example of how to trim the vertex/triangle arrays when copying data out to GPU storage
                    meshlet_vertices.resize(last.vertex_offset + last.vertex_count);
                    meshlet_triangles.resize(last.triangle_offset + ((last.triangle_count * 3 + 3) & ~3));

                }

                double avg_vertices  = 0;
                double avg_triangles = 0;
                size_t not_full      = 0;

                for (size_t i = 0; i < meshlets.size(); ++i)
                {
                    const meshopt_Meshlet &m = meshlets[i];

                    avg_vertices += m.vertex_count;
                    avg_triangles += m.triangle_count;
                    not_full += m.vertex_count < max_vertices;
                }

                avg_vertices /= double(meshlets.size());
                avg_triangles /= double(meshlets.size());

                logger::info("\t{} meshlets (avg vertices {}, avg triangles {}, not full {})\n",
                             int(meshlets.size()),
                             avg_vertices,
                             avg_triangles,
                             int(not_full));
#endif
            }

            u32 i_similar_mesh = u32_invalid;
            for (u32 i_mesh = 0; i_mesh < new_scene.meshes.size(); i_mesh += 1)
            {
                const auto &mesh = new_scene.meshes[i_mesh];
                if (mesh.is_similar(new_mesh))
                {
                    i_similar_mesh = i_mesh;
                    break;
                }
            }

            if (i_similar_mesh == u32_invalid)
            {
                mesh_remap[i_current_mesh] = new_scene.meshes.size();
                new_scene.meshes.push_back(std::move(new_mesh));
            }
            else
            {
                mesh_remap[i_current_mesh] = i_similar_mesh;
            }

            i_current_mesh += 1;
        }


        logger::info("Loaded {} unique meshes from {} meshes in file.\n", new_scene.meshes.size(), mesh_remap.size());
    }

    if (document.HasMember("images"))
    {
        u32 i_image = 0;
        for (auto &image : document["images"].GetArray())
        {
            ktxTexture2* ktx_texture = nullptr;
            if (image.HasMember("mimeType"))
            {
                const char *mime_type = image["mimeType"].GetString();
                int bufferview_index = image["bufferView"].GetInt();

                logger::info("[GLB] image #{} has mimeType {}\n", i_image, mime_type);

                if (std::string_view(mime_type) == "image/ktx2")
                {
                    auto bufferview = get_bufferview(bufferviews[bufferview_index]);

                    KTX_error_code result = KTX_SUCCESS;

                    const u8 *image_data = reinterpret_cast<const u8 *>(ptr_offset(binary_chunk->data, bufferview.byte_offset));
                    usize size           = bufferview.byte_length;
                    result               = ktxTexture2_CreateFromMemory(image_data, size, KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &ktx_texture);

                    if (result == KTX_SUCCESS)
                    {
                        logger::info("baseWidth: {} | baseHeight: {} | numDimensions: {} | numLevels: {} | numLayers: {}\n", ktx_texture->baseWidth, ktx_texture->baseHeight, ktx_texture->numDimensions, ktx_texture->numLevels, ktx_texture->numLayers);

                        if (ktxTexture2_NeedsTranscoding(ktx_texture))
                        {
                            // https://github.com/KhronosGroup/3D-Formats-Guidelines/blob/main/KTXDeveloperGuide.md
                            // Assume gpu that supports BC only
                            // ETC1S / BasisLZ Codec
                            //   RGB/A     : KTX_TTF_BC7_RGBA
                            //   R         : KTX_TTF_BC4_R
                            //   RG        : KTX_TTF_BC5_RG
                            // UASTC Codec :KTX_TTF_BC7_RGBA

                            ktx_transcode_fmt_e target_format = KTX_TTF_BC7_RGBA;

                            u32 nb_components = ktxTexture2_GetNumComponents(ktx_texture);
                            if (ktx_texture->supercompressionScheme == KTX_SS_BASIS_LZ)
                            {
                                if (nb_components == 1)
                                {
                                    target_format = KTX_TTF_BC4_R;
                                }
                                else if (nb_components == 2)
                                {
                                    target_format = KTX_TTF_BC5_RG;
                                }
                            }

                            logger::info("transcoding texture to BC7 RGBA\n");
                            result = ktxTexture2_TranscodeBasis(ktx_texture, target_format, 0);
                            if (result != KTX_SUCCESS)
                            {
                                logger::error("Could not transcode the input texture to the selected target format.\n");
                                ktxTexture_Destroy(reinterpret_cast<ktxTexture*>(ktx_texture));
                            }
                        }
                    }
                    else
                    {
                        logger::error("invalid ktx file\n");
                    }
                }
            }
            new_scene.images.push_back(ktx_texture);
            i_image += 1;
        }
    }

    new_scene.materials.emplace_back();
    if (document.HasMember("materials"))
    {
        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        document["materials"].Accept(writer);
        logger::info("[GLB] materials: {}\n", buffer.GetString());

        for (const auto &material : document["materials"].GetArray())
        {
            Material new_material = {};
            if (material.HasMember("pbrMetallicRoughness"))
            {
                const auto &pbr_metallic_roughness = material["pbrMetallicRoughness"].GetObj();

                // {"baseColorTexture":{"index":0,"texCoord":0,"extensions":{"KHR_texture_transform":{"offset":[0.125,0],"scale":[0.000183150187,0.000244200259]}}},"metallicFactor":0,"roughnessFactor":0.400000006}
                if (pbr_metallic_roughness.HasMember("baseColorTexture"))
                {
                    const auto &base_color_texture  = pbr_metallic_roughness["baseColorTexture"];
                    u32 texture_index               = base_color_texture["index"].GetUint();

                    const auto &texture = document["textures"][texture_index];
                    // texture: {"extensions":{"KHR_texture_basisu":{"source":0}}}

                    bool has_extension = false;
                    if (texture.HasMember("extensions"))
                    {
                        for (const auto &extension : texture["extensions"].GetObj())
                        {
                            if (std::string_view(extension.name.GetString()) == std::string_view("KHR_texture_basisu"))
                            {
                                has_extension = true;
                                break;
                            }
                        }
                    }

                    if (!has_extension)
                    {
                        logger::error("[GLB] Material references a texture that isn't in basis format.\n");
                    }
                    else
                    {
                        new_material.base_color_texture = texture["extensions"]["KHR_texture_basisu"]["source"].GetUint();
                    }

                    // TODO: Assert that all textures of this material have the same texture transform
                    if (base_color_texture.HasMember("extensions") && base_color_texture["extensions"].HasMember("KHR_texture_transform"))
                    {
                        const auto &extension = base_color_texture["extensions"]["KHR_texture_transform"];
                        if (extension.HasMember("offset"))
                        {
                            new_material.offset.raw[0] = extension["offset"].GetArray()[0].GetFloat();
                            new_material.offset.raw[1] = extension["offset"].GetArray()[1].GetFloat();
                        }
                        if (extension.HasMember("scale"))
                        {
                            new_material.scale.raw[0] = extension["scale"].GetArray()[0].GetFloat();
                            new_material.scale.raw[1] = extension["scale"].GetArray()[1].GetFloat();
                        }
                        if (extension.HasMember("rotation"))
                        {
                            new_material.rotation = extension["rotation"].GetFloat();
                        }
                    }
                }

                if (pbr_metallic_roughness.HasMember("baseColorFactor"))
                {
                    new_material.base_color_factor = {1.0, 1.0, 1.0, 1.0};
                    for (usize i = 0; i < 4; i += 1)
                    {
                        new_material.base_color_factor.raw[i] = pbr_metallic_roughness["baseColorFactor"].GetArray()[i].GetFloat();
                    }
                }
            }

            new_scene.materials.push_back(std::move(new_material));
        }
    }

    u32 i_scene = 0;
    if (document.HasMember("scene"))
    {
        i_scene = document["scene"].GetUint();
    }

    auto scenes = document["scenes"].GetArray();
    auto nodes = document["nodes"].GetArray();

    auto scene = scenes[i_scene].GetObject();
    auto roots = scene["nodes"].GetArray();

    Vec<u32> i_node_stack;
    Vec<float4x4> transforms_stack;
    i_node_stack.reserve(nodes.Size());
    transforms_stack.reserve(nodes.Size());

    auto get_transform = [](auto node) {
        float4x4 transform = float4x4::identity();


        if (node.HasMember("matrix"))
        {
            usize i = 0;
            auto matrix = node["matrix"].GetArray();
            assert(matrix.Size() == 16);

            for (u32 i_element = 0; i_element < matrix.Size(); i_element += 1) {
                transform.at(i%4, i/4) = static_cast<float>(matrix[i_element].GetDouble());
            }
        }

        if (node.HasMember("translation"))
        {
            auto translation_factors = node["translation"].GetArray();
            float4x4 translation = float4x4::identity();
            translation.at(0, 3) = static_cast<float>(translation_factors[0].GetDouble());
            translation.at(1, 3) = static_cast<float>(translation_factors[1].GetDouble());
            translation.at(2, 3) = static_cast<float>(translation_factors[2].GetDouble());
            transform = translation;
        }

        if (node.HasMember("rotation"))
        {
            auto rotation = node["rotation"].GetArray();
            float4 quaternion;
            quaternion.x = static_cast<float>(rotation[0].GetDouble());
            quaternion.y = static_cast<float>(rotation[1].GetDouble());
            quaternion.z = static_cast<float>(rotation[2].GetDouble());
            quaternion.w = static_cast<float>(rotation[3].GetDouble());

            transform = transform * float4x4({
                1.0f - 2.0f*quaternion.y*quaternion.y - 2.0f*quaternion.z*quaternion.z, 2.0f*quaternion.x*quaternion.y - 2.0f*quaternion.z*quaternion.w, 2.0f*quaternion.x*quaternion.z + 2.0f*quaternion.y*quaternion.w, 0.0f,
                2.0f*quaternion.x*quaternion.y + 2.0f*quaternion.z*quaternion.w, 1.0f - 2.0f*quaternion.x*quaternion.x - 2.0f*quaternion.z*quaternion.z, 2.0f*quaternion.y*quaternion.z - 2.0f*quaternion.x*quaternion.w, 0.0f,
                2.0f*quaternion.x*quaternion.z - 2.0f*quaternion.y*quaternion.w, 2.0f*quaternion.y*quaternion.z + 2.0f*quaternion.x*quaternion.w, 1.0f - 2.0f*quaternion.x*quaternion.x - 2.0f*quaternion.y*quaternion.y, 0.0f,
                0.0f, 0.0f, 0.0f, 1.0f,
            });
        }

        if (node.HasMember("scale"))
        {
            auto scale_factors = node["scale"].GetArray();
            float4x4 scale = {};
            scale.at(0, 0) = static_cast<float>(scale_factors[0].GetDouble());
            scale.at(1, 1) = static_cast<float>(scale_factors[1].GetDouble());
            scale.at(2, 2) = static_cast<float>(scale_factors[2].GetDouble());
            scale.at(3, 3) = 1.0f;

            transform = transform * scale;
        }

        return transform;
    };

    for (auto &root : roots)
    {
        u32 i_root = root.GetUint();

        i_node_stack.clear();
        transforms_stack.clear();

        i_node_stack.push_back(i_root);
        transforms_stack.push_back(float4x4::identity());

        while (!i_node_stack.empty())
        {
            u32 i_node = i_node_stack.back(); i_node_stack.pop_back();
            float4x4 parent_transform = transforms_stack.back(); transforms_stack.pop_back();

            auto node      = nodes[i_node].GetObject();
            auto transform = parent_transform * get_transform(node);

            if (node.HasMember("mesh"))
            {
                new_scene.instances.push_back({.i_mesh = mesh_remap[node["mesh"].GetUint()], .transform = transform});
            }

            if (node.HasMember("children"))
            {
                auto children = node["children"].GetArray();

                for (u32 i_child = 0; i_child < children.Size(); i_child += 1)
                {
                    i_node_stack.push_back(children[i_child].GetUint());
                    transforms_stack.push_back(transform);
                }
            }
        }
    }

}

Scene load_file(const std::string_view &path)
{
    auto file = platform::MappedFile::open(path);
    if (!file)
    {
        return {};
    }

    Scene scene = {};

    scene.file = std::move(*file);

    const auto &header = *reinterpret_cast<const Header*>(scene.file.base_addr);
    if (header.magic != 0x46546C67) {
        logger::error("[GLB] Invalid GLB file.\n");
        return {};
    }

    if (header.first_chunk.type != ChunkType::Json) {
        logger::error("[GLB] First chunk isn't JSON.\n");
        return {};
    }

    std::string_view json_content{reinterpret_cast<const char*>(&header.first_chunk.data), header.first_chunk.length};

    rapidjson::Document document;
    document.Parse(json_content.data(), json_content.size());

    if (document.HasParseError()) {
        logger::error("[GLB] JSON Error at offset {}: {}\n", document.GetErrorOffset(), rapidjson::GetParseError_En(document.GetParseError()));
        return {};
    }

    const Chunk *binary_chunk = nullptr;
    if (sizeof(Header) + header.first_chunk.length < header.length)
    {
        binary_chunk = reinterpret_cast<const Chunk*>(ptr_offset(header.first_chunk.data, header.first_chunk.length));
        if (binary_chunk->type != ChunkType::Binary) {
            logger::error("[GLB] Second chunk isn't BIN.\n");
            return {};
        }
    }

    process_json(scene, document, binary_chunk);

    return scene;
}
}

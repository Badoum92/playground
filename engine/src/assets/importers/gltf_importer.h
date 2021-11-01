#pragma once

#include "assets/subscene.h"
#include <exo/collections/vector.h>
#include <cross/uuid.h>

#include <rapidjson/fwd.h>

struct AssetManager;

enum struct GLTFError
{
    FirstChunkNotJSON,
    SecondChunkNotBIN,
};

struct GLTFImporter
{
    struct Settings
    {
        bool apply_transform = false;
        bool remove_degenerate_triangles = false;
        bool operator==(const Settings &other) const = default;
    };

    struct Data
    {
        Settings settings;
        Vec<cross::UUID> mesh_uuids;
    };

    bool can_import(const void *file_data, usize file_len);
    Result<Asset*> import(AssetManager *asset_manager, cross::UUID resource_uuid, const void *file_data, usize file_len, void *import_settings = nullptr);

    // Importer data
    void *create_default_importer_data();
    void *read_data_json(const rapidjson::Value &j_data);
    void write_data_json(rapidjson::GenericPrettyWriter<rapidjson::FileWriteStream> &writer, const void *data);
};

// automatically generated by the FlatBuffers compiler, do not modify


#ifndef FLATBUFFERS_GENERATED_MESH_ENGINE_SCHEMAS_H_
#define FLATBUFFERS_GENERATED_MESH_ENGINE_SCHEMAS_H_

#include "flatbuffers/flatbuffers.h"

#include "schemas/asset_generated.h"
#include "schemas/exo_generated.h"

namespace engine {
namespace schemas {

struct SubMesh;
struct SubMeshBuilder;

struct Mesh;
struct MeshBuilder;

struct SubMesh FLATBUFFERS_FINAL_CLASS : private flatbuffers::Table {
  typedef SubMeshBuilder Builder;
  enum FlatBuffersVTableOffset FLATBUFFERS_VTABLE_UNDERLYING_TYPE {
    VT_FIRST_INDEX = 4,
    VT_FIRST_VERTEX = 6,
    VT_INDEX_COUNT = 8,
    VT_MATERIAL = 10
  };
  uint32_t first_index() const {
    return GetField<uint32_t>(VT_FIRST_INDEX, 0);
  }
  uint32_t first_vertex() const {
    return GetField<uint32_t>(VT_FIRST_VERTEX, 0);
  }
  uint32_t index_count() const {
    return GetField<uint32_t>(VT_INDEX_COUNT, 0);
  }
  const engine::schemas::exo::UUID *material() const {
    return GetStruct<const engine::schemas::exo::UUID *>(VT_MATERIAL);
  }
  bool Verify(flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyField<uint32_t>(verifier, VT_FIRST_INDEX) &&
           VerifyField<uint32_t>(verifier, VT_FIRST_VERTEX) &&
           VerifyField<uint32_t>(verifier, VT_INDEX_COUNT) &&
           VerifyField<engine::schemas::exo::UUID>(verifier, VT_MATERIAL) &&
           verifier.EndTable();
  }
};

struct SubMeshBuilder {
  typedef SubMesh Table;
  flatbuffers::FlatBufferBuilder &fbb_;
  flatbuffers::uoffset_t start_;
  void add_first_index(uint32_t first_index) {
    fbb_.AddElement<uint32_t>(SubMesh::VT_FIRST_INDEX, first_index, 0);
  }
  void add_first_vertex(uint32_t first_vertex) {
    fbb_.AddElement<uint32_t>(SubMesh::VT_FIRST_VERTEX, first_vertex, 0);
  }
  void add_index_count(uint32_t index_count) {
    fbb_.AddElement<uint32_t>(SubMesh::VT_INDEX_COUNT, index_count, 0);
  }
  void add_material(const engine::schemas::exo::UUID *material) {
    fbb_.AddStruct(SubMesh::VT_MATERIAL, material);
  }
  explicit SubMeshBuilder(flatbuffers::FlatBufferBuilder &_fbb)
        : fbb_(_fbb) {
    start_ = fbb_.StartTable();
  }
  flatbuffers::Offset<SubMesh> Finish() {
    const auto end = fbb_.EndTable(start_);
    auto o = flatbuffers::Offset<SubMesh>(end);
    return o;
  }
};

inline flatbuffers::Offset<SubMesh> CreateSubMesh(
    flatbuffers::FlatBufferBuilder &_fbb,
    uint32_t first_index = 0,
    uint32_t first_vertex = 0,
    uint32_t index_count = 0,
    const engine::schemas::exo::UUID *material = nullptr) {
  SubMeshBuilder builder_(_fbb);
  builder_.add_material(material);
  builder_.add_index_count(index_count);
  builder_.add_first_vertex(first_vertex);
  builder_.add_first_index(first_index);
  return builder_.Finish();
}

struct Mesh FLATBUFFERS_FINAL_CLASS : private flatbuffers::Table {
  typedef MeshBuilder Builder;
  enum FlatBuffersVTableOffset FLATBUFFERS_VTABLE_UNDERLYING_TYPE {
    VT_ASSET = 4,
    VT_INDICES = 6,
    VT_POSITIONS = 8,
    VT_UVS = 10,
    VT_SUBMESHES = 12
  };
  const engine::schemas::Asset *asset() const {
    return GetPointer<const engine::schemas::Asset *>(VT_ASSET);
  }
  const flatbuffers::Vector<uint32_t> *indices() const {
    return GetPointer<const flatbuffers::Vector<uint32_t> *>(VT_INDICES);
  }
  const flatbuffers::Vector<const engine::schemas::exo::float4 *> *positions() const {
    return GetPointer<const flatbuffers::Vector<const engine::schemas::exo::float4 *> *>(VT_POSITIONS);
  }
  const flatbuffers::Vector<const engine::schemas::exo::float2 *> *uvs() const {
    return GetPointer<const flatbuffers::Vector<const engine::schemas::exo::float2 *> *>(VT_UVS);
  }
  const flatbuffers::Vector<flatbuffers::Offset<engine::schemas::SubMesh>> *submeshes() const {
    return GetPointer<const flatbuffers::Vector<flatbuffers::Offset<engine::schemas::SubMesh>> *>(VT_SUBMESHES);
  }
  bool Verify(flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyOffset(verifier, VT_ASSET) &&
           verifier.VerifyTable(asset()) &&
           VerifyOffset(verifier, VT_INDICES) &&
           verifier.VerifyVector(indices()) &&
           VerifyOffset(verifier, VT_POSITIONS) &&
           verifier.VerifyVector(positions()) &&
           VerifyOffset(verifier, VT_UVS) &&
           verifier.VerifyVector(uvs()) &&
           VerifyOffset(verifier, VT_SUBMESHES) &&
           verifier.VerifyVector(submeshes()) &&
           verifier.VerifyVectorOfTables(submeshes()) &&
           verifier.EndTable();
  }
};

struct MeshBuilder {
  typedef Mesh Table;
  flatbuffers::FlatBufferBuilder &fbb_;
  flatbuffers::uoffset_t start_;
  void add_asset(flatbuffers::Offset<engine::schemas::Asset> asset) {
    fbb_.AddOffset(Mesh::VT_ASSET, asset);
  }
  void add_indices(flatbuffers::Offset<flatbuffers::Vector<uint32_t>> indices) {
    fbb_.AddOffset(Mesh::VT_INDICES, indices);
  }
  void add_positions(flatbuffers::Offset<flatbuffers::Vector<const engine::schemas::exo::float4 *>> positions) {
    fbb_.AddOffset(Mesh::VT_POSITIONS, positions);
  }
  void add_uvs(flatbuffers::Offset<flatbuffers::Vector<const engine::schemas::exo::float2 *>> uvs) {
    fbb_.AddOffset(Mesh::VT_UVS, uvs);
  }
  void add_submeshes(flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<engine::schemas::SubMesh>>> submeshes) {
    fbb_.AddOffset(Mesh::VT_SUBMESHES, submeshes);
  }
  explicit MeshBuilder(flatbuffers::FlatBufferBuilder &_fbb)
        : fbb_(_fbb) {
    start_ = fbb_.StartTable();
  }
  flatbuffers::Offset<Mesh> Finish() {
    const auto end = fbb_.EndTable(start_);
    auto o = flatbuffers::Offset<Mesh>(end);
    return o;
  }
};

inline flatbuffers::Offset<Mesh> CreateMesh(
    flatbuffers::FlatBufferBuilder &_fbb,
    flatbuffers::Offset<engine::schemas::Asset> asset = 0,
    flatbuffers::Offset<flatbuffers::Vector<uint32_t>> indices = 0,
    flatbuffers::Offset<flatbuffers::Vector<const engine::schemas::exo::float4 *>> positions = 0,
    flatbuffers::Offset<flatbuffers::Vector<const engine::schemas::exo::float2 *>> uvs = 0,
    flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<engine::schemas::SubMesh>>> submeshes = 0) {
  MeshBuilder builder_(_fbb);
  builder_.add_submeshes(submeshes);
  builder_.add_uvs(uvs);
  builder_.add_positions(positions);
  builder_.add_indices(indices);
  builder_.add_asset(asset);
  return builder_.Finish();
}

inline flatbuffers::Offset<Mesh> CreateMeshDirect(
    flatbuffers::FlatBufferBuilder &_fbb,
    flatbuffers::Offset<engine::schemas::Asset> asset = 0,
    const std::vector<uint32_t> *indices = nullptr,
    const std::vector<engine::schemas::exo::float4> *positions = nullptr,
    const std::vector<engine::schemas::exo::float2> *uvs = nullptr,
    const std::vector<flatbuffers::Offset<engine::schemas::SubMesh>> *submeshes = nullptr) {
  auto indices__ = indices ? _fbb.CreateVector<uint32_t>(*indices) : 0;
  auto positions__ = positions ? _fbb.CreateVectorOfStructs<engine::schemas::exo::float4>(*positions) : 0;
  auto uvs__ = uvs ? _fbb.CreateVectorOfStructs<engine::schemas::exo::float2>(*uvs) : 0;
  auto submeshes__ = submeshes ? _fbb.CreateVector<flatbuffers::Offset<engine::schemas::SubMesh>>(*submeshes) : 0;
  return engine::schemas::CreateMesh(
      _fbb,
      asset,
      indices__,
      positions__,
      uvs__,
      submeshes__);
}

inline const engine::schemas::Mesh *GetMesh(const void *buf) {
  return flatbuffers::GetRoot<engine::schemas::Mesh>(buf);
}

inline const engine::schemas::Mesh *GetSizePrefixedMesh(const void *buf) {
  return flatbuffers::GetSizePrefixedRoot<engine::schemas::Mesh>(buf);
}

inline const char *MeshIdentifier() {
  return "MESH";
}

inline bool MeshBufferHasIdentifier(const void *buf) {
  return flatbuffers::BufferHasIdentifier(
      buf, MeshIdentifier());
}

inline bool VerifyMeshBuffer(
    flatbuffers::Verifier &verifier) {
  return verifier.VerifyBuffer<engine::schemas::Mesh>(MeshIdentifier());
}

inline bool VerifySizePrefixedMeshBuffer(
    flatbuffers::Verifier &verifier) {
  return verifier.VerifySizePrefixedBuffer<engine::schemas::Mesh>(MeshIdentifier());
}

inline void FinishMeshBuffer(
    flatbuffers::FlatBufferBuilder &fbb,
    flatbuffers::Offset<engine::schemas::Mesh> root) {
  fbb.Finish(root, MeshIdentifier());
}

inline void FinishSizePrefixedMeshBuffer(
    flatbuffers::FlatBufferBuilder &fbb,
    flatbuffers::Offset<engine::schemas::Mesh> root) {
  fbb.FinishSizePrefixed(root, MeshIdentifier());
}

}  // namespace schemas
}  // namespace engine

#endif  // FLATBUFFERS_GENERATED_MESH_ENGINE_SCHEMAS_H_

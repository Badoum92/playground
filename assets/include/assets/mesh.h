#pragma once
#include <exo/collections/handle.h>
#include <exo/collections/vector.h>
#include <exo/maths/vectors.h>

#include "assets/asset.h"

struct Material;

struct SubMesh
{
	u32       first_index  = 0;
	u32       first_vertex = 0;
	u32       index_count  = 0;
	exo::UUID material     = {};

	bool operator==(const SubMesh &other) const = default;
};

// Dependencies: Material
struct Mesh : Asset
{
	static Asset *create();
	const char   *type_name() const final { return "Mesh"; }
	void          serialize(exo::Serializer &serializer) final;

	// doesn't check the name
	bool is_similar(const Mesh &other) const
	{
		return indices == other.indices && positions == other.positions && submeshes == other.submeshes;
	}
	bool operator==(const Mesh &other) const = default;

	// --
	Vec<u32>     indices;
	Vec<float4>  positions;
	Vec<float2>  uvs;
	Vec<SubMesh> submeshes;
};

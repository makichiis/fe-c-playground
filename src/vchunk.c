#include <fe/geometries/vchunk.h>
#include <fe/logger.h>
#include <fe/err.h>

#include <cglm/cglm.h>

#include <string.h>

struct Chunk Chunk__create(struct Size3D chunk_size) {
    struct Chunk chunk = {.scale = 1.0, .size = chunk_size};

    size_t allocsz = sizeof (struct Voxel)
        * chunk.size.x
        * chunk.size.y
        * chunk.size.z;
    chunk.voxels = calloc(chunk.size.x * chunk.size.y * chunk.size.z, sizeof (struct Voxel));
    
    if (!chunk.voxels) {
        FE_FATAL("Could not allocate %lu bytes for voxel chunk.", allocsz); 
        exit(FE_ERR_BAD_ALLOC);
    }

    return chunk;
}

void Chunk_destroy(struct Chunk* chunk) {
    if (!chunk) {
        // TODO: Mention caller (via frame pointer ?) ?
    // https://stackoverflow.com/a/64291642
        FE_WARNING("Warning: NULL chunk passed to `Chunk_destroy`");
        return;
    }

    free(chunk->voxels); 
}

struct Size3D Chunk_get_iaspos(struct Chunk* chunk, size_t idx) {
    struct Size3D size = chunk->size;
    
    uint32_t ix = (uint32_t)((idx % (size.x * size.y)) % size.x);
    uint32_t iy = (uint32_t)((idx % (size.x * size.y)) / size.x);
    uint32_t iz = (uint32_t)(idx / (size.x * size.y));

    return (struct Size3D){ ix, iy, iz };
}

// Verts per voxel = verts per side * 6 
// verts pre side = verts per polygon * polygons per side 
// verts per polygon = 3 
// verts per side = 2 
// verts per voxel = 3 * 2 * 6 = 36 
// scalars per voxel = 36 * points per vertex = 36 * 3 = 108

#define SCALARS_PER_VERTEX 3 
#define VERTICES_PER_POLYGON 3
#define POLYGONS_PER_FACE 2
#define FACES_PER_VOXEL 6 
#define VERTS_PER_VOXEL (SCALARS_PER_VERTEX\
    * VERTICES_PER_POLYGON\
    * POLYGONS_PER_FACE\
    * FACES_PER_VOXEL)


// CLOCKWISE RENDERING ALWAYS
static struct vc__mesh_vertex vc_vverts[] = {
    { 0.f, 1.f, 0.f },
    { 1.f, 1.f, 0.f },
    { 1.f, 0.f, 0.f },
    { 1.f, 0.f, 0.f },
    { 0.f, 0.f, 0.f },
    { 0.f, 1.f, 0.f },

    /*{ 1.f, 1.f, 0.f },
    { 1.f, 0.f, 0.f },
    { 1.f, 0.f, -1.f },
    { 1.f, 0.f, -1.f },
    { 1.f, 1.f, -1.f },
    { 1.f, 1.f, 0.f },*/

    { 1.f, 1.f, 0.f },
    { 1.f, 1.f, 1.f },
    { 1.f, 0.f, 1.f },
    { 1.f, 0.f, 1.f },
    { 1.f, 0.f, 0.f },
    { 1.f, 1.f, 0.f },

    { 1.f, 1.f, 1.f },
    { 0.f, 1.f, 1.f },
    { 0.f, 0.f, 1.f },
    { 0.f, 0.f, 1.f },
    { 1.f, 0.f, 1.f },
    { 1.f, 1.f, 1.f },

    { 0.f, 1.f, 1.f },
    { 0.f, 1.f, 0.f },
    { 0.f, 0.f, 0.f },
    { 0.f, 0.f, 0.f },
    { 0.f, 0.f, 1.f },
    { 0.f, 1.f, 1.f },

    { 0.f, 1.f, 1.f },
    { 1.f, 1.f, 1.f },
    { 1.f, 1.f, 0.f },
    { 1.f, 1.f, 0.f },
    { 0.f, 1.f, 0.f },
    { 0.f, 1.f, 1.f },

    { 0.f, 0.f, 1.f },
    { 0.f, 0.f, 0.f },
    { 1.f, 0.f, 0.f },
    { 1.f, 0.f, 0.f },
    { 1.f, 0.f, 1.f },
    { 0.f, 0.f, 1.f }
};

#define VC__MV_ELEMS (sizeof vc_vverts / sizeof (struct vc__mesh_vertex))

// TODO: Make way to pass relational chunks/faces to perform culling of 
// outwardly-facing but still-hidden faces. Future concern.
struct vc__float_verts_t vc__create_verts_dumb_naive(
        struct Chunk* chunk) {
    struct vc__float_verts_t verts;

    verts.cap = chunk->size.x * chunk->size.y * chunk->size.z * VERTS_PER_VOXEL;
    verts.len = 0ULL;
    verts.data = calloc(verts.cap, sizeof *verts.data);

    float scale = (float)chunk->scale;

    if (!verts.data) {
        FE_FATAL("Failed to allocate %ld bytes for vertices. Exiting.", 
                 verts.len * sizeof (float));
        exit(FE_ERR_BAD_ALLOC);
    }
    FE_DEBUG("%ld bytes allocated for chunk.", verts.cap * sizeof *verts.data); 

    // this is probably an awful way to index. i havent decided how to do the 
    // mapping. I hope this is correct though ..
    size_t voxel_len = chunk->size.x * chunk->size.y * chunk->size.z;
    for (size_t i = 0; i < voxel_len; ++i) {
        if (!chunk->voxels[i].enabled)
            continue;

        struct Size3D pos = Chunk_get_iaspos(chunk, i);

        // vert = (pos_3v * scale + vpos_3v)
        // "local voxel pos times scale plus local chunk pos"

        // TODO: Multiply by scale, shift by local chunk coord

        struct vc__mesh_vertex transverts[VC__MV_ELEMS] = {};
        memcpy(transverts, vc_vverts, sizeof vc_vverts);

        vec3 locvec = { (float)pos.x, (float)pos.y, (float)pos.z };

        for (int i = 0; i < VC__MV_ELEMS; ++i) {
            //glm_vec3_add((float*)&transverts[i], locvec, (float*)&transverts[i]);
            transverts[i].x += pos.x;
            transverts[i].y += pos.y;
            transverts[i].z += pos.z;

            transverts[i].x *= scale;
            transverts[i].y *= scale;
            transverts[i].z *= scale;
        }

        memcpy(verts.data + verts.len, transverts, sizeof vc_vverts);
        //verts.len += 18 * 4;
        verts.len += sizeof (vc_vverts) / sizeof vc_vverts[0] * 3;
    }

    return verts;
}

void vc__float_verts_destroy(struct vc__float_verts_t* verts) {
    free(verts->data);
}

// TODO: Look into "recycling" existing chunk data to optimize rebuild
// times (for when only a few voxels are destroy), but maybe not 
// necessary with sufficiently efficient renderers.
struct ChunkMesh ChunkMesh__from_chunk(struct Chunk* chunk) {
    struct ChunkMesh mesh = {};

    struct vc__float_verts_t verts = vc__create_verts_dumb_naive(chunk);
    mesh.verts = verts;

    glGenVertexArrays(1, &mesh.vao);
    glGenBuffers(1, &mesh.vbo);

    glBindVertexArray(mesh.vao);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);

    glBufferData(GL_ARRAY_BUFFER, verts.len * sizeof (float), 
                 verts.data, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof (float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    vc__float_verts_destroy(&verts);
    return mesh;
}

size_t ChunkMesh_polygon_count(struct ChunkMesh* mesh) {
    return mesh->verts.len;
}

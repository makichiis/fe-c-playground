#ifndef VOXEL_CHUNK_H
#define VOXEL_CHUNK_H 

#include <glad/gl.h>

#include <stdlib.h>
#include <stdint.h>

struct Size3D {
    uint32_t x;
    uint32_t y;
    uint32_t z;
};

struct Voxel {
    uint8_t enabled:    1;
    uint8_t pad:        7;
} __attribute__((packed));

struct Chunk {
    double scale;
    struct Size3D size;
    struct Voxel* voxels;
};

struct vc__mesh_vertex {
    float x;
    float y;
    float z;
} __attribute__((packed)); // we are copying bytes directly 

struct vc__float_verts_t {
    size_t cap;
    size_t len;
    float* data;
};

struct ChunkMesh {
    GLuint vao;
    GLuint vbo;
    struct vc__float_verts_t verts;
};

/** 
 * @brief Initialize an empty chunk of size `size`. The underlying
 * data must be freed via a call to `Chunk__destroy()` when this
 * chunk is no longer in use.
 */
struct Chunk Chunk__create(struct Size3D chunk_size);

/** 
 * @brief Destroys a chunk and frees internal resources. Since the 
 * encompassing `struct Chunk` type is not constructed via heap
 * allocation, it is not freed by `Chunk_destroy(struct Chunk*)` 
 * either.
 */
void Chunk_destroy(struct Chunk* chunk);

/**
 * @brief Computes the local (x, y, z) coordinates of a given index 
 * within this chunk. 
 * @return The 3D cordinates, or {0, 0, 0} if idx is invalid.
 */ 
struct Size3D Chunk_get_iaspos(struct Chunk* chunk, size_t idx);

/** 
 * @brief Generates a chunk mesh from `chunk`.
 * @returns `ChunkMesh` containing the chunk mesh VAO and VBO,
 * and other necessary metadata (if any).
 */
struct ChunkMesh ChunkMesh__from_chunk(struct Chunk* chunk);

#endif 

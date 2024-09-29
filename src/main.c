/************************************************************
 * Framework Engine
 * makichiis. All rights reserved.
 *
 * All code and declarations in this file are relevant 
 * only to the driver code, hence their use here.
 *
 * The only headers relevant to the Framework Engine 
 * API are located in ${root}/inc/fe/, but this may 
 * change.
 ************************************************************/ 

#include <glad/gl.h>
#include <GLFW/glfw3.h> 
#include <cglm/cglm.h>

#include <fe/geometries/vchunk.h>
#include <fe/glfw_callbacks.h>
#include <fe/glinfo.h>
#include <fe/logger.h>
#include <fe/err.h>

#include <demo/noise1234.h>

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/time.h>
#include <linux/limits.h>

#define LOG_BAR "--------------------------------------------"

#ifndef FE_VERSION
#pragma GCC warning "This file is likely not being built by CMake,"\
    " or this is a bug. Expect this to break some things."
#define FE_VERSION 
#define FE_VERSION_MAJOR 0
#define FE_VERSION_MINOR 0 
#define FE_VERSION_PATCH 1
#define FE_VERSION_TIME "<error>"
#endif 

/**
 * @brief Verifies the integrity of the current working directory 
 * by scanning for necessary metadata/assets files. Basically just
 * checks if resources/ exists.
 * @return CWD_VALID on success, non-zero value on failure. Exit with this code.
 */ 
int verify_working_directory();
#define CWD_VALID 0

/*
 * @brief Retrieves the byte contents of a resource from the specified path. 
 * The resource referenced by the result `void*` must be freed when no longer 
 * in use.
 * @return A reference to a resource, or NULL on failure. The parameters
 * passed to get_resource() are set to the address of the data and the number
 * of bytes the data comprises respectively.
*/
void* get_resource(const char* path, void** data_p, size_t* size);

static mat4 projection;

void glfw_framebuffer_size_callback(GLFWwindow* window, int x, int y) {
    glViewport(0, 0, x, y);
    glm_mat4_identity(projection);
    glm_perspective(glm_rad(60.0f), (float)x/(float)y, 1, 10000, projection);
}

// TODO: Re-research event subscriber pattern for glfw et al. event handling 
// TODO: Rewrite build script that bootstraps cmake configurations 
// TODO: Logger colors ! https://gist.github.com/RabaDabaDoba/145049536f815903c79944599c6f952a
// TODO: Figure out basic module/mesh/render pipeline
// TODO: Decide on voxel structure(s)

int main(int argc, const char** argv) {
    char cwd[PATH_MAX];

    if (!getcwd(cwd, sizeof cwd)) {
        FE_FATAL("Could not get CWD. Is this running on Linux at all?");
        return FE_ERR_INFALLIBLE;
    }

    FE_INFO("cwd: %s", cwd);

    int status = verify_working_directory();
    if (status != CWD_VALID) {
        FE_FATAL("Could not verify integrity of current working directory.");
        return status;
    }

    FE_DEBUG("folder '%s/resources' exists.", cwd);

    FE_INFO("Initializing GLFW...");

    if (glfwInit() == GLFW_FALSE) {
        FE_FATAL("Could not initialize GLFW.");
        return FE_ERR_GLFW_INIT;
    }
    glfwSetErrorCallback(fe_base_glfw_error_callback);

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,  FE_OPENGL_VERSION_MAJOR);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,  FE_OPENGL_VERSION_MINOR);
    glfwWindowHint(GLFW_OPENGL_PROFILE,         FE_OPENGL_PROFILE);

    FE_INFO("GLFW initialized successfully.");

    // replace with program options 
    GLFWwindow* window = glfwCreateWindow(
        FE_GLFW_DEFAULT_WINDOW_LENGTH,
        FE_GLFW_DEFAULT_WINDOW_HEIGHT,
        FE_GLFW_DEFAULT_WINDOW_TITLE,
        FE_GLFW_CURRENT_MONITOR,
        FE_GLFW_NO_CONTEXT_SHARE 
    ); 

    if (!window) {
        FE_FATAL("GLFW window context failed to initialize.");
        return FE_ERR_GLFW_WINDOW_INIT;
    }

    glfwMakeContextCurrent(window);

    // TODO: Event subscriber pattern that attaches multiple callbacks 
    // to base callback (it isnt that deep, just call them in the 
    // base callback)
    glfwSetKeyCallback(window, fe_base_glfw_key_callback);
    glfwSetFramebufferSizeCallback(window, glfw_framebuffer_size_callback);

    if (!gladLoadGL(glfwGetProcAddress)) {
        FE_FATAL("GLAD failed to initialize.");
        return FE_ERR_GLAD_INIT;
    }

    FE_INFO("Engine initialized successfully.");
    FE_INFO(LOG_BAR);
    FE_INFO("Welcome to my graphics demo.");
    FE_INFO("v%d.%d.%d (%s)", FE_VERSION_MAJOR,
            FE_VERSION_MINOR,
            FE_VERSION_PATCH,
            FE_VERSION_TIME);
    FE_INFO("makichiis. All rights reserved.");
    FE_INFO(LOG_BAR);

    FE_WARNING("Finish this project by September 18th.");

    GLuint program = glCreateProgram();
    {
        size_t shader_src_vert_length = 0;
        char* shader_src_vert = get_resource("resources/default_vertex.glsl", 
                                         NULL, &shader_src_vert_length);
        if (!shader_src_vert) {
            FE_ERROR("Could not load shader source.");
            return 1;
        }

        size_t shader_src_frag_length = 0;
        char* shader_src_frag = get_resource("resources/default_fragment.glsl",
                                         NULL, &shader_src_frag_length);
        if (!shader_src_frag) {
            FE_ERROR("Could not load shader source.");
            return 1;
        }

        FE_DEBUG(shader_src_vert);
        FE_DEBUG(shader_src_frag);

        GLuint vert = glCreateShader(GL_VERTEX_SHADER);
        GLuint frag = glCreateShader(GL_FRAGMENT_SHADER);

        GLint shader_src_vert_len = (GLint)shader_src_vert_length;
        GLint shader_src_frag_len = (GLint)shader_src_frag_length;

        glShaderSource(vert, 1, 
                       (const GLchar* const *)&shader_src_vert,
                       &shader_src_vert_len);
        glShaderSource(frag, 1, 
                       (const GLchar* const *)&shader_src_frag,
                       &shader_src_frag_len);

        glCompileShader(vert);
        int success;
        char info_log[512];
        glGetShaderiv(vert, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(vert, 512, NULL, info_log);
            FE_ERROR("Failed to compile vertex shader: %s", info_log);
            goto END_SHADER_BUILD;
        }

        glCompileShader(frag);
        glGetShaderiv(frag, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(frag, 512, NULL, info_log);
            FE_ERROR("Failed to compile fragment shader: %s", info_log);
            goto END_SHADER_BUILD;
        } 

        glAttachShader(program, vert);
        glAttachShader(program, frag);
        glLinkProgram(program);

        glGetProgramiv(program, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(program, 512, NULL, info_log);
            FE_ERROR("Failed to link shader program: %s", info_log);
        }

        END_SHADER_BUILD:
        glDeleteShader(vert);
        glDeleteShader(frag);
        free(shader_src_vert);
        free(shader_src_frag);
    } // TODO: Analyze with valgrind

#ifdef DEBUG 
    if (argc >= 2 && strcmp(argv[1], "dont") == 0) {
        FE_WARNING("This is an init test run. Shutting down.");
        glfwTerminate();
        return 0;
    }
#endif

    float points[9] = {
        -1.0f, 0.0f, -2.0f,
        1.0f,  0.0f, 0.0f,
        -1.0f, 1.0f, 0.0f 
    };

    GLuint vao;
    GLuint vbo;

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    glBufferData(GL_ARRAY_BUFFER, sizeof points, points, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof (float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    //struct Chunk base_chunk = Chunk__create(
    //    (struct Size3D){.x = 64, .y = 64, .z = 64});
    //FE_DEBUG("Address of voxel data: 0x%8X, size: %lu", base_chunk.voxels, 
    //         sizeof (struct Voxel) * base_chunk.size.x 
    //         * base_chunk.size.y * base_chunk.size.z);

    //base_chunk.scale = 0.25;
    //struct ChunkMesh base_mesh = ChunkMesh__from_chunk(&base_chunk);

    /*struct Chunk test = Chunk__create((struct Size3D){ 4, 4, 2 });
    test.scale = 0.5f;
    test.voxels[0].enabled = true;
    test.voxels[1].enabled = true;
    test.voxels[2].enabled = true;
    test.voxels[3].enabled = true;
    test.voxels[4].enabled = true;
    test.voxels[20].enabled = true;*/ 

    struct Chunk test = Chunk__create((struct Size3D){ 16, 16, 16 });
    for (size_t i = 0; i < 16 * 16 * 16; ++i) {
        struct Size3D coord = Chunk_get_iaspos(&test, i);

        //printf("%d %d %d\n", coord.x, coord.y, coord.z);

        double noise = 0.0;

        noise = noise3((float)coord.x / 10., 
                       (float)coord.y / 10., 
                       (float)coord.z / 10.);

        //printf("%.2lf ", noise);
        if (noise >= 0.16) test.voxels[i].enabled = true;
    } //puts("");

    //struct Size3D p = Chunk_get_iaspos(&test, 30);
    //FE_DEBUG("idx 30 for chunk (4, 4, 2) is in pos %u %u %u\n", p.x, p.y, p.z);

    struct ChunkMesh test_mesh = ChunkMesh__from_chunk(&test);

    // initialize camera position matrix 
    
    glm_mat4_identity(projection);
    glm_perspective(glm_rad(60.0f), 400.0/400.0, 1, 100000, projection);

    vec3 camera_pos = { 0.0f, 0.0f, -3.0f };
    float pitch = 0.f;
    float yaw = 0.f;
    
    glUseProgram(program);
    GLuint u_transform = glGetUniformLocation(program, "u_transform");
    GLuint u_resolution = glGetUniformLocation(program, "u_resolution");
    GLuint u_time = glGetUniformLocation(program, "u_time");
    GLuint u_lightpos = glGetUniformLocation(program, "u_lightpos");
    glUniform2f(u_resolution, 400.0f, 400.0f);
    glUniform1f(u_time, 0.0f);
    glUseProgram(0);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    int enabled = 0;
    size_t size = test.size.x * test.size.y * test.size.z;
    for (size_t i = 0; i < size; ++i) {
        enabled += test.voxels[i].enabled ? 1 : 0;
    }

    float yaw_mult = 60.0f;
    float pitch_mult = 60.0f;
    float speed_mult = 20.0f;

    float delta_time = 0.0f;

    while (!glfwWindowShouldClose(window)) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        //glClearColor(0.3f, 0.3f, 0.35f, 1.0f); // cool editor bg
        
        struct timeval frametime_start;
        gettimeofday(&frametime_start, NULL);

        if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS)
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        else if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS)
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            //yaw -= .01f;
            yaw -= yaw_mult * delta_time;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            //yaw += .01f;
            yaw += yaw_mult * delta_time;
        if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS)
            //pitch += .01f;
            pitch += pitch_mult * delta_time;
        if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS)
            //pitch -= .01f;
            pitch -= pitch_mult * delta_time;
        
        FE_WARNING("%f", delta_time);
        
        vec3 direction = {};
        direction[0] = cos(glm_rad(yaw)) * cos(glm_rad(pitch));
        direction[1] = sin(glm_rad(pitch));
        direction[2] = sin(glm_rad(yaw)) * cos(glm_rad(pitch));
        vec3 camera_front;
        glm_vec3_copy(direction, camera_front);
        glm_normalize(camera_front);
        
        vec3 mul = { 
            speed_mult * delta_time, 
            speed_mult * delta_time, 
            speed_mult * delta_time };
        glm_vec3_mul(camera_front, mul, camera_front);
        
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            glm_vec3_add(camera_pos, camera_front, camera_pos);
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            glm_vec3_sub(camera_pos, camera_front, camera_pos);

        vec3 camera_up = { 0.f, 1.f, 0.f };
        vec3 camera_trans;
        glm_vec3_add(camera_pos, camera_front, camera_trans);

        mat4 view;
        glm_mat4_identity(view);
        glm_lookat(camera_pos, camera_trans, camera_up, view);

        mat4 trans;
        glm_mat4_identity(trans);
        glm_mat4_mul(projection, view, trans);

        glUseProgram(program);
        glUniformMatrix4fv(u_transform, 1, GL_FALSE, (const GLfloat*)trans);
        glUniform3f(u_lightpos, 4.5f, 3.6f, 0.0f);
        //glUniform2f(u_resolution, 400.0f, 400.0f);
        //glUniform1f(u_time, glfwGetTime());

        glBindVertexArray(test_mesh.vao);
        glDrawArrays(GL_TRIANGLES, 0, ChunkMesh_polygon_count(&test_mesh));
        //glBindVertexArray(vao);
        //glDrawArrays(GL_TRIANGLES, 0, 3);
        
        

        glfwPollEvents();
        glfwSwapBuffers(window);

        struct timeval frametime_end;
        gettimeofday(&frametime_end, NULL);

        float frametime_seconds 
            = (frametime_end.tv_sec - frametime_start.tv_sec)
            + ((float)frametime_end.tv_usec - (float)frametime_start.tv_usec) / 1000000;
        delta_time = frametime_seconds;
    }

    //Chunk_destroy(&base_chunk);
    Chunk_destroy(&test);
    free(test_mesh.verts.data);
    glfwTerminate();
}

int verify_working_directory() {
    if (access("./resources/", F_OK) == 0) {
        return CWD_VALID;
    } else {
        return FE_ERR_INVALID_CWD;
    }
}

void* get_resource(const char* path, void** data_p, size_t* size) {
    void** data_pinit = data_p;
    FILE* fp = fopen(path, "r");
    if (!fp) {
        return NULL;
    }

    fseek(fp, 0L, SEEK_END);
    size_t file_bytesize = ftell(fp);
    fseek(fp, 0L, SEEK_SET);

    size_t bufsz = file_bytesize + 1;
    void* data;
    if (data_p) {
        *data_p = realloc(*data_p, bufsz);
        data = *data_p;
    } else {
        data = malloc(bufsz);
    }
    *size = file_bytesize;
    ((char*)data)[file_bytesize] = '\0';

    if (fread(data, sizeof (char), *size, fp) != file_bytesize) {
        FE_ERROR("Could not retrieve resource %s.", path);
        if (data_pinit == NULL)
            free(data);
        fclose(fp);
        return NULL;
    }
    fclose(fp);

    return data;
}


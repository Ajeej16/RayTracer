
#ifndef RENDERER_H
#define RENDERER_H

struct material_t {
    vec3 rgb;
    f32 smoothness;
    vec3 emission_color;
    f32 emission_strength;
};

struct sphere_t {
    vec3 pos;
    f32 r;
    u32 mat_id;
    f32 padding[3];
};

struct triangle_t {
    vec3 v0;
    f32 p0;
    vec3 v1;
    f32 p1;
    vec3 v2;
    f32 p2;
    vec3 n0;
    f32 p3;
    vec3 n1;
    f32 p4;
    vec3 n2;
    f32 p5;
};

struct mesh_t {
    vec3 pos;
    f32 p0;
    versor rot;
    vec3 scale;
    u32 tri_idx;
    u32 tri_count;
    u32 mat_id;
    f32 p[2];
};

struct camera_t {
    vec3 pos, front, side, up;
    f32 yaw, pitch;
    
    f32 near_plane, far_plane;
    
    u32 width, height;
    f32 p_width, p_height;
    
    u32 perspective;
};

struct render_settings_t {
    u32 max_bounce;
    vec3 horizon_color;
    vec3 zenith_color;
    vec3 ground_color;
};

struct scene_t {
    STACK(sphere_t) *spheres;
    STACK(triangle_t) *triangles;
    STACK(material_t) *mats;
    STACK(mesh_t) *meshes;
    
    render_settings_t settings;
    u32 sphere_buffer, mat_buffer, tri_buffer, mesh_buffer;
    bool moving, clean_frame;
    bool ambient, diffuse, specular;
};

#endif //RENDERER_H

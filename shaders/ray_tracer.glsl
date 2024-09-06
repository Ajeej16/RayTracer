#version 430

layout(local_size_x = 8, local_size_y = 8) in;
layout(rgba32f, binding = 0) uniform image2D texture;

struct Sphere {
    vec3 pos;
    float r;
    uint mat_id;
};

struct Mesh {
    vec3 pos;
    vec4 quat;
    vec3 scale;
    uint tri_idx;
    uint tri_count;
    uint mat_id;
};

struct Triangle {
    vec3 v[3];
    vec3 n[3];
};

struct Material {
    vec3 color;
    float smoothness;
    vec3 emission_color;
    float emission_strength;
};

struct Ray {
    vec3 origin;
    vec3 dir;
};

struct HitInfo {
    vec3 point;
    vec3 norm;
    float dist;
    uint mat_id;
    bool hit;
};

layout(std430, binding = 1) buffer SphereBuffer {
    Sphere spheres[];
};

layout(std430, binding = 2) buffer MaterialBuffer {
    Material mats[];
};

layout(std430, binding = 3) buffer TriangleBuffer {
    Triangle triangles[];
};

layout(std430, binding = 4) buffer MeshBuffer {
    Mesh meshes[];
};

uniform uint sphere_count;
uniform uint mesh_count;
uniform vec3 camera_pos;
uniform vec3 forward;
uniform vec3 right;
uniform vec3 up;
uniform uint max_bounce;
uniform uint frame_count;

uniform vec3 horizon_color;
uniform vec3 zenith_color;
uniform vec3 ground_color;
uniform vec3 sun_dir;
uniform float sun_focus;
uniform float sun_intensity;
uniform uint perspective;


vec3 get_color_from_environment(Ray ray)
{
    float sky_gradient_t = pow(smoothstep(0.0, 0.4, ray.dir.y), 0.35);
    vec3 sky_gradient = mix(horizon_color, zenith_color, sky_gradient_t);
    
    float ground_to_sky_t = smoothstep(-0.01, 0.0, ray.dir.y);
    return mix(ground_color, sky_gradient, ground_to_sky_t);
}

float
gen_random_number(inout uint state)
{
    state = state * 74779605 + 2891336453;
    uint result = ((state >> ((state >> 28) + 4)) ^ state) * 277803737;
    result = (result >> 22) ^ result;
    return result / 4294967295.0;
}

float
gen_random_normal_number(inout uint state)
{
    float theta = 2 * 3.1415926 * gen_random_number(state);
    float rho = sqrt(-2 * log(gen_random_number(state)));
    return rho * cos(theta);
}

vec3
gen_random_hemisphere_dir(vec3 norm, inout uint state)
{
    vec3 dir = vec3(gen_random_normal_number(state),
                    gen_random_normal_number(state),
                    gen_random_normal_number(state));
    return dir * sign(dot(norm, dir));
}

HitInfo
intersect_sphere(Ray ray, Sphere sphere)
{
    HitInfo info;
    info.hit = false;
    
    vec3 s_to_r = ray.origin - sphere.pos;
    
    float a = dot(ray.dir, ray.dir);
    float b = 2*dot(s_to_r, ray.dir);
    float c = dot(s_to_r, s_to_r) - sphere.r*sphere.r;
    
    float disc = b*b - 4*a*c;
    
    if(disc >= 0)
    {
        float dist = (-b - sqrt(disc))/(2*a);
        if(dist >= 0) {
            info.hit = true;
            info.dist = dist;
            info.mat_id = sphere.mat_id;
            info.point = ray.origin + ray.dir * dist;
            info.norm = normalize(info.point - sphere.pos);
        }
    }
    
    return info;
}

vec3 rotate_vertex(vec3 vert, vec4 quat)
{
    float q0 = quat.w;
    vec3 qv = quat.xyz;
    
    vec3 t = 2.0 * cross(qv, vert);
    return vert + q0 * t + cross(qv, t);
}

HitInfo
intersect_triangle(Ray ray, Triangle tri,
                   vec3 pos, vec4 quat, vec3 scale)
{
    HitInfo info;
    info.hit = false;
    
    float u, v, w, t;
    
    vec3 v0 = rotate_vertex(tri.v[0]*scale, quat) + pos;
    vec3 v1 = rotate_vertex(tri.v[1]*scale, quat) + pos;
    vec3 v2 = rotate_vertex(tri.v[2]*scale, quat) + pos;
    
    vec3 n0 = rotate_vertex(tri.n[0], quat);
    vec3 n1 = rotate_vertex(tri.n[1], quat);
    vec3 n2 = rotate_vertex(tri.n[2], quat);
    
    vec3 v0v1 = v1 - v0;
    vec3 v0v2 = v2 - v0;
    vec3 norm = cross(v0v1, v0v2);
    
    vec3 v0o = ray.origin - v0;
    vec3 dv0o = cross(v0o, ray.dir);
    
    float det = -dot(ray.dir, norm);
    float inv_det = 1.0 / det;
    
    float dist = dot(v0o, norm) * inv_det;
    u = dot(v0v2, dv0o) * inv_det;
    v = -dot(v0v1, dv0o) * inv_det;
    w = 1 - u - v;
    
    info.hit = (det >= 1E-6 && dist >= 0 && u >= 0 && v >= 0 && w >= 0);
    info.dist = dist;
    info.point = ray.origin + ray.dir * dist;
    info.norm = normalize(norm);
    
    return info;
}

HitInfo
shoot_out_ray(Ray ray)
{
    HitInfo closest_info, info;
    closest_info.hit = false;
    closest_info.dist = 100000000.0;
    
    
    for(int i = 0; i < sphere_count; i++)
    {
        Sphere sphere = spheres[i];
        info = intersect_sphere(ray, sphere);
        
        if(info.hit && info.dist < closest_info.dist)
        {
            closest_info = info;
            closest_info.mat_id = sphere.mat_id;
        }
    }
    
    for(int i = 0; i < mesh_count; i++)
    {
        Mesh mesh = meshes[i];
        
        for(int j = 0; j < mesh.tri_count; j++)
        {
            Triangle tri = triangles[mesh.tri_idx+j];
            info = intersect_triangle(ray, tri,
                                      mesh.pos, mesh.quat, mesh.scale);
            
            if(info.hit && info.dist < closest_info.dist)
            {
                closest_info = info;
                closest_info.mat_id = mesh.mat_id;
            }
        }
    }
    
    /*Triangle tri;
    tri.v[0] = vec3(20.0f, 10.0f, 0.0f);
    tri.v[1] = vec3(20.0f, 0.0f, 0.0f);
    tri.v[2] = vec3(10.0f, 10.0f, 0.0f);
    tri.mat_id = 0;
    closest_info = intersect_triangle(ray, tri);*/
    
    return closest_info;
}

vec3
ray_trace(Ray ray, inout uint state)
{
    Material mat;
    HitInfo info;
    
    vec3 r_color = vec3(1.0, 1.0, 1.0);
    vec3 final_color = vec3(0.0, 0.0, 0.0);
    
    for(int i = 0; i < max_bounce; i++)
    {
        info = shoot_out_ray(ray);
        if(info.hit)
        {
            ray.origin = info.point;
            
            vec3 diffuse_dir = gen_random_hemisphere_dir(info.norm, state);
            vec3 specular_dir = reflect(ray.dir, info.norm);
            
            // NOTE(ajeej): The smoothness of the material determines
            // to what extent the lighting is specular or diffuse
            ray.dir = mix(diffuse_dir, specular_dir, mat.smoothness);
            
            mat = mats[info.mat_id];
            vec3 emission = mat.emission_color * mat.emission_strength;
            float light_strength = dot(info.norm, ray.dir);
            
            final_color += emission * r_color;
            r_color *= mat.color * light_strength;
            
            // NOTE(ajeej): 
            // This is what gives the ambient color, can only be done
            // when max_bounce == 1
            //final_color = mat.color;
            
            float p = max(r_color.x, max(r_color.y, r_color.z));
            if (gen_random_number(state) >= p) {
                break;
            }
            r_color *= 1.0f / p; 
        }
        else
        {
            // NOTE(ajeej): This applys ambient color after all the shading is
            // done. This is removed if only diffuse or specular ar desired.
            final_color += get_color_from_environment(ray) * r_color;
            break;
        }
    }
    
    return final_color;
}

void main() {
    ivec2 pixel_pos = ivec2(gl_GlobalInvocationID.xy);
    ivec2 screen_size = imageSize(texture);
    if (pixel_pos.x >= screen_size.x || pixel_pos.y >= screen_size.y) {
        return;
    }
    
    float x_comp = (2.0 * pixel_pos.x - screen_size.x)/screen_size.x;
    float y_comp = (2.0 * pixel_pos.y - screen_size.y)/screen_size.y;
    
    uint rand_state = pixel_pos.y * screen_size.x + pixel_pos.x + frame_count * 789235;
    
    Ray ray;
    
    ray.origin = camera_pos;
    ray.dir = normalize(-forward + x_comp*right + y_comp*up);
    
    vec3 total_color = vec3(0, 0, 0);
    
    for(int i = 0; i < 100; i++)
        total_color += ray_trace(ray, rand_state);
    
    vec3 color = total_color/100;
    imageStore(texture, pixel_pos, vec4(color, 1.0));
}
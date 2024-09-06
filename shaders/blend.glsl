#version 430

layout(local_size_x = 8, local_size_y = 8) in;
layout(rgba32f, binding = 0) uniform image2D old_image;
layout(rgba32f, binding = 1) uniform image2D new_image;

uniform uint frame_count;

void main() {
    ivec2 pixel_pos = ivec2(gl_GlobalInvocationID.xy);
    
    vec4 old_color = imageLoad(old_image, pixel_pos);
    vec4 new_color = imageLoad(new_image, pixel_pos);
    
    float w = 1.0 / (frame_count + 1);
    vec4 av = clamp((old_color * (1.0-w)) + (new_color * w), 0.0, 1.0);
    
    imageStore(old_image, pixel_pos, av);
}
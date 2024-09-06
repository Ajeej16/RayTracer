
#ifndef RAY_TRACER_H
#define RAY_TRACER_H

struct hit_info_t {
    vec3 enter_point, exit_point, norm;
    f32 dist;
    u32 mat_id;
    bool hit;
};

#endif //RAY_TRACER_H

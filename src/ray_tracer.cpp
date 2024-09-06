
static bool
solve_quadratic(f32 a, f32 b, f32 c,
                f32 *out_s0, f32 *out_s1)
{
    f32 disc = b*b - 4*a*c, q, s0, s1, temp;
    if (disc < 0)
        return false;
    else if (disc == 0) {
        s0 = s1 = -0.5f*b/a;
    }
    else {
        q = (b > 0) ? 
            -0.5f*(b+sqrt(disc)) : -0.5f*(b-sqrt(disc));
        s0 = q/a;
        s1 = c/q;
    }
    
    if(s0 > s1) { temp = s0; s0 = s1; s1 = temp; }
    
    *out_s0 = s0;
    *out_s1 = s1;
    
    return true;;
}

/*static hit_info_t
intersect_sphere(vec3 p, vec3 dir, sphere_t *s)
{
    hit_info_t result = {0};
    f32 a, b, c, t0, t1, temp;
    vec3 fs;
    
    glm_vec3_sub(p, s->pos, fs);
    
    a = glm_vec3_dot(dir, dir);
    b = 2 * glm_vec3_dot(dir, fs);
    c = glm_vec3_dot(fs, fs) - s->r*s->r;
    if (!solve_quadratic(a, b, c, &t0, &t1)) return result;
    
    if(t0 > t1) { temp = t0; t0 = t1; t1 = temp; }
    if(t0 < 0) {
        t0 = t1;
        if(t0 < 0) return result;
    }
    
    result.hit = true;
    result.dist = t0;
    
    glm_vec3_scale(dir, t0, result.enter_point);
    glm_vec3_add(result.enter_point, p, result.enter_point);
    glm_vec3_scale(dir, t1, result.exit_point);
    glm_vec3_add(result.exit_point, p, result.exit_point);
    
    glm_vec3_sub(result.enter_point, s->pos, result.norm);
    glm_vec3_normalize(result.norm);
    
    
    return result;
}*/

static hit_info_t
intersect_sphere(vec3 p, vec3 dir, sphere_t *s)
{
    hit_info_t result = {0};
    vec3 delta;
    glm_vec3_sub(p, s->pos, delta);
    
    f32 a = glm_vec3_dot(dir, dir);
    f32 b = 2 * glm_vec3_dot(delta, dir);
    f32 c = glm_vec3_dot(delta, delta) - s->r*s->r;
    
    f32 disc = b*b - 4*a*c;
    
    if (disc >= 0) {
        f32 dist = (-b - sqrt(disc)) / (2*a);
        
        if(dist >= 0) {
            result.hit = true;
            result.dist = dist;
            result.mat_id = s->mat_id;
            
            glm_vec3_scale(dir, dist, result.enter_point);
            glm_vec3_add(result.enter_point, p, result.enter_point);
            
            glm_vec3_sub(result.enter_point, s->pos, result.norm);
            glm_vec3_normalize(result.norm);
        }
    }
    
    return result;
}

static hit_info_t
get_ray_collision(vec3 p, vec3 dir, sphere_t *ss, u32 s_count)
{
    hit_info_t closest_info = {0};
    closest_info.dist = 10000000.0f;
    
    for(u32 i = 0; i < s_count; i++)
    {
        hit_info_t info = intersect_sphere(p, dir, ss+i);
        
        if (info.hit && info.dist < closest_info.dist) {
            closest_info = info;
            closest_info.mat_id = ss[i].mat_id;
        }
    }
    
    return closest_info;
}

static float
random_value(u32 *state)
{
    *state += (*state + 195439) * (*state + 124395) * (*state + 845921);
    return *state / 4294967295.0;
}

static float
random_normal_dist(u32 *state)
{
    float theta = 2*3.1415926 * random_value(state);
    float rho = sqrt(-2 * log(random_value(state)));
    return rho * cos(theta);
}

static void
random_direction(u32 * state, vec3 dir)
{
    for(int i = 0; i < 3; i++)
        dir[i] = random_normal_dist(state);
    
    glm_vec3_normalize(dir);
}

static void
random_hemisphere_direction(vec3 norm, u32 *state, vec3 dir)
{
    random_direction(state, dir);
    if(glm_vec3_dot(dir, norm) < 0)
        glm_vec3_negate(dir);
}

/*static void
get_rand_dir_on_hemisphere(vec3 norm, vec3 dir)
{
    
    for(u32 i = 0; i < 3; i++)
        dir[i] = dist(gen);
    
    glm_vec3_normalize(dir);
    
    if(glm_vec3_dot(dir, norm) < 0)
        glm_vec3_negate(dir);
}*/

static void
shoot_ray(scene_t *scene,
          vec3 r_origin, vec3 r_dir, u32 max_bounce,
          vec3 final_color, u32 *state)
{
    vec3 r_color = {1.0f, 1.0f, 1.0f}, emission, temp;
    vec3 origin, dir;
    glm_vec3_copy(r_origin, origin);
    glm_vec3_copy(r_dir, dir);
    glm_vec3_zero(final_color);
    material_t mat;
    
    for (int i = 0; i < 30; i++)
    {
        hit_info_t info = get_ray_collision(origin, dir, 
                                            scene->spheres, get_stack_count(scene->spheres));
        
        if (info.hit)
        {
            mat = scene->mats[info.mat_id];
            
            glm_vec3_copy(info.enter_point, origin);
            //get_rand_dir_on_hemisphere(info.norm, dir);
            random_hemisphere_direction(info.norm, state, dir);
            
            glm_vec3_scale(mat.emission_color, mat.emission_strength, emission);
            glm_vec3_mul(emission, r_color, temp);
            glm_vec3_add(final_color, temp, final_color);
            glm_vec3_mul(r_color, mat.rgb, r_color);
            
            //glm_vec3_copy(mat.rgb, final_color);
        }
        else
            break;
    }
    
    //glm_vec3_clamp(final_color, 0.0f, 1.0f);
}


static void
init_camera(camera_t *cam, vec3 pos, f32 np, f32 fp, 
            u32 px, u32 py, f32 pw, f32 ph,
            vec3 f, vec3 s)
{
    glm_vec3_copy(pos, cam->pos);
    glm_vec3_copy(f, cam->front);
    glm_vec3_copy(s, cam->side);
    glm_vec3_cross(s, f, cam->up);
    
    cam->pitch = 0;
    cam->yaw = -90;
    cam->near_plane = np;
    cam->far_plane = fp;
    
    cam->width = pow(2, px);
    cam->height = pow(2, py);
    
    cam->p_width = pw;
    cam->p_height = ph;
    
    cam->perspective = 1;
}

static void
init_sphere(sphere_t *s, vec3 pos, f32 r, u32 mat_id)
{
    glm_vec3_copy(pos, s->pos);
    s->r = r;
    s->mat_id = mat_id;
}

static void
init_material(material_t *mat, vec3 rgb, vec3 emission_color, f32 emission_strength, f32 smoothness)
{
    glm_vec3_copy(rgb, mat->rgb);
    glm_vec3_copy(emission_color, mat->emission_color);
    mat->emission_strength = emission_strength;
    mat->smoothness = smoothness;
}

static void
init_scene(scene_t *sc, render_settings_t settings)
{
    sc->spheres = NULL;
    sc->triangles = NULL;
    sc->mats = NULL;
    sc->meshes = NULL;
    sc->settings = settings;
    sc->sphere_buffer = 0;
    sc->mat_buffer = 0;
    sc->tri_buffer = 0;
    sc->mesh_buffer = 0;
    sc->moving = true;
    sc->clean_frame = true;
    sc->ambient = sc->diffuse = sc->specular = true;
}

static void
free_scene(scene_t *sc)
{
    if(sc->spheres)
        stack_free(sc->spheres);
    if(sc->triangles)
        stack_free(sc->triangles);
    if(sc->mats)
        stack_free(sc->mats);
    if(sc->meshes)
        stack_free(sc->meshes);
    
    glDeleteBuffers(1, &sc->sphere_buffer);
    glDeleteBuffers(1, &sc->mat_buffer);
    glDeleteBuffers(1, &sc->tri_buffer);
    glDeleteBuffers(1, &sc->mesh_buffer);
}

static void
add_sphere(scene_t *sc, vec3 pos, f32 r, u32 mat_id)
{
    sphere_t *s = (sphere_t *)stack_push(&sc->spheres);
    init_sphere(s, pos, r, mat_id);
}

static void
move_mesh(scene_t *sc, u32 mesh_id, vec3 delta)
{
    mesh_t *mesh = sc->meshes+mesh_id;
    glm_vec3_add(mesh->pos, delta, mesh->pos);
}

static void
set_mesh_pos(scene_t *sc, u32 mesh_id, vec3 pos)
{
    mesh_t *mesh = sc->meshes+mesh_id;
    glm_vec3_copy(pos, mesh->pos);
}

static void
rotate_mesh(scene_t *sc, u32 mesh_id, f32 angle, vec3 axis)
{
    mesh_t *mesh = sc->meshes+mesh_id;
    versor rotation;
    glm_quatv(rotation, angle, axis);
    glm_quat_mul(mesh->rot, rotation, mesh->rot);
}

static void
set_mesh_rot(scene_t *sc, u32 mesh_id, f32 angle, vec3 axis)
{
    mesh_t *mesh = sc->meshes+mesh_id;
    glm_quatv(mesh->rot, angle, axis);
}

static void
scale_mesh(scene_t *sc, u32 mesh_id, vec3 scale)
{
    mesh_t *mesh = sc->meshes+mesh_id;
    glm_vec3_copy(scale, mesh->scale);
}

static void
scale_mesh(scene_t *sc, u32 mesh_id, f32 scale)
{
    mesh_t *mesh = sc->meshes+mesh_id;
    mesh->scale[0] = scale;
    mesh->scale[1] = scale;
    mesh->scale[2] = scale;
}

static u32
add_mesh(scene_t *sc, 
         f32 *verts, u32 *indices, u32 idx_count, u32 mat_id)
{
    triangle_t *tri;
    u32 mesh_id = get_stack_count(sc->meshes);
    mesh_t *mesh = (mesh_t *)stack_push(&sc->meshes);
    u32 tri_count = idx_count/3;
    
    glm_vec3_zero(mesh->pos);
    glm_quat_identity(mesh->rot);
    glm_vec3_one(mesh->scale);
    
    mesh->tri_idx = get_stack_count(sc->triangles);
    mesh->tri_count = tri_count;
    mesh->mat_id = mat_id;
    
    for(u32 i = 0; i < tri_count; i++)
    {
        tri = (triangle_t *)stack_push(&sc->triangles);
        
        memcpy(tri->v0, verts+(*indices++)*3, 3*sizeof(f32));
        memcpy(tri->v1, verts+(*indices++)*3, 3*sizeof(f32));
        memcpy(tri->v2, verts+(*indices++)*3, 3*sizeof(f32));
    }
    
    return mesh_id;
}

static u32
add_plane(scene_t *sc, u32 mat_id, f32 w, f32 h)
{
    f32 verts[] = {
        -1, -1, 0,
        -1,  1, 0,
        1,  -1, 0,
        1,   1, 0
    };
    
    u32 indices[] = {
        0, 1, 2,
        1, 3, 2,
    };
    
    u32 id = add_mesh(sc, verts, indices, ARRAY_COUNT(indices), mat_id);
    scale_mesh(sc, id, vec3{w, h, 0.0f});
    
    return id;
}

static u32
add_material(scene_t *sc, vec3 rgb, vec3 emission_color, f32 emission_strength, f32 smoothness)
{
    u32 id = get_stack_count(sc->mats);
    material_t *mat = (material_t *)stack_push(&sc->mats);
    init_material(mat, rgb, emission_color, emission_strength, smoothness);
    
    return id;
}

static void
setup_scene(camera_t *cam, scene_t *sc, u32 compute_program)
{
    glGenBuffers(1, &sc->sphere_buffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, sc->sphere_buffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(sphere_t)*get_stack_count(sc->spheres), sc->spheres, GL_DYNAMIC_COPY);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, sc->sphere_buffer);
    
    glGenBuffers(1, &sc->mat_buffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, sc->mat_buffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(material_t)*get_stack_count(sc->mats), sc->mats, GL_DYNAMIC_COPY);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, sc->mat_buffer);
    
    glGenBuffers(1, &sc->tri_buffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, sc->tri_buffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(triangle_t)*get_stack_count(sc->triangles), sc->triangles, GL_DYNAMIC_COPY);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, sc->tri_buffer);
    
    glGenBuffers(1, &sc->mesh_buffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, sc->mesh_buffer);
    
    glUseProgram(compute_program);
    glUniform1ui(glGetUniformLocation(compute_program, "perspective"), cam->perspective);
    glUniform1ui(glGetUniformLocation(compute_program, "sphere_count"), get_stack_count(sc->spheres));
    glUniform1ui(glGetUniformLocation(compute_program, "mesh_count"), get_stack_count(sc->meshes));
    glUniform1ui(glGetUniformLocation(compute_program, "max_bounce"), sc->settings.max_bounce);
    
    glUniform3f(glGetUniformLocation(compute_program, "horizon_color"), 
                sc->settings.horizon_color[0], sc->settings.horizon_color[1], sc->settings.horizon_color[2]);
    glUniform3f(glGetUniformLocation(compute_program, "zenith_color"),
                sc->settings.zenith_color[0], sc->settings.zenith_color[1], sc->settings.zenith_color[2]);
    glUniform3f(glGetUniformLocation(compute_program, "ground_color"), 
                sc->settings.ground_color[0], sc->settings.ground_color[1], sc->settings.ground_color[2]);
    
    glUseProgram(0);
}

static void
render_frame(camera_t *cam, scene_t *sc, u32 compute_program,
             u32 texture, u64 frame_id)
{
    glUseProgram(compute_program);
    
    glUniform1ui(glGetUniformLocation(compute_program, "perspective"), cam->perspective);
    glUniform3f(glGetUniformLocation(compute_program, "camera_pos"), cam->pos[0], cam->pos[1], cam->pos[2]);
    glUniform3f(glGetUniformLocation(compute_program, "forward"), cam->front[0], cam->front[1], cam->front[2]);
    glUniform3f(glGetUniformLocation(compute_program, "right"), cam->side[0], cam->side[1], cam->side[2]);
    glUniform3f(glGetUniformLocation(compute_program, "up"), cam->up[0], cam->up[1], cam->up[2]);
    
    glUniform1ui(glGetUniformLocation(compute_program, "frame_count"), frame_id);
    
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(mesh_t)*get_stack_count(sc->meshes), sc->meshes, GL_DYNAMIC_COPY);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, sc->mesh_buffer);
    
    glBindImageTexture(0, texture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
    glDispatchCompute(cam->width/8, cam->height/8, 1);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    
    glUseProgram(0);
}

static void
render_scene(camera_t *cam, scene_t *sc, u32 compute_program, u32 blend_program,
             u32 texture, u32 new_texture, u64 frame_id)
{
    glUseProgram(compute_program);
    
    glUniform1ui(glGetUniformLocation(compute_program, "perspective"), cam->perspective);
    glUniform3f(glGetUniformLocation(compute_program, "camera_pos"), cam->pos[0], cam->pos[1], cam->pos[2]);
    glUniform3f(glGetUniformLocation(compute_program, "forward"), cam->front[0], cam->front[1], cam->front[2]);
    glUniform3f(glGetUniformLocation(compute_program, "right"), cam->side[0], cam->side[1], cam->side[2]);
    glUniform3f(glGetUniformLocation(compute_program, "up"), cam->up[0], cam->up[1], cam->up[2]);
    
    if(sc->moving)
    {
        glUniform1ui(glGetUniformLocation(compute_program, "frame_count"), frame_id);
        
        glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(mesh_t)*get_stack_count(sc->meshes), sc->meshes, GL_DYNAMIC_COPY);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, sc->mesh_buffer);
        
        glBindImageTexture(0, texture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
        glDispatchCompute(cam->width/8, cam->height/8, 1);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    }
    else
    {
        glUniform1ui(glGetUniformLocation(compute_program, "frame_count"), frame_id);
        
        glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(mesh_t)*get_stack_count(sc->meshes), sc->meshes, GL_DYNAMIC_COPY);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, sc->mesh_buffer);
        
        glBindImageTexture(0, new_texture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
        glDispatchCompute(cam->width/8, cam->height/8, 1);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
        
        glUseProgram(0);
        
        glUseProgram(blend_program);
        glUniform1ui(glGetUniformLocation(blend_program, "frame_count"), frame_id);
        glBindImageTexture(0, texture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
        glBindImageTexture(1, new_texture, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);
        glDispatchCompute(cam->width/8, cam->height/8, 1);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    }
    
    glUseProgram(0);
}

/*static void
blend_images(u8 *base, u8 *over, u32 size, f32 w)
{
    u32 av;
    
    for(u32 i = 0; i < size; i++) {
        if(*base == *over) {
            *base++; *over++;
        }
        else {
            av = (u32)*base * (1-w) + (u32)*over++ * w;
            *base++ = (u8)MAX(MIN(av, 255), 0);
        }
    }
}

static void
render_scene(camera_t *cam, scene_t *scene, u8 **out_img, u64 frame_id)
{
    vec3 o, p, temp, step_x, step_y, p_first, dir;
    f32 dx, dy;
    u32 x, y, i, j;
    u8 *img = (u8 *)malloc(cam->width*cam->height*3), *iter;
    memset(img, 0, cam->width*cam->height*3);
    
    sphere_t *objs = scene->spheres;
    
    // NOTE(ajeej): calculate position of first pixel
    glm_vec3_scale(cam->front, cam->near_plane, o);
    glm_vec3_add(o, cam->pos, o);
    
    dx = (1.0f - cam->width)*0.5f*cam->p_width;
    dy = -(1.0f - cam->height)*0.5f*cam->p_height;
    glm_vec3_scale(cam->side, dx, temp);
    glm_vec3_scale(cam->up, dy, p);
    glm_vec3_addadd(o, temp, p);
    
    // NOTE(ajeej): Generate step vectors
    glm_vec3_scale(cam->side, cam->p_width, step_x);
    glm_vec3_scale(cam->up, -cam->p_height, step_y);
    
    // NOTE(ajeej): Iterate through screen
    iter= img;
    for(y = 0; y < cam->height; y++)
    {
        glm_vec3_copy(p, p_first);
        
        for(x = 0; x < cam->width; x++)
        {
            glm_vec3_sub(p, cam->pos, dir);
            glm_vec3_normalize(dir);
            
            u32 state = (cam->width*y+x) * frame_id * 793123;
            
            
            vec3 color = {0.0f, 0.0f, 0.0f};
            vec3 total_color = {0.0f, 0.0f, 0.0f};
            u32 rays_per_pixel = 100;
            
            for(int i = 0; i < rays_per_pixel; i++) {
                shoot_ray(scene, p, dir, 30, color, &state);
                glm_vec3_add(total_color, color, total_color);
            }
            
            glm_vec3_scale(total_color, 1.0f/rays_per_pixel, color);
            glm_vec3_clamp(color, 0.0f, 1.0f);
            
            for(j = 0; j < 3; j++)
                *(iter++) = color[j]*255;
            
            glm_vec3_add(p, step_x, p);
        }
        
        glm_vec3_copy(p_first, p);
        glm_vec3_add(p, step_y, p);
    }
    
    *out_img = img;
}*/
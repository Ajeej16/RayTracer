
// Based on templates from learnopengl.com
#include <GL/glew.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <cglm/cglm.h>

#include <opencv2/opencv.hpp>


#include <iostream>
#include <string>

#include <cstdio>
#include <cstdlib>
#include <ctime>

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 800;

const char *vert_filename = "vert.glsl";
const char *frag_filename = "frag.glsl";
const char *compute_filename = "ray_tracer.glsl";

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef float f32;
typedef double f64;

#define ARRAY_COUNT(x) (sizeof((x))/sizeof((x)[0]))

#include "stack.h"

#include "ray_tracer.h"
#include "renderer.h"

#include "timer.h"
#include "video.h"

struct callback_data_t {
    f64 m_last_x, m_last_y;
    f32 m_sensitivity;
    bool m_first;
    
    camera_t *cam;
    bool *moving;
};


void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow *window, camera_t *cam, video_info_t *v_info, scene_t *sc);

static bool centering_mouse = false;

#include "shader.cpp"

// NOTE(ajeej): the software raytracer is no longer being used
#include "ray_tracer.cpp"
#include "renderer.cpp"

void
mouse_callback(GLFWwindow *window, double x, double y)
{
    callback_data_t *data = (callback_data_t *)glfwGetWindowUserPointer(window);
    camera_t *cam = data->cam;
    
    // TODO(ajeej): when moving or playing
    if(!*data->moving)
        return;
    
    if(data->m_first) {
        data->m_last_x = x;
        data->m_last_y = y;
        data->m_first = 0;
    }
    
    f64 last_x = data->m_last_x;
    f64 last_y = data->m_last_y;
    
    f32 offset_x = (x - last_x)*data->m_sensitivity;
    f32 offset_y = (last_y - y)*data->m_sensitivity;
    
    cam->yaw -= offset_x;
    cam->pitch -= offset_y;
    
    if(cam->pitch > 89.0f)
        cam->pitch = 89.0f;
    if(cam->pitch < -89.0f)
        cam->pitch = -89.0f;
    
    vec3 dir;
    dir[0] = cos(glm_rad(cam->yaw))*cos(glm_rad(cam->pitch));
    dir[1] = sin(glm_rad(cam->pitch));
    dir[2] = sin(glm_rad(cam->yaw)) * cos(glm_rad(cam->pitch));
    glm_normalize_to(dir, cam->front);
    
    glm_vec3_cross(cam->front, vec3{0, 1, 0}, cam->side);
    glm_vec3_normalize(cam->side);
    glm_vec3_cross(cam->side, cam->front, cam->up);
    glm_vec3_normalize(cam->up);
    
    data->m_last_x = x;
    data->m_last_y = y;
}

int main()
{
    srand(time(NULL));
    
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    
    // glfw window creation
    // --------------------
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Display RGB Array", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    
    // // GLEW: load all OpenGL function pointers
    glewInit();
    
    char *vert_src = load_shader_source(vert_filename);
    char *frag_src = load_shader_source(frag_filename);
    char *compute_src = load_shader_source(compute_filename);
    char *blend_src = load_shader_source("blend.glsl");
    u32 shader_program = create_shader(vert_src, frag_src);
    u32 compute_program = create_compute_shader(compute_src);
    u32 blend_program = create_compute_shader(blend_src);
    free(vert_src);
    free(frag_src);
    free(compute_src);
    free(blend_src);
    
    // set up vertex data (and buffer(s)) and configure vertex attributes
    // ------------------------------------------------------------------
    float vertices[] = {
        // positions          // colors           // texture coords
        1.0f,  1.0f, 0.0f,   1.0f, 0.0f, 0.0f,   1.0f, 1.0f, // top right
        1.0f, -1.0f, 0.0f,   0.0f, 1.0f, 0.0f,   1.0f, 0.0f, // bottom right
        -1.0f, -1.0f, 0.0f,   0.0f, 0.0f, 1.0f,   0.0f, 0.0f, // bottom left
        -1.0f,  1.0f, 0.0f,   1.0f, 1.0f, 0.0f,   0.0f, 1.0f  // top left 
    };
    unsigned int indices[] = {  
        0, 1, 3, // first triangle
        1, 2, 3  // second triangle
    };
    unsigned int VBO, VAO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    
    glBindVertexArray(VAO);
    
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    
    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // color attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    // texture coord attribute
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    
    int alignment = 0;
    glGetIntegerv(GL_SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT, &alignment);
    
    
    u64 frame_id = 1;
    camera_t cam;
    scene_t scene;
    init_camera(&cam, vec3{0, 5, -20}, 10.0f, 15.0f, 9, 9, 0.1f, 0.1f, vec3{0.0f, 0.0f, -1.0f}, vec3{1.0f, 0.0f, 0.0f});
    
    render_settings_t setting = {0}; {
        setting.max_bounce = 30;
        glm_vec3_copy(vec3{1, 1, 1}, setting.horizon_color);
        glm_vec3_copy(vec3{0.08, 0.36, 0.7}, setting.zenith_color);
        glm_vec3_copy(vec3{0.35, 0.35, 0.35}, setting.ground_color);
    }
    init_scene(&scene, setting);
    
    // TODO(ajeej): create a function for each of these things
    u32 texture, new_texture, clear_texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture); 
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA32F, cam.width, cam.height);
    
    glGenTextures(1, &new_texture);
    glBindTexture(GL_TEXTURE_2D, new_texture); 
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA32F, cam.width, cam.height);
    
    glGenTextures(1, &clear_texture);
    glBindTexture(GL_TEXTURE_2D, new_texture); 
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    
    f32 vs[] = {
        0.0f, 1.0f, 0.0f,
        0.5f, 0.0f, -0.5f,
        -0.5f, 0.0f, -0.5f,
        0.0f, 0.0f, 0.5f,
    };
    
    u32 is[] = {
        0, 1, 2,
        0, 2, 3,
        0, 3, 1,
        1, 3, 2,
    };
    
    u32 mirror = add_material(&scene, vec3{1.0f, 1.0f, 1.0f,}, vec3{0.0f, 0.0f, 0.0f}, 0.0f, 1.0f);
    u32 light = add_material(&scene, vec3{0.0f, 0.0f, 0.0f}, vec3{1.0f, 1.0f, 1.0f}, 60.0f, 0.0f);
    u32 red = add_material(&scene, vec3{0.82f, 0.25f, 0.28f}, vec3{0.0f, 0.0f, 0.0f}, 0.0f, 0.2f);
    u32 blue = add_material(&scene, vec3{0.0f, 0.0f, 0.98f}, vec3{0.0f, 0.0f, 0.0f}, 0.0f, 0.4f);
    u32 grey = add_material(&scene, vec3{0.67f, 0.67f, 0.67f}, vec3{0.0f, 0.0, 0.0f}, 0.0f, 0.3f);
    
    u32 plane = add_plane(&scene, grey, 200.0f, 200.0f);
    rotate_mesh(&scene, plane, glm_rad(90.0f), vec3{1, 0, 0});
    
    
    add_sphere(&scene, vec3{1000.0f, 500.0f, 0.0f}, 200.0f, light);
    add_sphere(&scene, vec3{-20.0f, 5.0f, -5.0f}, 5.0f, red);
    u32 tetra = add_mesh(&scene, vs, is, ARRAY_COUNT(is), blue);
    set_mesh_pos(&scene, tetra, vec3{15.0f, 0.0f, -10.0f});
    scale_mesh(&scene, tetra, 15.0f);
    
    add_sphere(&scene, vec3{0.0f, 10.0f, 20.0f}, 10.0f, mirror);
    
    setup_scene(&cam, &scene, compute_program);
    
    
    timer_t timer;
    video_info_t v_info;
    
    init_timer(&timer);
    init_video_info(&v_info, 20, 5);
    
    callback_data_t data = {0};
    data.m_sensitivity = 0.7f;
    data.cam = &cam;
    data.moving = &scene.moving;
    glfwSetWindowUserPointer(window, &data);
    glfwSetCursorPosCallback(window, mouse_callback);
    
    scene.moving = false;
    
    // render loop
    // -----------
    while (!glfwWindowShouldClose(window))
    {
        start_timer(&timer);
        
        processInput(window, &cam, &v_info, &scene);
        
        static bool m_pressed = false;
        if(glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS && !m_pressed) {
            m_pressed = true;
        }
        else if(glfwGetKey(window, GLFW_KEY_M) == GLFW_RELEASE && m_pressed) {
            m_pressed = false;
            scene.moving = !scene.moving;
            
            frame_id = 1;
            
            render_frame(&cam, &scene, compute_program, texture, frame_id);
        }
        
        
        if(v_info.is_uploading && get_stack_count(v_info.pos)) {
            char *path = "video.mp4";
            cv::VideoWriter video_writer(path, cv::VideoWriter::fourcc('X', '2', '6', '4'),
                                         v_info.frames_per_second, cv::Size(cam.width, cam.height));
            
            if(!video_writer.isOpened()) {
                std::cout << "Could not open the output video file." << std::endl;
                continue;
            }
            
            scene.moving = false;
            timer_t render_delay;
            init_timer(&render_delay);
            start_timer(&render_delay);
            
            for(int i = 0; i < get_stack_count(v_info.pos)/3; i++) {
                reset_timer(&render_delay);
                glm_vec3_copy(v_info.pos+i*3, cam.pos);
                glm_vec3_copy(v_info.front+i*3, cam.front);
                glm_vec3_copy(v_info.side+i*3, cam.side);
                glm_vec3_cross(cam.side, cam.front, cam.up);
                
                frame_id = 1;
                render_frame(&cam, &scene, compute_program, texture, frame_id++);
                
                u64 nanos_elapsed = check_timer(&render_delay);
                while(nanos_elapsed < v_info.seconds_per_render * 1E9) {
                    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
                    glClear(GL_COLOR_BUFFER_BIT);
                    
                    render_scene(&cam, &scene, compute_program, blend_program, texture, new_texture, frame_id++);
                    
                    
                    glBindTexture(GL_TEXTURE_2D, texture);
                    
                    glUseProgram(shader_program);
                    glBindVertexArray(VAO);
                    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
                    
                    glfwSwapBuffers(window);
                    glfwPollEvents();
                    
                    nanos_elapsed = check_timer(&render_delay);
                }
                
                cv::Mat frame(cam.width, cam.height, CV_8UC3);
                glBindTexture(GL_TEXTURE_2D, texture);
                glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_UNSIGNED_BYTE, frame.data);
                
                cv::flip(frame, frame, 0);
                cv::cvtColor(frame, frame, cv::COLOR_RGB2BGR);
                
                video_writer.write(frame);
            }
            end_timer(&render_delay);
            
            video_writer.release();
            
            v_info.is_uploading = false;
        }
        
        
        
        if(v_info.is_playing) {
            glm_vec3_copy(v_info.pos+v_info.play_idx*3, cam.pos);
            glm_vec3_copy(v_info.front+v_info.play_idx*3, cam.front);
            glm_vec3_copy(v_info.side+v_info.play_idx*3, cam.side);
            if(v_info.play_idx == get_stack_count(v_info.pos)/3 - 1)
                v_info.play_idx = 0;
            else
                v_info.play_idx++;
        }
        
        render_scene(&cam, &scene, compute_program, blend_program, texture, new_texture, frame_id++);
        
        u64 ne = check_timer(&timer);
        if(ne >= 1/v_info.frames_per_second * 1E9 && v_info.is_recording)
        {
            reset_timer(&timer);
            
            f32 *pos = (f32 *)stack_push_array(&v_info.pos, 3);
            f32 *front = (f32 *)stack_push_array(&v_info.front, 3);
            f32 *side = (f32 *)stack_push_array(&v_info.side, 3);
            glm_vec3_copy(cam.pos, pos);
            glm_vec3_copy(cam.front, front);
            glm_vec3_copy(cam.side, side);
        }
        
        // render
        // ------
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        
        // bind Texture
        glBindTexture(GL_TEXTURE_2D, texture);
        
        // render container
        glUseProgram(shader_program);
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        
        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
        
        end_timer(&timer);
    }
    
    free_scene(&scene);
    
    // optional: de-allocate all resources once they've outlived their purpose:
    // ------------------------------------------------------------------------
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    
    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window, camera_t *cam,
                  video_info_t *v_info, scene_t *sc)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    
    if(sc->moving) {
        
        if(glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
            vec3 dir;
            glm_vec3_scale(cam->front, -1.0f, dir);
            glm_vec3_add(cam->pos, dir, cam->pos);
        }
        
        if(glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
            vec3 dir;
            glm_vec3_scale(cam->front, 1.0f, dir);
            glm_vec3_add(cam->pos, dir, cam->pos);
        }
        
        if(glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
            vec3 dir;
            glm_vec3_scale(cam->side, -1.0f, dir);
            glm_vec3_add(cam->pos, dir, cam->pos);
        }
        
        if(glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
            vec3 dir;
            glm_vec3_scale(cam->side, 1.0f, dir);
            glm_vec3_add(cam->pos, dir, cam->pos);
        }
        
        if(glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
            vec3 dir;
            glm_vec3_scale(cam->up, 1.0f, dir);
            glm_vec3_add(cam->pos, dir, cam->pos);
        }
        
        if(glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) {
            vec3 dir;
            glm_vec3_scale(cam->up, -1.0f, dir);
            glm_vec3_add(cam->pos, dir, cam->pos);
        }
        
    }
    
    static bool r_pressed = false;
    if(glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS && !r_pressed) {
        r_pressed = true;
    }
    else if(glfwGetKey(window, GLFW_KEY_R) == GLFW_RELEASE && r_pressed) {
        r_pressed = false;
        v_info->is_recording = !v_info->is_recording;
        if(v_info->is_recording == true)
            stack_clear(v_info->pos);
    }
    
    static bool p_pressed = false;
    if(glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS && !p_pressed) {
        p_pressed = true;
    }
    else if(glfwGetKey(window, GLFW_KEY_P) == GLFW_RELEASE && p_pressed) {
        p_pressed = false;
        v_info->is_playing = !v_info->is_playing;
        v_info->play_idx = 0;
    }
    
    static bool v_pressed = false;
    if(glfwGetKey(window, GLFW_KEY_V) == GLFW_PRESS && !v_pressed) {
        v_pressed = true;
    }
    else if(glfwGetKey(window, GLFW_KEY_V) == GLFW_RELEASE && v_pressed) {
        v_pressed = false;
        v_info->is_uploading = !v_info->is_uploading;
    }
    
    static bool o_pressed = false;
    if(glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS && !o_pressed) {
        o_pressed = true;
    }
    else if(glfwGetKey(window, GLFW_KEY_O) == GLFW_RELEASE && o_pressed) {
        o_pressed = false;
        cam->perspective = !cam->perspective;
    }
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

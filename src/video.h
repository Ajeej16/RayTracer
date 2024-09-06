
#ifndef VIDEO_H
#define VIDEO_H

struct video_info_t {
    u32 frames_per_second;
    u32 nanos_per_frame;
    
    u32 seconds_per_render;
    
    u32 play_idx;
    bool is_recording, is_playing, is_uploading;
    
    STACK(f32) *pos;
    STACK(f32) *front;
    STACK(f32) *side;
};

static void
init_video_info(video_info_t *info, u32 fps, u32 spf)
{
    info->frames_per_second = fps;
    info->nanos_per_frame = 1E9/fps;
    info->seconds_per_render = spf;
    info->play_idx = 0;
    info->is_recording = false;
    info->is_playing = false;
    info->is_uploading = false;
    info->pos = NULL;
    info->front = NULL;
    info->side = NULL;
}

#endif //VIDEO_H

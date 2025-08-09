#include "dr_cursor_tracker.h"
#include <windows.h>
#include <util/platform.h>
#include <graphics/graphics.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <graphics/image-file.h>
#include <math.h>

// forward declarations for local texture creators
static gs_texture_t *create_solid_circle_texture(int radius, uint32_t color, float alpha);
static gs_texture_t *create_white_circle_texture(int radius);

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

#define BLOG_PREFIX "[crosshair_box] "

// 內建 Tint Effect（當 Default/Draw 無法設定顏色時的後備方案）
static const char *g_tint_effect_src =
    "uniform texture2d image;\n"
    "uniform float4 color;\n"
    "sampler_state texSampler { Filter = Linear; AddressU = Clamp; AddressV = Clamp; };\n"
    "struct VertIn { float4 pos : POSITION; float2 uv : TEXCOORD0; };\n"
    "struct VertOut { float4 pos : POSITION; float2 uv : TEXCOORD0; };\n"
    "VertOut VS(VertIn v) { VertOut o; o.pos = v.pos; o.uv = v.uv; return o; }\n"
    "float4 PS(VertOut v) : TARGET { float4 c = image.Sample(texSampler, v.uv); return c * color; }\n"
    "technique DrawTint { pass { vertex_shader = VS(v); pixel_shader = PS(v); } }\n";
static gs_effect_t *g_tint_effect = NULL;
static gs_eparam_t *g_tint_image_param = NULL;
static gs_eparam_t *g_tint_color_param = NULL;

// 創建PNG紋理的輔助函數
static gs_texture_t *create_crosshair_texture(const char *path)
{
    if (!path || strlen(path) == 0) {
        blog(LOG_WARNING, BLOG_PREFIX "路徑為空或無效");
        return NULL;
    }
    
    // 使用 gs_image_file4_t 載入圖片（使用直接Alpha通道）
    gs_image_file4_t image4;
    gs_image_file4_init(&image4, path, GS_IMAGE_ALPHA_STRAIGHT);
    
    if (!image4.image3.image2.image.loaded) {
        blog(LOG_WARNING, BLOG_PREFIX "無法載入圖片: %s", path);
        gs_image_file4_free(&image4);
        return NULL;
    }
    
    // 在圖形上下文中初始化紋理
    obs_enter_graphics();
    gs_image_file4_init_texture(&image4);
    obs_leave_graphics();
    
    if (!image4.image3.image2.image.texture) {
        blog(LOG_WARNING, BLOG_PREFIX "無法初始化紋理");
        gs_image_file4_free(&image4);
        return NULL;
    }
    
    /* 成功載入圖片：不輸出資訊日誌以降低噪音 */
    
    // 複製紋理以避免被銷毀
    gs_texture_t *texture = image4.image3.image2.image.texture;
    image4.image3.image2.image.texture = NULL; // 防止被 gs_image_file4_free 銷毀
    gs_image_file4_free(&image4);
    return texture;
}

static const char *crosshair_box_get_name(void *unused)
{
    UNUSED_PARAMETER(unused);
    return obs_module_text("CrosshairTracker");
}

// 添加顏色轉換函數
static uint32_t obs_color_to_uint32(uint32_t obs_color)
{
    // OBS 顏色格式: RGBA
    // 我們需要轉換為 ARGB 格式
    uint8_t r = (obs_color >> 0) & 0xFF;
    uint8_t g = (obs_color >> 8) & 0xFF;
    uint8_t b = (obs_color >> 16) & 0xFF;
    uint8_t a = (obs_color >> 24) & 0xFF;
    
    // 轉換為 ARGB 格式
    return (a << 24) | (r << 16) | (g << 8) | b;
}

static uint32_t uint32_to_obs_color(uint32_t color)
{
    // ARGB 格式轉換為 RGBA 格式
    uint8_t a = (color >> 24) & 0xFF;
    uint8_t r = (color >> 16) & 0xFF;
    uint8_t g = (color >> 8) & 0xFF;
    uint8_t b = (color >> 0) & 0xFF;
    
    // 轉換為 RGBA 格式
    return (r << 0) | (g << 8) | (b << 16) | (a << 24);
}

static void *crosshair_box_create(obs_data_t *settings, obs_source_t *source)
{
    struct dr_cursor_tracker_data *data = bzalloc(sizeof(struct dr_cursor_tracker_data));
    data->mode = (enum crosshair_mode)obs_data_get_int(settings, "crosshair_mode");
    data->box_size = (int)obs_data_get_int(settings, "box_size");
    
    // 如果尚未建立後備效果，嘗試建立一次
    if (!g_tint_effect) {
        obs_enter_graphics();
        g_tint_effect = gs_effect_create(g_tint_effect_src, "DRTintEffect", NULL);
        if (g_tint_effect) {
            g_tint_image_param = gs_effect_get_param_by_name(g_tint_effect, "image");
            g_tint_color_param = gs_effect_get_param_by_name(g_tint_effect, "color");
        } else {
            /* 後備效果初始化失敗，僅記憶體內處理，無需日誌 */
        }
        obs_leave_graphics();
    }
    
    // 使用正確的顏色轉換
    uint32_t obs_box_color = (uint32_t)obs_data_get_int(settings, "box_color");
    data->box_color = obs_color_to_uint32(obs_box_color);
    
    data->box_thickness = (int)obs_data_get_int(settings, "box_thickness");
    {
        bool show_box_init = obs_data_get_bool(settings, "show_box");
        double box_alpha_val = obs_data_get_double(settings, "box_alpha");
        data->box_alpha = show_box_init ? (float)box_alpha_val : 0.0f;
    }
    data->show_default_crosshair = obs_data_get_bool(settings, "show_default_crosshair");
    
    // 使用正確的顏色轉換
    uint32_t obs_crosshair_color = (uint32_t)obs_data_get_int(settings, "crosshair_color");
    data->crosshair_color = obs_color_to_uint32(obs_crosshair_color);
    
    data->crosshair_thickness = (int)obs_data_get_int(settings, "crosshair_thickness");
    data->crosshair_alpha = (float)obs_data_get_double(settings, "crosshair_alpha");
    data->crosshair_size = (int)obs_data_get_int(settings, "crosshair_size");
    
    // 圓圈設定
    uint32_t obs_circle_color = (uint32_t)obs_data_get_int(settings, "circle_color");
    data->circle_color = obs_color_to_uint32(obs_circle_color);
    data->circle_thickness = (int)obs_data_get_int(settings, "circle_thickness");
    data->circle_alpha = (float)obs_data_get_double(settings, "circle_alpha");
    data->circle_radius = (int)obs_data_get_int(settings, "circle_radius");
    
    data->recenter_speed = (float)obs_data_get_double(settings, "recenter_speed");
    data->recenter_speed_center = (float)obs_data_get_double(settings, "recenter_speed_center");
    data->recenter_speed_edge = (float)obs_data_get_double(settings, "recenter_speed_edge");
    data->crosshair_move_speed_center = (float)obs_data_get_double(settings, "crosshair_move_speed_center");
    data->crosshair_move_speed_edge = (float)obs_data_get_double(settings, "crosshair_move_speed_edge");
    data->sensitivity = (float)obs_data_get_double(settings, "sensitivity");
    data->max_offset = (int)obs_data_get_int(settings, "max_offset");
    data->enable_idle_recenter = obs_data_get_bool(settings, "enable_idle_recenter");
    data->idle_recenter_delay = (float)obs_data_get_double(settings, "idle_recenter_delay");
    data->idle_recenter_time = (float)obs_data_get_double(settings, "idle_recenter_time");
    data->idle_recenter_boost = (float)obs_data_get_double(settings, "idle_recenter_boost");
    data->current_idle_time = 0.0f;
    data->current_recenter_speed = data->recenter_speed_center;
    data->is_mouse_moving = false;
    data->crosshair_path = bstrdup(obs_data_get_string(settings, "crosshair_path"));
    data->offset_x = 0.0f;
    data->offset_y = 0.0f;
    data->last_mouse_x = 0;
    data->last_mouse_y = 0;
    data->last_update_time = os_gettime_ns();
    data->source = source; // 保存源指針
    
    // 初始化圓形紋理
    data->circle_texture = NULL;
    data->circle_texture_size = 0;
    data->circle_texture_color = 0;
    data->circle_texture_thickness = 0;
    data->circle_texture_alpha = 0.0f;
    
    // 初始化追蹤線設定
    data->show_tracking_line = obs_data_get_bool(settings, "show_tracking_line");
    data->tracking_line_mode = (enum tracking_line_mode)obs_data_get_int(settings, "tracking_line_mode");
    data->tracking_line_color = obs_color_to_uint32((uint32_t)obs_data_get_int(settings, "tracking_line_color"));
    data->tracking_line_thickness = (int)obs_data_get_int(settings, "tracking_line_thickness");
    data->tracking_line_alpha = (float)obs_data_get_double(settings, "tracking_line_alpha");
    
    // 初始化路徑追蹤設定
    data->path_circle_radius = (float)obs_data_get_int(settings, "path_circle_radius");
    data->path_lifetime = (float)obs_data_get_double(settings, "path_lifetime");
    data->path_generation_interval = (float)obs_data_get_double(settings, "path_generation_interval");
    data->path_circle_color = obs_color_to_uint32((uint32_t)obs_data_get_int(settings, "path_circle_color"));
    data->path_head = NULL;
    data->path_tail = NULL;
    data->path_point_count = 0;
    data->path_circle_texture = NULL;
    data->path_circle_texture_size = 0;
    data->path_circle_texture_thickness = 0;
    data->path_circle_texture_alpha = 0.0f;
    data->last_path_time = 0;
    data->path_generation_interval = 20.0f; // 距離間隔：20像素

    // 初始化預生成 alpha 紋理快取
    for (int i = 0; i < 21; ++i) data->path_alpha_textures[i] = NULL;
    data->path_alpha_texture_count = 21;
    data->path_alpha_radius_cache = -1;
    data->path_alpha_thickness_cache = -1;
    data->path_alpha_color_cache = 0;
    
    // 初始化自訂圖片紋理
    data->custom_image_texture = NULL;
    data->custom_image_texture_size = 0;
    
    return data;
}

static void crosshair_box_destroy(void *data)
{
    struct dr_cursor_tracker_data *d = data;
    if (d->crosshair_path) {
        bfree(d->crosshair_path);
        d->crosshair_path = NULL;
    }
    if (d->circle_texture) {
        gs_texture_destroy(d->circle_texture);
        d->circle_texture = NULL;
    }
    if (d->custom_image_texture) {
        gs_texture_destroy(d->custom_image_texture);
        d->custom_image_texture = NULL;
    }
    if (d->path_circle_texture) {
        gs_texture_destroy(d->path_circle_texture);
        d->path_circle_texture = NULL;
    }
    // 釋放預生成 alpha 紋理
    for (int i = 0; i < d->path_alpha_texture_count; ++i) {
        if (d->path_alpha_textures[i]) {
            gs_texture_destroy(d->path_alpha_textures[i]);
            d->path_alpha_textures[i] = NULL;
        }
    }
    
    // 嘗試釋放後備 tint effect（僅在存在時）
    if (g_tint_effect) {
        obs_enter_graphics();
        gs_effect_destroy(g_tint_effect);
        g_tint_effect = NULL;
        g_tint_image_param = NULL;
        g_tint_color_param = NULL;
        obs_leave_graphics();
    }
    
    // 清理路徑點鏈表
    struct path_point *current = d->path_head;
    while (current) {
        struct path_point *next = current->next;
        bfree(current);
        current = next;
    }
    d->path_head = NULL;
    d->path_tail = NULL;
    d->path_point_count = 0;
    
    bfree(data);
}

static void crosshair_box_update(void *data, obs_data_t *settings)
{
    struct dr_cursor_tracker_data *d = data;
    d->mode = (enum crosshair_mode)obs_data_get_int(settings, "crosshair_mode");
    d->box_size = (int)obs_data_get_int(settings, "box_size");
    
    // 使用正確的顏色轉換
    uint32_t obs_box_color = (uint32_t)obs_data_get_int(settings, "box_color");
    d->box_color = obs_color_to_uint32(obs_box_color);
    
    d->box_thickness = (int)obs_data_get_int(settings, "box_thickness");
    {
        bool show_box_upd = obs_data_get_bool(settings, "show_box");
        double box_alpha_val = obs_data_get_double(settings, "box_alpha");
        d->box_alpha = show_box_upd ? (float)box_alpha_val : 0.0f;
    }
    d->show_default_crosshair = obs_data_get_bool(settings, "show_default_crosshair");
    
    // 使用正確的顏色轉換
    uint32_t obs_crosshair_color = (uint32_t)obs_data_get_int(settings, "crosshair_color");
    d->crosshair_color = obs_color_to_uint32(obs_crosshair_color);
    
    d->crosshair_thickness = (int)obs_data_get_int(settings, "crosshair_thickness");
    d->crosshair_alpha = (float)obs_data_get_double(settings, "crosshair_alpha");
    d->crosshair_size = (int)obs_data_get_int(settings, "crosshair_size");
    

    
    // 圓圈設定
    uint32_t obs_circle_color = (uint32_t)obs_data_get_int(settings, "circle_color");
    d->circle_color = obs_color_to_uint32(obs_circle_color);
    d->circle_thickness = (int)obs_data_get_int(settings, "circle_thickness");
    d->circle_alpha = (float)obs_data_get_double(settings, "circle_alpha");
    d->circle_radius = (int)obs_data_get_int(settings, "circle_radius");
    
    // 更新追蹤線設定
    d->show_tracking_line = obs_data_get_bool(settings, "show_tracking_line");
    d->tracking_line_mode = (enum tracking_line_mode)obs_data_get_int(settings, "tracking_line_mode");
    d->tracking_line_color = obs_color_to_uint32((uint32_t)obs_data_get_int(settings, "tracking_line_color"));
    d->tracking_line_thickness = (int)obs_data_get_int(settings, "tracking_line_thickness");
    d->tracking_line_alpha = (float)obs_data_get_double(settings, "tracking_line_alpha");
    
    // 更新路徑追蹤設定
    d->path_circle_radius = (float)obs_data_get_int(settings, "path_circle_radius");
    d->path_lifetime = (float)obs_data_get_double(settings, "path_lifetime");
    d->path_generation_interval = (float)obs_data_get_double(settings, "path_generation_interval");
    d->path_circle_color = obs_color_to_uint32((uint32_t)obs_data_get_int(settings, "path_circle_color"));

    // 若半徑或顏色改變，重建預生成 alpha 紋理
    if (d->path_alpha_radius_cache != (int)d->path_circle_radius ||
        d->path_alpha_color_cache != d->path_circle_color) {
        for (int i = 0; i < d->path_alpha_texture_count; ++i) {
            if (d->path_alpha_textures[i]) {
                gs_texture_destroy(d->path_alpha_textures[i]);
                d->path_alpha_textures[i] = NULL;
            }
        }
        obs_enter_graphics();
        for (int i = 0; i < d->path_alpha_texture_count; ++i) {
            float alpha = (float)(i * 5) / 100.0f;
            d->path_alpha_textures[i] = create_solid_circle_texture((int)d->path_circle_radius, d->path_circle_color, alpha);
        }
        obs_leave_graphics();
        d->path_alpha_radius_cache = (int)d->path_circle_radius;
        d->path_alpha_color_cache = d->path_circle_color;
    }
    
    d->recenter_speed = (float)obs_data_get_double(settings, "recenter_speed");
    d->recenter_speed_center = (float)obs_data_get_double(settings, "recenter_speed_center");
    d->recenter_speed_edge = (float)obs_data_get_double(settings, "recenter_speed_edge");
    d->crosshair_move_speed_center = (float)obs_data_get_double(settings, "crosshair_move_speed_center");
    d->crosshair_move_speed_edge = (float)obs_data_get_double(settings, "crosshair_move_speed_edge");
    d->sensitivity = (float)obs_data_get_double(settings, "sensitivity");
    d->max_offset = (int)obs_data_get_int(settings, "max_offset");
    
    // 靜止回彈加速設定
    d->enable_idle_recenter = obs_data_get_bool(settings, "enable_idle_recenter");
    d->idle_recenter_delay = (float)obs_data_get_double(settings, "idle_recenter_delay");
    d->idle_recenter_time = (float)obs_data_get_double(settings, "idle_recenter_time");
    d->idle_recenter_boost = (float)obs_data_get_double(settings, "idle_recenter_boost");
    

    
    const char *new_path = obs_data_get_string(settings, "crosshair_path");
    if (d->crosshair_path && (!new_path || strcmp(d->crosshair_path, new_path) != 0)) {
        bfree(d->crosshair_path);
        d->crosshair_path = new_path ? bstrdup(new_path) : NULL;
        
        // 強制重新創建紋理
        if (d->custom_image_texture) {
            gs_texture_destroy(d->custom_image_texture);
            d->custom_image_texture = NULL;
            d->custom_image_texture_size = 0;
        }
    }
}

// 恢復滑鼠追蹤和回彈功能
// 用於保存螢幕資訊的結構
struct monitor_info {
    POINT pt;
    RECT rect;
};

// 獲取滑鼠所在螢幕的資訊
static BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData)
{
    struct monitor_info *info = (struct monitor_info *)dwData;
    if (info->pt.x >= lprcMonitor->left && info->pt.x < lprcMonitor->right &&
        info->pt.y >= lprcMonitor->top && info->pt.y < lprcMonitor->bottom) {
        // 找到滑鼠所在的螢幕，保存其座標範圍
        info->rect = *lprcMonitor;
        return FALSE; // 停止枚舉
    }
    return TRUE; // 繼續枚舉
}

static void crosshair_box_tick(void *data, float seconds)
{
    struct dr_cursor_tracker_data *d = data;
    
    // 取得滑鼠座標
    POINT pt;
    if (GetCursorPos(&pt)) {
        if (d->mode == MODE_MOVEMENT) {
            // 移動模式：使用相對移動和回彈
            float dx = (float)pt.x - d->last_mouse_x;
            float dy = (float)pt.y - d->last_mouse_y;
            
            // 計算距離中心的距離用於動態速度
            float distance_from_center = sqrtf(d->offset_x * d->offset_x + d->offset_y * d->offset_y);
            float max_distance = (float)d->max_offset;
            float normalized_distance = (max_distance > 0.0f) ? distance_from_center / max_distance : 0.0f;
            if (normalized_distance > 1.0f) normalized_distance = 1.0f;
            
                    // 檢測滑鼠是否移動
        if (dx != 0 || dy != 0) {
            d->is_mouse_moving = true;
            d->current_idle_time = 0.0f;
            d->is_mouse_moving = true;
        } else {
            d->is_mouse_moving = false;
            // 只有在靜止時才增加時間
            if (d->enable_idle_recenter) {
                d->current_idle_time += seconds;
            }
        }

        // 計算基礎速度
        // 基礎速度 = 中心速度 + (外圍速度 - 中心速度) * 距離
        float base_speed = d->recenter_speed_center + 
            (d->recenter_speed_edge - d->recenter_speed_center) * normalized_distance;

        // 初始化加速值
        float current_boost = 0.0f;
        float progress = 0.0f;

        // 處理靜止回彈加速
        if (d->enable_idle_recenter && !d->is_mouse_moving) {
            // 增加靜止時間
            d->current_idle_time += seconds;

            // 檢查是否已經超過延遲時間（修正延遲判斷）
            if (d->idle_recenter_delay == 0.0f || d->current_idle_time >= d->idle_recenter_delay) {
                // 計算加速進度
                float acceleration_time = d->current_idle_time - d->idle_recenter_delay;
                
                if (d->idle_recenter_time > 0.0f) {
                    // 計算基本進度
                    progress = acceleration_time / d->idle_recenter_time;
                    if (progress > 1.0f) progress = 1.0f;
                } else {
                    // 如果加速時間為0，直接使用最大進度
                    progress = 1.0f;
                }

                // 加速值 = 目標加速值 * 進度
                current_boost = d->idle_recenter_boost * progress;
            } else {
                // 仍在延遲中
            }
        } else {
            // 移動時重置靜止時間
            d->current_idle_time = 0.0f;
        }

        // 計算最終速度
        // 最終速度 = (中心速度 + 加速值) + ((外圍速度 + 加速值) - (中心速度 + 加速值)) * 距離
        float boosted_center = d->recenter_speed_center + current_boost;
        float boosted_edge = d->recenter_speed_edge + current_boost;
        float final_speed = boosted_center + (boosted_edge - boosted_center) * normalized_distance;

        // 確保速度不會小於0
        if (final_speed < 0.0f) final_speed = 0.0f;

        // 更新當前回彈速度
        d->current_recenter_speed = final_speed;

        // 應用回彈效果
        if (final_speed > 0.0f) {
            float recenter_factor = 1.0f - (final_speed * seconds);
            if (recenter_factor < 0.0f) recenter_factor = 0.0f;
            d->offset_x *= recenter_factor;
            d->offset_y *= recenter_factor;
        }
            
            // 根據滑鼠移動更新偏移量（應用動態速度）
            d->offset_x += dx * d->sensitivity * d->crosshair_move_speed_center;
            d->offset_y += dy * d->sensitivity * d->crosshair_move_speed_center;
            
            // 限制最大偏移
            if (d->offset_x > (float)d->max_offset) d->offset_x = (float)d->max_offset;
            if (d->offset_x < -(float)d->max_offset) d->offset_x = -(float)d->max_offset;
            if (d->offset_y > (float)d->max_offset) d->offset_y = (float)d->max_offset;
            if (d->offset_y < -(float)d->max_offset) d->offset_y = -(float)d->max_offset;
            
                        // 動態回彈效果 - 根據距離中心的遠近和靜止時間調整回彈速度
            float distance_ratio = (max_distance > 0.0f) ? distance_from_center / max_distance : 0.0f;
            if (distance_ratio > 1.0f) distance_ratio = 1.0f;

            // 使用當前回彈速度
            d->offset_x *= (1.0f - d->current_recenter_speed * seconds);
            d->offset_y *= (1.0f - d->current_recenter_speed * seconds);
        } else {
            // 座標模式：直接映射滑鼠位置到方框內
            // 更新當前螢幕資訊
            struct monitor_info info;
            info.pt = pt;
            EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, (LPARAM)&info);
            
            // 計算滑鼠在當前螢幕中的相對位置（0.0 - 1.0）
            float relative_x = (float)(pt.x - info.rect.left) / 
                             (float)(info.rect.right - info.rect.left);
            float relative_y = (float)(pt.y - info.rect.top) / 
                             (float)(info.rect.bottom - info.rect.top);
            
            // 將相對位置映射到方框範圍內
            d->offset_x = ((relative_x * 2.0f) - 1.0f) * d->max_offset;
            d->offset_y = ((relative_y * 2.0f) - 1.0f) * d->max_offset;
        }
        
        // 路徑模式：先清理過期點，再依距離新增點
        if (d->show_tracking_line && d->tracking_line_mode == TRACKING_MODE_PATH) {
            // 計算準心中心位置
            uint32_t width = obs_source_get_base_width(d->source);
            uint32_t height = obs_source_get_base_height(d->source);
            float center_x = (float)width / 2.0f + d->offset_x;
            float center_y = (float)height / 2.0f + d->offset_y;

            // 先清理過期點（避免因尚未清掉而卡在上限）
            uint64_t lifetime_ns = (uint64_t)(d->path_lifetime * 1000000000.0f);
            struct path_point *cur = d->path_head;
            struct path_point *prev = NULL;
            uint64_t now_ns = os_gettime_ns();
            while (cur) {
                uint64_t age = now_ns - cur->timestamp;
                if (age > lifetime_ns) {
                    struct path_point *next = cur->next;
                    if (prev) prev->next = next; else d->path_head = next;
                    if (cur == d->path_tail) d->path_tail = prev;
                    bfree(cur);
                    d->path_point_count--;
                    cur = next;
                } else {
                    prev = cur;
                    cur = cur->next;
                }
            }

            // 檢查滑鼠是否移動
            bool mouse_moved = (d->last_mouse_x != (float)pt.x || d->last_mouse_y != (float)pt.y);
            if (mouse_moved) {
                // 計算是否應生成新點（以準心中心距離為準）
                bool should_generate = false;
                if (d->path_head == NULL) {
                    should_generate = true;
                } else {
                    struct path_point *last_point = d->path_tail;
                    float distance_to_last = sqrtf(
                        (center_x - last_point->x) * (center_x - last_point->x) +
                        (center_y - last_point->y) * (center_y - last_point->y));
                    if (distance_to_last >= d->path_generation_interval) should_generate = true;
                }

                const int max_points = 1000; // 提高上限，避免因上限造成週期性停頓
                if (should_generate && d->path_point_count < max_points) {
                    struct path_point *new_point = bzalloc(sizeof(struct path_point));
                    new_point->x = center_x;
                    new_point->y = center_y;
                    new_point->timestamp = now_ns;
                    new_point->next = NULL;

                    if (d->path_tail) { d->path_tail->next = new_point; d->path_tail = new_point; }
                    else { d->path_head = d->path_tail = new_point; }
                    d->path_point_count++;

                    /* 移除頻繁的點數量除錯日誌 */
                }

                // 更新最後的滑鼠位置
                d->last_mouse_x = (float)pt.x;
                d->last_mouse_y = (float)pt.y;
            }
        }
        
        d->last_mouse_x = (float)pt.x;
        d->last_mouse_y = (float)pt.y;
    }
}

static void set_effect_color(gs_effect_t *effect, uint32_t color, float alpha)
{
    // 解析 ARGB 格式的顏色 (0xAARRGGBB)
    uint8_t r = (color >> 16) & 0xFF;
    uint8_t g = (color >> 8) & 0xFF;
    uint8_t b = color & 0xFF;
    
    struct vec4 c = {
        r / 255.0f,  // R
        g / 255.0f,  // G
        b / 255.0f,  // B
        alpha         // A (使用傳入的透明度值)
    };
    gs_effect_set_vec4(gs_effect_get_param_by_name(effect, "color"), &c);
}



// 創建圓形紋理的輔助函數

static inline float clampf(float x, float min_val, float max_val)
{
    if (x < min_val) return min_val;
    if (x > max_val) return max_val;
    return x;
}

static inline float smoothstep(float edge0, float edge1, float x)
{
    float t = clampf((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

static gs_texture_t *create_circle_texture(int radius, int thickness, uint32_t color, float alpha)
{
    int texture_size = (radius + thickness) * 2;
    uint8_t *data = (uint8_t *)bzalloc(texture_size * texture_size * 4);

    float center = (float)texture_size / 2.0f;
    float inner_radius = (float)radius;
    float outer_radius = (float)(radius + thickness);
    float edge_distance = 1.0f;

    uint8_t r = (color >> 16) & 0xFF;
    uint8_t g = (color >> 8) & 0xFF;
    uint8_t b = color & 0xFF;
    uint8_t a = (uint8_t)(alpha * 255);

    for (int y = 0; y < texture_size; y++) {
        for (int x = 0; x < texture_size; x++) {
            float dx = (float)x - center;
            float dy = (float)y - center;
            float distance = sqrtf(dx * dx + dy * dy);

            if (distance >= inner_radius - edge_distance &&
                distance <= outer_radius + edge_distance) {

                float fade_in  = smoothstep(inner_radius, inner_radius + edge_distance, distance);
                float fade_out = smoothstep(outer_radius - edge_distance, outer_radius, distance);
                float alpha_multiplier = fade_in * (1.0f - fade_out);

                size_t index = (y * texture_size + x) * 4;
                data[index + 0] = r;
                data[index + 1] = g;
                data[index + 2] = b;
                data[index + 3] = (uint8_t)(a * alpha_multiplier);
            }
        }
    }

    gs_texture_t *tex = gs_texture_create(texture_size, texture_size, GS_RGBA, 1,
                                          (const uint8_t **)&data, GS_DYNAMIC);
    bfree(data);
    return tex;
}

// 創建實心圓形紋理的輔助函數
static gs_texture_t *create_solid_circle_texture(int radius, uint32_t color, float alpha)
{
    int texture_size = radius * 2;
    uint8_t *data = (uint8_t *)bzalloc(texture_size * texture_size * 4);

    float center = (float)texture_size / 2.0f;
    float circle_radius = (float)radius;
    float edge_distance = 1.0f;

    uint8_t r = (color >> 16) & 0xFF;
    uint8_t g = (color >> 8) & 0xFF;
    uint8_t b = color & 0xFF;
    uint8_t a = (uint8_t)(alpha * 255);

    for (int y = 0; y < texture_size; y++) {
        for (int x = 0; x < texture_size; x++) {
            float dx = (float)x - center;
            float dy = (float)y - center;
            float distance = sqrtf(dx * dx + dy * dy);

            size_t index = (y * texture_size + x) * 4;
            if (distance <= circle_radius + edge_distance) {
                float alpha_multiplier = 1.0f;
                if (distance > circle_radius - edge_distance) {
                    // 邊緣抗鋸齒
                    alpha_multiplier = smoothstep(circle_radius, circle_radius - edge_distance, distance);
                }

                data[index + 0] = r;
                data[index + 1] = g;
                data[index + 2] = b;
                data[index + 3] = (uint8_t)(a * alpha_multiplier);
            } else {
                // 透明背景
                data[index + 0] = 0;
                data[index + 1] = 0;
                data[index + 2] = 0;
                data[index + 3] = 0;
            }
        }
    }

    gs_texture_t *tex = gs_texture_create(texture_size, texture_size, GS_RGBA, 1,
                                          (const uint8_t **)&data, GS_DYNAMIC);
    bfree(data);
    return tex;
}

static gs_texture_t *create_white_circle_texture(int radius)
{
    int texture_size = radius * 2;
    uint8_t *data = (uint8_t *)bzalloc(texture_size * texture_size * 4);

    float center = (float)texture_size / 2.0f;
    float circle_radius = (float)radius;
    float edge_distance = 1.0f;

    for (int y = 0; y < texture_size; y++) {
        for (int x = 0; x < texture_size; x++) {
            float dx = (float)x - center;
            float dy = (float)y - center;
            float distance = sqrtf(dx * dx + dy * dy);

            size_t index = (y * texture_size + x) * 4;
            if (distance <= circle_radius + edge_distance) {
                float alpha_multiplier = 1.0f;
                if (distance > circle_radius - edge_distance) {
                    // 邊緣抗鋸齒
                    alpha_multiplier = smoothstep(circle_radius, circle_radius - edge_distance, distance);
                }

                // 純白色紋理，透明度為1.0
                data[index + 0] = 255; // R
                data[index + 1] = 255; // G
                data[index + 2] = 255; // B
                data[index + 3] = (uint8_t)(255 * alpha_multiplier); // A
            } else {
                // 透明背景
                data[index + 0] = 0;
                data[index + 1] = 0;
                data[index + 2] = 0;
                data[index + 3] = 0;
            }
        }
    }

    gs_texture_t *tex = gs_texture_create(texture_size, texture_size, GS_RGBA, 1,
                                          (const uint8_t **)&data, GS_DYNAMIC);
    bfree(data);
    return tex;
}


static uint32_t crosshair_box_get_width(void *data)
{
    struct dr_cursor_tracker_data *d = data;
    return d ? (uint32_t)d->box_size : 300;
}

static uint32_t crosshair_box_get_height(void *data)
{
    struct dr_cursor_tracker_data *d = data;
    return d ? (uint32_t)d->box_size : 300;
}

static void crosshair_box_render(void *data, gs_effect_t *effect)
{
    struct dr_cursor_tracker_data *d = data;
    if (!d) return;
    
    // 獲取源的大小
    uint32_t width = obs_source_get_base_width(d->source);
    uint32_t height = obs_source_get_base_height(d->source);
    
    // 計算方框位置（固定在中心）
    float box_x = (float)width / 2.0f - (float)d->box_size / 2.0f;
    float box_y = (float)height / 2.0f - (float)d->box_size / 2.0f;
    
    // 繪製追蹤線（最下層）
    if (d->show_tracking_line && d->tracking_line_alpha > 0.0f) {
        if (d->tracking_line_mode == TRACKING_MODE_LINEAR) {
            // 線性模式：繪製直線
            float center_x = (float)width / 2.0f;
            float center_y = (float)height / 2.0f;
            float target_x = center_x + d->offset_x;
            float target_y = center_y + d->offset_y;
            
            // 計算線的角度
            float dx = target_x - center_x;
            float dy = target_y - center_y;
            float angle = atan2f(dy, dx);
            
            // 計算線的長度
            float length = sqrtf(dx * dx + dy * dy);
            
            gs_effect_t *effect = obs_get_base_effect(OBS_EFFECT_SOLID);
            gs_technique_t *tech = gs_effect_get_technique(effect, "Solid");
            
            gs_technique_begin(tech);
            gs_technique_begin_pass(tech, 0);
            
            // 設置顏色和透明度
            set_effect_color(effect, d->tracking_line_color, d->tracking_line_alpha);
            
            // 繪製線條
            gs_matrix_push();
            gs_matrix_translate3f(center_x, center_y, 0.0f);
            gs_matrix_rotaa4f(0.0f, 0.0f, 1.0f, angle);
            // 使用線條的中心點作為原點，確保線條的中心線對準準心中心
            gs_matrix_translate3f(-(float)d->tracking_line_thickness / 2.0f, -(float)d->tracking_line_thickness / 2.0f, 0.0f);
            // 延長線條長度，確保完全覆蓋到準心中心
            gs_draw_sprite(NULL, 0, (uint32_t)(length + d->tracking_line_thickness), d->tracking_line_thickness);
            gs_matrix_pop();
            
            gs_technique_end_pass(tech);
            gs_technique_end(tech);
        } else if (d->tracking_line_mode == TRACKING_MODE_PATH) {
            // 路徑模式：繪製路徑點
            if (d->path_head && d->path_point_count > 0) {
                // 確保預生成 alpha 紋理存在
                if (d->path_alpha_radius_cache != (int)d->path_circle_radius ||
                    d->path_alpha_color_cache != d->path_circle_color) {
                    for (int i = 0; i < d->path_alpha_texture_count; ++i) {
                        if (d->path_alpha_textures[i]) {
                            gs_texture_destroy(d->path_alpha_textures[i]);
                            d->path_alpha_textures[i] = NULL;
                        }
                    }
                    obs_enter_graphics();
                    for (int i = 0; i < d->path_alpha_texture_count; ++i) {
                        float alpha = (float)(i * 5) / 100.0f;
                        d->path_alpha_textures[i] = create_solid_circle_texture((int)d->path_circle_radius, d->path_circle_color, alpha);
                    }
                    obs_leave_graphics();
                    d->path_alpha_radius_cache = (int)d->path_circle_radius;
                    d->path_alpha_color_cache = d->path_circle_color;
                }

                // 使用 Default effect 直接繪製對應 alpha 的紋理
                if (d->path_alpha_textures[0]) {
                    gs_effect_t *def_effect = obs_get_base_effect(OBS_EFFECT_DEFAULT);
                    gs_technique_t *tech = gs_effect_get_technique(def_effect, "Draw");
                    gs_eparam_t *image_param = gs_effect_get_param_by_name(def_effect, "image");
 
                    /* path-mode: texture size will be read per selected texture inside the loop */

                    uint64_t current_time = os_gettime_ns();
                    uint64_t lifetime_ns = (uint64_t)(d->path_lifetime * 1000000000.0f);

                    int drawn_points = 0;

                    // 啟用標準 alpha 混合
                    gs_blend_state_push();
                    gs_blend_function(GS_BLEND_SRCALPHA, GS_BLEND_INVSRCALPHA);
                    gs_enable_color(true, true, true, true);

                    // 一律使用 Default effect 的 Draw 技術進行繪製（若 color 參數存在則套色，否則直接繪製白色紋理）
                    gs_technique_begin(tech);
                    gs_technique_begin_pass(tech, 0);

                    struct path_point *current = d->path_head;
                    int debug_logged = 0;
                    while (current) {
                        uint64_t age = current_time - current->timestamp;
                        float age_ratio = lifetime_ns > 0 ? (float)age / (float)lifetime_ns : 1.0f;
                        if (age_ratio < 0.0f) age_ratio = 0.0f;
                        if (age_ratio > 1.0f) age_ratio = 1.0f;
                        float alpha = 1.0f - age_ratio; // 0~1
                        int idx = (int)roundf(alpha * 20.0f); // 0~20
                        if (idx < 0) idx = 0; if (idx > 20) idx = 20;
                        gs_texture_t *tex = d->path_alpha_textures[idx];
                        if (tex) {
                            gs_effect_set_texture(image_param, tex);

                            uint32_t w = gs_texture_get_width(tex);
                            uint32_t h = gs_texture_get_height(tex);

                            gs_matrix_push();
                            gs_matrix_translate3f(current->x - (float)w / 2.0f, current->y - (float)h / 2.0f, 0.0f);
                            gs_draw_sprite(tex, 0, w, h);
                            gs_matrix_pop();

                            drawn_points++;
                        }
                        current = current->next;
                    }

                    gs_technique_end_pass(tech);
                    gs_technique_end(tech);

                    gs_blend_state_pop();

                    /* 移除每幀路徑點統計的除錯日誌 */
                }
            }
        }
    }

    // 繪製圓圈（只有在不顯示自訂圖片時才顯示）
    if (d->circle_alpha > 0.0f && d->circle_thickness > 0 && !d->show_default_crosshair) {
        // 檢查是否需要重新創建圓形紋理
        bool need_recreate = false;
        if (!d->circle_texture) {
            need_recreate = true;
        } else if (d->circle_texture_size != d->circle_radius ||
                   d->circle_texture_color != d->circle_color ||
                   d->circle_texture_thickness != d->circle_thickness ||
                   fabsf(d->circle_texture_alpha - d->circle_alpha) > 0.001f) {
            need_recreate = true;
        }
        
        if (need_recreate) {
            if (d->circle_texture) {
                gs_texture_destroy(d->circle_texture);
            }
            d->circle_texture = create_circle_texture(d->circle_radius, d->circle_thickness, d->circle_color, d->circle_alpha);
            d->circle_texture_size = d->circle_radius;
            d->circle_texture_color = d->circle_color;
            d->circle_texture_thickness = d->circle_thickness;
            d->circle_texture_alpha = d->circle_alpha;
        }
        
        if (d->circle_texture) {
            gs_effect_t *effect = obs_get_base_effect(OBS_EFFECT_DEFAULT);
            gs_technique_t *tech = gs_effect_get_technique(effect, "Draw");
            gs_effect_set_texture(gs_effect_get_param_by_name(effect, "image"), d->circle_texture);
            
            gs_technique_begin(tech);
            gs_technique_begin_pass(tech, 0);
            
            // 計算準心位置（中心）
            float center_x = (float)width / 2.0f + d->offset_x;
            float center_y = (float)height / 2.0f + d->offset_y;
            
            // 獲取紋理尺寸
            uint32_t texture_width = gs_texture_get_width(d->circle_texture);
            uint32_t texture_height = gs_texture_get_height(d->circle_texture);
            
            // 繪製圓形紋理，確保紋理中心對齊到準心位置
            gs_matrix_push();
            gs_matrix_translate3f(center_x, center_y, 0.0f);
            // 計算正確的縮放比例，確保圓圈大小正確
            // 紋理尺寸是 (radius + thickness) * 8，所以實際圓圈直徑是 (radius + thickness) * 2
            float actual_circle_diameter = (float)(d->circle_radius + d->circle_thickness) * 2.0f;
            float scale = actual_circle_diameter / (float)texture_width;
            gs_matrix_scale3f(scale, scale, 1.0f);
            // 將紋理中心對齊到準心位置
            gs_matrix_translate3f(-(float)texture_width / 2.0f, -(float)texture_height / 2.0f, 0.0f);
            gs_draw_sprite(d->circle_texture, 0, texture_width, texture_height);
            gs_matrix_pop();
            
            gs_technique_end_pass(tech);
            gs_technique_end(tech);
        }
    }
    
    // 繪製準心（只有在不顯示自訂圖片時才顯示）
    if (!d->show_default_crosshair && d->crosshair_alpha > 0.0f) {
        gs_effect_t *effect = obs_get_base_effect(OBS_EFFECT_SOLID);
        gs_technique_t *tech = gs_effect_get_technique(effect, "Solid");
        
        gs_technique_begin(tech);
        gs_technique_begin_pass(tech, 0);
        
        // 設置顏色和透明度
        set_effect_color(effect, d->crosshair_color, d->crosshair_alpha);
        
        // 計算準心位置（中心）
        float center_x = (float)width / 2.0f + d->offset_x;
        float center_y = (float)height / 2.0f + d->offset_y;
        
        gs_matrix_push();
        gs_matrix_translate3f(center_x, center_y, 0.0f);
        
        // 繪製十字準心
        int crosshair_size = d->crosshair_size;
        
        // 水平線
        gs_matrix_push();
        gs_matrix_translate3f(-(float)crosshair_size / 2.0f, -(float)d->crosshair_thickness / 2.0f, 0.0f);
        gs_draw_sprite(NULL, 0, crosshair_size, d->crosshair_thickness);
        gs_matrix_pop();
        
        // 垂直線
        gs_matrix_push();
        gs_matrix_translate3f(-(float)d->crosshair_thickness / 2.0f, -(float)crosshair_size / 2.0f, 0.0f);
        gs_draw_sprite(NULL, 0, d->crosshair_thickness, crosshair_size);
        gs_matrix_pop();
        
        gs_matrix_pop();
        
        gs_technique_end_pass(tech);
        gs_technique_end(tech);
    }
    

    
    // 繪製自訂圖片（只有在顯示自訂圖片時才顯示）
    if (d->show_default_crosshair && d->crosshair_path && strlen(d->crosshair_path) > 0) {
            // 檢查是否需要重新創建自訂圖片紋理
        bool need_recreate = false;
        if (!d->custom_image_texture) {
            need_recreate = true;
        } else if (d->custom_image_texture_size != 1) { // 使用1作為標識符
            need_recreate = true;
        }
        
        if (need_recreate) {
            if (d->custom_image_texture) {
                gs_texture_destroy(d->custom_image_texture);
            }
            d->custom_image_texture = create_crosshair_texture(d->crosshair_path); // 使用相同的載入函數
            if (d->custom_image_texture) {
                /* 自訂圖片紋理創建成功：不輸出資訊日誌以降低噪音 */
                d->custom_image_texture_size = 1; // 標識符
            }
        }
        
        if (d->custom_image_texture) {
            // 計算準心位置（中心）
            float center_x = (float)width / 2.0f + d->offset_x;
            float center_y = (float)height / 2.0f + d->offset_y;
            
            uint32_t texture_width = gs_texture_get_width(d->custom_image_texture);
            uint32_t texture_height = gs_texture_get_height(d->custom_image_texture);
            
            // 設定混合狀態
            gs_blend_state_push();
            gs_blend_function(GS_BLEND_SRCALPHA, GS_BLEND_INVSRCALPHA);
            gs_enable_color(true, true, true, true);
            
            // 使用預設效果
            gs_effect_t *effect = obs_get_base_effect(OBS_EFFECT_DEFAULT);
            gs_technique_t *tech = gs_effect_get_technique(effect, "Draw");
            gs_effect_set_texture(gs_effect_get_param_by_name(effect, "image"), d->custom_image_texture);
            
            gs_technique_begin(tech);
            gs_technique_begin_pass(tech, 0);
            
            // 繪製自訂圖片紋理，確保紋理中心對齊到準心位置
            gs_matrix_push();
            gs_matrix_translate3f(center_x, center_y, 0.0f);
            // 將紋理中心對齊到準心位置
            gs_matrix_translate3f(-(float)texture_width / 2.0f, -(float)texture_height / 2.0f, 0.0f);
            gs_draw_sprite(d->custom_image_texture, 0, texture_width, texture_height);
            gs_matrix_pop();
            
            gs_technique_end_pass(tech);
            gs_technique_end(tech);
            
            // 恢復混合狀態
            gs_blend_state_pop();
        }
    }
    
    // 最後繪製方框（確保顯示在最上層）
    if (d->box_alpha > 0.0f) {
        gs_effect_t *effect = obs_get_base_effect(OBS_EFFECT_SOLID);
        gs_technique_t *tech = gs_effect_get_technique(effect, "Solid");
        
        gs_technique_begin(tech);
        gs_technique_begin_pass(tech, 0);
        
        // 設置顏色和透明度
        set_effect_color(effect, d->box_color, d->box_alpha);
        
        // 繪製方框的四條邊
        gs_matrix_push();
        gs_matrix_translate3f(box_x, box_y, 0.0f);
        
        // 上邊
        gs_draw_sprite(NULL, 0, d->box_size, d->box_thickness);
        
        // 下邊
        gs_matrix_translate3f(0.0f, (float)(d->box_size - d->box_thickness), 0.0f);
        gs_draw_sprite(NULL, 0, d->box_size, d->box_thickness);
        
        // 左邊
        gs_matrix_translate3f(0.0f, -(float)(d->box_size - d->box_thickness), 0.0f);
        gs_draw_sprite(NULL, 0, d->box_thickness, d->box_size);
        
        // 右邊
        gs_matrix_translate3f((float)(d->box_size - d->box_thickness), 0.0f, 0.0f);
        gs_draw_sprite(NULL, 0, d->box_thickness, d->box_size);
        
        gs_matrix_pop();
        
        gs_technique_end_pass(tech);
        gs_technique_end(tech);
    }
}

// 添加屬性變更回調函數
static bool crosshair_properties_modified(obs_properties_t *props, obs_property_t *property, obs_data_t *settings)
{
    UNUSED_PARAMETER(property);
    
    if (!props || !settings) {
        return true;
    }
    
    bool show_custom_image = obs_data_get_bool(settings, "show_default_crosshair");
    bool show_box = obs_data_get_bool(settings, "show_box");
    bool show_tracking_line = obs_data_get_bool(settings, "show_tracking_line");
    int tracking_line_mode = (int)obs_data_get_int(settings, "tracking_line_mode");
    
    // 根據 show_default_crosshair 的狀態來顯示/隱藏相關屬性
    obs_property_t *crosshair_path_prop = obs_properties_get(props, "crosshair_path");
    obs_property_t *crosshair_thickness_prop = obs_properties_get(props, "crosshair_thickness");
    obs_property_t *crosshair_color_prop = obs_properties_get(props, "crosshair_color");
    obs_property_t *crosshair_size_prop = obs_properties_get(props, "crosshair_size");
    obs_property_t *crosshair_alpha_prop = obs_properties_get(props, "crosshair_alpha");
    
    if (crosshair_path_prop) {
        obs_property_set_visible(crosshair_path_prop, show_custom_image);
    }
    if (crosshair_thickness_prop) {
        obs_property_set_visible(crosshair_thickness_prop, !show_custom_image);
    }
    if (crosshair_color_prop) {
        obs_property_set_visible(crosshair_color_prop, !show_custom_image);
    }
    if (crosshair_size_prop) {
        obs_property_set_visible(crosshair_size_prop, !show_custom_image);
    }
    if (crosshair_alpha_prop) {
        obs_property_set_visible(crosshair_alpha_prop, !show_custom_image);
    }

    // 當使用自訂圖片時，隱藏整個圓圈設定群組
    obs_property_t *circle_settings_group_prop = obs_properties_get(props, "circle_settings");
    if (circle_settings_group_prop) {
        obs_property_set_visible(circle_settings_group_prop, !show_custom_image);
    }

    // 方框設定的可見性：未勾選顯示方框時，隱藏方框相關欄位（保留群組以顯示勾選）
    obs_property_t *box_size_prop = obs_properties_get(props, "box_size");
    obs_property_t *box_color_prop = obs_properties_get(props, "box_color");
    obs_property_t *box_thickness_prop = obs_properties_get(props, "box_thickness");
    obs_property_t *box_alpha_prop = obs_properties_get(props, "box_alpha");
    if (box_size_prop) {
        // 方框大小不隨顯示方框開關而隱藏
        obs_property_set_visible(box_size_prop, true);
    }
    if (box_color_prop) {
        obs_property_set_visible(box_color_prop, show_box);
    }
    if (box_thickness_prop) {
        obs_property_set_visible(box_thickness_prop, show_box);
    }
    if (box_alpha_prop) {
        obs_property_set_visible(box_alpha_prop, show_box);
    }
    
    // 根據 show_tracking_line 的狀態來顯示/隱藏追蹤線相關屬性
    obs_property_t *tracking_mode_list_prop = obs_properties_get(props, "tracking_line_mode");
    obs_property_t *tracking_line_color_prop = obs_properties_get(props, "tracking_line_color");
    obs_property_t *tracking_line_thickness_prop = obs_properties_get(props, "tracking_line_thickness");
    obs_property_t *tracking_line_alpha_prop = obs_properties_get(props, "tracking_line_alpha");
    
    if (tracking_mode_list_prop) {
        obs_property_set_visible(tracking_mode_list_prop, show_tracking_line);
    }
    // 線性模式專用設定：僅在線性模式時顯示
    bool show_linear_settings = show_tracking_line && (tracking_line_mode == TRACKING_MODE_LINEAR);
    if (tracking_line_color_prop) {
        obs_property_set_visible(tracking_line_color_prop, show_linear_settings);
    }
    if (tracking_line_thickness_prop) {
        obs_property_set_visible(tracking_line_thickness_prop, show_linear_settings);
    }
    if (tracking_line_alpha_prop) {
        obs_property_set_visible(tracking_line_alpha_prop, show_linear_settings);
    }
    
    // 路徑模式專用設定的可見性（只有在顯示追蹤線且模式為路徑模式時才顯示）
    bool show_path_settings = show_tracking_line && (tracking_line_mode == TRACKING_MODE_PATH);
    obs_property_t *path_circle_color_prop = obs_properties_get(props, "path_circle_color");
    obs_property_t *path_circle_radius_prop = obs_properties_get(props, "path_circle_radius");
    obs_property_t *path_lifetime_prop = obs_properties_get(props, "path_lifetime");
    obs_property_t *path_generation_interval_prop = obs_properties_get(props, "path_generation_interval");
    
    if (path_circle_color_prop) {
        obs_property_set_visible(path_circle_color_prop, show_path_settings);
    }
    if (path_circle_radius_prop) {
        obs_property_set_visible(path_circle_radius_prop, show_path_settings);
    }
    if (path_lifetime_prop) {
        obs_property_set_visible(path_lifetime_prop, show_path_settings);
    }
    if (path_generation_interval_prop) {
        obs_property_set_visible(path_generation_interval_prop, show_path_settings);
    }
    
    // 根據 enable_idle_recenter 的狀態來顯示/隱藏靜止回彈加速相關屬性
    bool enable_idle_recenter = obs_data_get_bool(settings, "enable_idle_recenter");
    obs_property_t *idle_recenter_delay_prop = obs_properties_get(props, "idle_recenter_delay");
    obs_property_t *idle_recenter_time_prop = obs_properties_get(props, "idle_recenter_time");
    obs_property_t *idle_recenter_boost_prop = obs_properties_get(props, "idle_recenter_boost");
    
    if (idle_recenter_delay_prop) {
        obs_property_set_visible(idle_recenter_delay_prop, enable_idle_recenter);
    }
    if (idle_recenter_time_prop) {
        obs_property_set_visible(idle_recenter_time_prop, enable_idle_recenter);
    }
    if (idle_recenter_boost_prop) {
        obs_property_set_visible(idle_recenter_boost_prop, enable_idle_recenter);
    }
    
    return true;
}

static obs_properties_t *crosshair_box_properties(void *data)
{
    obs_properties_t *props = obs_properties_create();
    
    // 方框設定群組
    obs_properties_t *box_group = obs_properties_create();
    obs_property_t *show_box_prop = obs_properties_add_bool(box_group, "show_box", obs_module_text("ShowBox"));
    obs_property_set_modified_callback(show_box_prop, crosshair_properties_modified);
    obs_properties_add_int(box_group, "box_size", obs_module_text("BoxSize"), 50, 1920, 1);
    obs_properties_add_color(box_group, "box_color", obs_module_text("BoxColor"));
    obs_properties_add_int_slider(box_group, "box_thickness", obs_module_text("BoxThickness"), 1, 2000, 1);
    obs_properties_add_float_slider(box_group, "box_alpha", obs_module_text("BoxAlpha"), 0.0, 1.0, 0.01);
    obs_properties_add_group(props, "box_settings", obs_module_text("BoxSettings"), OBS_GROUP_NORMAL, box_group);
    
    // 準心設定群組
    obs_properties_t *crosshair_group = obs_properties_create();
    
    // 添加模式選擇
    obs_property_t *mode_list = obs_properties_add_list(crosshair_group, "crosshair_mode", 
        obs_module_text("CrosshairMode"), OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
    obs_property_list_add_int(mode_list, obs_module_text("MovementMode"), MODE_MOVEMENT);
    obs_property_list_add_int(mode_list, obs_module_text("CoordinateMode"), MODE_COORDINATE);
    
    obs_properties_add_int(crosshair_group, "max_offset", obs_module_text("MaxOffset"), 10, 500, 1);
    
    // 添加自訂圖片選項，並設定回調函數
    obs_property_t *show_custom_image_prop = obs_properties_add_bool(crosshair_group, "show_default_crosshair", obs_module_text("ShowCustomImage"));
    obs_property_set_modified_callback(show_custom_image_prop, crosshair_properties_modified);
    
    // 自訂圖片路徑（當 show_default_crosshair 為 true 時顯示）
    obs_properties_add_path(crosshair_group, "crosshair_path", obs_module_text("CrosshairImage"), OBS_PATH_FILE, obs_module_text("Browse"), NULL);
    
    // 準心十字設定（當 show_default_crosshair 為 false 時顯示）
    obs_properties_add_int_slider(crosshair_group, "crosshair_thickness", obs_module_text("CrosshairCrossLength"), 1, 2000, 1);
    obs_properties_add_color(crosshair_group, "crosshair_color", obs_module_text("CrosshairColor"));
    obs_properties_add_int_slider(crosshair_group, "crosshair_size", obs_module_text("CrosshairThickness"), 1, 100, 1);
    obs_properties_add_float_slider(crosshair_group, "crosshair_alpha", obs_module_text("CrosshairAlpha"), 0.0, 1.0, 0.01);
    
    obs_properties_add_group(props, "crosshair_settings", obs_module_text("CrosshairSettings"), OBS_GROUP_NORMAL, crosshair_group);
    
    // 圓圈設定群組
    obs_properties_t *circle_group = obs_properties_create();
    obs_properties_add_color(circle_group, "circle_color", obs_module_text("CircleColor"));
    obs_properties_add_int_slider(circle_group, "circle_thickness", obs_module_text("CircleThickness"), 0, 1000, 1);
    obs_properties_add_int_slider(circle_group, "circle_radius", obs_module_text("CircleRadius"), 1, 1000, 1);
    obs_properties_add_float_slider(circle_group, "circle_alpha", obs_module_text("CircleAlpha"), 0.0, 1.0, 0.01);
    obs_properties_add_group(props, "circle_settings", obs_module_text("CircleSettings"), OBS_GROUP_NORMAL, circle_group);
    
    // 追蹤線設定群組
    obs_properties_t *tracking_line_group = obs_properties_create();
    obs_property_t *show_tracking_line_prop = obs_properties_add_bool(tracking_line_group, "show_tracking_line", obs_module_text("ShowTrackingLine"));
    obs_property_set_modified_callback(show_tracking_line_prop, crosshair_properties_modified);
    
    // 添加追蹤線模式選擇
    obs_property_t *tracking_mode_list = obs_properties_add_list(tracking_line_group, "tracking_line_mode", 
        obs_module_text("TrackingLineMode"), OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
    obs_property_list_add_int(tracking_mode_list, obs_module_text("LinearMode"), TRACKING_MODE_LINEAR);
    obs_property_list_add_int(tracking_mode_list, obs_module_text("PathMode"), TRACKING_MODE_PATH);
    obs_property_set_modified_callback(tracking_mode_list, crosshair_properties_modified);
    
    obs_properties_add_color(tracking_line_group, "tracking_line_color", obs_module_text("TrackingLineColor"));
    obs_properties_add_int_slider(tracking_line_group, "tracking_line_thickness", obs_module_text("TrackingLineThickness"), 1, 200, 1);
    obs_properties_add_float_slider(tracking_line_group, "tracking_line_alpha", obs_module_text("TrackingLineAlpha"), 0.0, 1.0, 0.01);
    
    // 路徑模式專用設定
    obs_properties_add_color(tracking_line_group, "path_circle_color", obs_module_text("PathCircleColor"));
    obs_properties_add_int_slider(tracking_line_group, "path_circle_radius", obs_module_text("PathCircleRadius"), 1, 50, 1);
    obs_properties_add_float_slider(tracking_line_group, "path_lifetime", obs_module_text("PathLifetime"), 0.1, 10.0, 0.1);
    obs_properties_add_float_slider(tracking_line_group, "path_generation_interval", obs_module_text("PathGenerationInterval"), 5.0f, 100.0f, 5.0f);
    
    obs_properties_add_group(props, "tracking_line_settings", obs_module_text("TrackingLineSettings"), OBS_GROUP_NORMAL, tracking_line_group);
    
    // 速度設定群組
    obs_properties_t *speed_group = obs_properties_create();
    obs_properties_add_float_slider(speed_group, "recenter_speed", obs_module_text("ReboundSpeed"), 0.0, 10.0, 0.1);
    obs_properties_add_float_slider(speed_group, "recenter_speed_center", obs_module_text("CenterReboundSpeed"), 0.0, 20.0, 0.01);
    obs_properties_add_float_slider(speed_group, "recenter_speed_edge", obs_module_text("OuterReboundSpeed"), 0.0, 20.0, 0.01);
    obs_properties_add_float_slider(speed_group, "sensitivity", obs_module_text("CrosshairSensitivity"), 0.01, 10.0, 0.01);
    obs_properties_add_float_slider(speed_group, "crosshair_move_speed_center", obs_module_text("CrosshairCenterMoveSpeed"), 0.0, 20.0, 0.01);
    obs_properties_add_float_slider(speed_group, "crosshair_move_speed_edge", obs_module_text("CrosshairOuterMoveSpeed"), 0.0, 20.0, 0.01);
    obs_property_t *enable_idle_recenter_prop = obs_properties_add_bool(speed_group, "enable_idle_recenter", obs_module_text("EnableIdleRecenter"));
    obs_property_set_modified_callback(enable_idle_recenter_prop, crosshair_properties_modified);
    obs_properties_add_float_slider(speed_group, "idle_recenter_delay", obs_module_text("IdleRecenterDelay"), 0.0, 2.0, 0.1);
    obs_properties_add_float_slider(speed_group, "idle_recenter_time", obs_module_text("IdleRecenterTime"), 0.1, 5.0, 0.1);
    obs_properties_add_float_slider(speed_group, "idle_recenter_boost", obs_module_text("IdleRecenterBoost"), 0.0, 10.0, 0.01);
    obs_properties_add_group(props, "speed_settings", obs_module_text("SpeedSettings"), OBS_GROUP_NORMAL, speed_group);
    
    // 初始化屬性可見性
    if (data) {
        struct dr_cursor_tracker_data *d = (struct dr_cursor_tracker_data*)data;
        if (d->source) {
            obs_data_t *settings = obs_source_get_settings(d->source);
            if (settings) {
                crosshair_properties_modified(props, NULL, settings);
                obs_data_release(settings);
            }
        }
    }
    
    return props;
}

static void crosshair_box_defaults(obs_data_t *settings)
{
    obs_data_set_default_int(settings, "crosshair_mode", MODE_MOVEMENT);
    obs_data_set_default_int(settings, "box_size", 500);
    obs_data_set_default_int(settings, "box_color", uint32_to_obs_color(0xFF0000FF)); // 藍色
    obs_data_set_default_int(settings, "box_thickness", 4);
    obs_data_set_default_double(settings, "box_alpha", 1.0);
    
    obs_data_set_default_int(settings, "max_offset", 200);
    
    obs_data_set_default_bool(settings, "show_default_crosshair", false);
    obs_data_set_default_int(settings, "crosshair_thickness", 48);
    obs_data_set_default_int(settings, "crosshair_color", uint32_to_obs_color(0xFFFF0000)); // 紅色
    obs_data_set_default_int(settings, "crosshair_size", 4);
    obs_data_set_default_double(settings, "crosshair_alpha", 1.0);
    
    // 圓圈預設值
    obs_data_set_default_int(settings, "circle_color", uint32_to_obs_color(0xFF00FFFF)); // 青色
    obs_data_set_default_int(settings, "circle_thickness", 48);
    obs_data_set_default_int(settings, "circle_radius", 15);
    obs_data_set_default_double(settings, "circle_alpha", 1.0);
    // 方框顯示預設
    obs_data_set_default_bool(settings, "show_box", true);
    
    // 追蹤線預設值
    obs_data_set_default_bool(settings, "show_tracking_line", true);
    // 預設維持線性模式
    obs_data_set_default_int(settings, "tracking_line_mode", TRACKING_MODE_LINEAR);
    obs_data_set_default_int(settings, "tracking_line_color", uint32_to_obs_color(0xFF0000FF)); // 藍色
    obs_data_set_default_int(settings, "tracking_line_thickness", 6);
    obs_data_set_default_double(settings, "tracking_line_alpha", 1.0);
    
    // 路徑模式預設值
    obs_data_set_default_int(settings, "path_circle_radius", 8);
    obs_data_set_default_int(settings, "path_circle_color", uint32_to_obs_color(0xFF00FF00)); // 綠色
    obs_data_set_default_double(settings, "path_generation_interval", 20.0); // 距離間隔：20像素
    obs_data_set_default_double(settings, "path_lifetime", 2.0);
    
    obs_data_set_default_double(settings, "recenter_speed", 5.0);
    obs_data_set_default_double(settings, "recenter_speed_center", 0.75);
    obs_data_set_default_double(settings, "recenter_speed_edge", 1.50);
    obs_data_set_default_double(settings, "crosshair_move_speed_center", 1.0);
    obs_data_set_default_double(settings, "crosshair_move_speed_edge", 0.05);
    obs_data_set_default_double(settings, "sensitivity", 0.25);
    obs_data_set_default_bool(settings, "enable_idle_recenter", true);
    obs_data_set_default_double(settings, "idle_recenter_delay", 0.10);
    obs_data_set_default_double(settings, "idle_recenter_time", 2.0);
    obs_data_set_default_double(settings, "idle_recenter_boost", 10.0);
}

struct obs_source_info dr_cursor_tracker_info = {
    .id = "dr_cursor_tracker",
    .type = OBS_SOURCE_TYPE_INPUT,
    .output_flags = OBS_SOURCE_VIDEO,
    .get_name = crosshair_box_get_name,
    .create = crosshair_box_create,
    .destroy = crosshair_box_destroy,
    .update = crosshair_box_update,
    .video_render = crosshair_box_render,
    .video_tick = crosshair_box_tick, // 恢復 video_tick
    .get_properties = crosshair_box_properties,
    .get_defaults = crosshair_box_defaults,
    .get_width = crosshair_box_get_width,
    .get_height = crosshair_box_get_height,
};

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("DR_CursorTracker", "en-US")

bool obs_module_load(void)
{
    obs_register_source(&dr_cursor_tracker_info);
    blog(LOG_INFO, BLOG_PREFIX "插件載入成功");
    return true;
}
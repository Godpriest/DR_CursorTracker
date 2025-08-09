
#pragma once
#include <obs-module.h>
#include <graphics/image-file.h>
#include <windows.h>

// 抗鋸齒圓圈紋理產生函式
gs_texture_t *create_circle_texture(int radius, int thickness, uint32_t color, float alpha);

// 準心運作模式
enum crosshair_mode {
    MODE_MOVEMENT = 0,  // 移動模式（原有的移動+回彈）
    MODE_COORDINATE = 1 // 座標模式（根據滑鼠位置）
};

// 追蹤線模式
enum tracking_line_mode {
    TRACKING_MODE_LINEAR = 0,  // 線性模式（原有的直線）
    TRACKING_MODE_PATH = 1     // 路徑模式（生成小圈路徑）
};

// 路徑點結構
struct path_point {
    float x;
    float y;
    uint64_t timestamp;
    struct path_point *next;
};

struct dr_cursor_tracker_data {
    enum crosshair_mode mode; // 準心運作模式
    int box_size;
    uint32_t box_color;
    int box_thickness;
    float box_alpha; // 方框透明度
    bool show_default_crosshair;
    uint32_t crosshair_color;
    int crosshair_thickness;
    float crosshair_alpha; // 準心透明度
    int crosshair_size;
    // 圓圈設定
    uint32_t circle_color;
    int circle_thickness;
    float circle_alpha; // 圓圈透明度
    int circle_radius; // 圓圈半徑
    float recenter_speed;
    float recenter_speed_center; // 中心回彈速度比例 (0.0 = 0%, 20.0 = 2000%)
    float recenter_speed_edge;   // 外圍回彈速度比例 (0.0 = 0%, 20.0 = 2000%)
    float crosshair_move_speed_center; // 準心中心移速 (0.0 = 0%, 20.0 = 2000%)
    float crosshair_move_speed_edge;   // 準心外圍移速 (0.0 = 0%, 20.0 = 2000%)
    float sensitivity;
    int max_offset;
    char *crosshair_path;
    // 自訂圖片紋理
    gs_texture_t *custom_image_texture;
    int custom_image_texture_size;
    // 圓形紋理
    gs_texture_t *circle_texture;
    int circle_texture_size;
    uint32_t circle_texture_color;
    int circle_texture_thickness;
    float circle_texture_alpha;
    // 追蹤線設定
    bool show_tracking_line;
    enum tracking_line_mode tracking_line_mode; // 追蹤線模式
    uint32_t tracking_line_color;
    int tracking_line_thickness;
    float tracking_line_alpha;
    // 路徑模式設定
    float path_circle_radius;
    float path_lifetime;
    struct path_point *path_head;
    struct path_point *path_tail;
    int path_point_count;
    gs_texture_t *path_circle_texture;
    int path_circle_texture_size;
    uint32_t path_circle_color;
    int path_circle_texture_thickness;
    float path_circle_texture_alpha;
    uint64_t last_path_time;
    float path_generation_interval; // 距離間隔（像素）

    // 預先生成的 alpha 紋理（0,5,10,...,100 共 21 檔）
    gs_texture_t *path_alpha_textures[21];
    int path_alpha_texture_count;           // 固定為 21
    int path_alpha_radius_cache;            // 目前快取對應的半徑
    int path_alpha_thickness_cache;         // 若改為實心可忽略
    uint32_t path_alpha_color_cache;        // 目前快取對應的顏色

    float offset_x;
    float offset_y;
    float last_mouse_x;
    float last_mouse_y;
    uint64_t last_update_time;
    obs_source_t *source; // 保存源指針
    // 靜止回彈加速設定
    bool enable_idle_recenter;       // 是否啟用靜止回彈加速
    float idle_recenter_delay;       // 開始加速前的延遲時間（秒）
    float idle_recenter_time;        // 回彈加速時間（秒）
    float idle_recenter_boost;       // 靜止回彈速度增加值 (0.5 = +50%, 1.0 = +100%)
    float current_idle_time;         // 當前靜止時間
    float current_recenter_speed;    // 當前回彈速度
    bool is_mouse_moving;           // 滑鼠是否正在移動
};

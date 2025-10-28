/**
 * @file ui_landscape.c
 * @brief Implementation of landscape UI layout with video/animation panel and chat interface
 *
 * This file implements a horizontal layout with:
 * - Left panel: Video/animation display area (50% width)
 * - Right panel: Chat interface (50% width)
 *
 * Screen orientation: Landscape (480x320)
 * Layout: [Video/Animation Panel | Chat Interface Panel]
 *
 * @copyright Copyright (c) 2021-2025 Tuya Inc. All Rights Reserved.
 */

#include "tuya_cloud_types.h"

#if defined(ENABLE_GUI_LANDSCAPE) && (ENABLE_GUI_LANDSCAPE == 1)

#include "ui_display.h"
#include "font_awesome_symbols.h"
#include "lvgl.h"

// 声明GIF资源
LV_IMG_DECLARE(talking_tiny);

/***********************************************************
************************macro define************************
***********************************************************/
#define VIDEO_PANEL_WIDTH (LV_HOR_RES / 2) // Left half for video/animation
#define CHAT_PANEL_WIDTH  (LV_HOR_RES / 2) // Right half for chat
#define STATUS_BAR_HEIGHT 24               // Status bar height
#define PANEL_PADDING     5                // Padding between panels

/***********************************************************
***********************typedef define***********************
***********************************************************/
// Theme color structure
typedef struct {
    lv_color_t background;
    lv_color_t text;
    lv_color_t panel_bg;
    lv_color_t user_bubble;
    lv_color_t assistant_bubble;
    lv_color_t system_bubble;
    lv_color_t border;
    lv_color_t video_bg;
} LANDSCAPE_THEME_COLORS_T;

typedef struct {
    // Main containers
    lv_obj_t *main_container;
    lv_obj_t *status_bar;
    lv_obj_t *content_container;

    // Left panel (video/animation)
    lv_obj_t *video_panel;
    lv_obj_t *video_content;
    lv_obj_t *video_placeholder;

    // Right panel (chat interface)
    lv_obj_t *chat_panel;
    lv_obj_t *chat_content;
    lv_obj_t *emotion_label;
    lv_obj_t *chat_message_label;
    lv_obj_t *chat_scroll_area;

    // Status bar elements
    lv_obj_t *status_label;
    lv_obj_t *network_label;
    lv_obj_t *notification_label;
    lv_obj_t *chat_mode_label;
} LANDSCAPE_UI_T;

typedef struct {
    LANDSCAPE_UI_T ui;
    LANDSCAPE_THEME_COLORS_T theme;
    UI_FONT_T font;
    lv_timer_t *notification_tm;
    lv_timer_t *video_animation_tm;
    lv_obj_t *talking_gif;    // 动画GIF对象
} LANDSCAPE_CHATBOT_UI_T;

/***********************************************************
***********************variable define**********************
***********************************************************/
static LANDSCAPE_CHATBOT_UI_T sg_landscape_ui = {0};

/***********************************************************
***********************function define**********************
***********************************************************/

/**
 * @brief Initialize light theme colors
 */
static void __landscape_theme_init(LANDSCAPE_THEME_COLORS_T *theme)
{
    if (theme == NULL) {
        return;
    }

    theme->background = lv_color_hex(0xFDF6E3);     // 暖米色背景，护眼
    theme->text = lv_color_hex(0x657B83);           // 蓝灰文字
    theme->panel_bg = lv_color_hex(0xEEE8D5);       // 浅米色面板
    theme->user_bubble = lv_color_hex(0x268BD2);    // 蓝色用户消息
    theme->assistant_bubble = lv_color_hex(0x859900); // 绿色AI回复
    theme->system_bubble = lv_color_hex(0xB58900);   // 橙色系统消息
    theme->border = lv_color_hex(0xD3C7AA);         // 米色边框
    theme->video_bg = lv_color_hex(0x073642);
}

/**
 * @brief Create video/animation panel (left side)
 */
static void __create_video_panel(void)
{
    // Video panel container
    sg_landscape_ui.ui.video_panel = lv_obj_create(sg_landscape_ui.ui.content_container);
    lv_obj_set_size(sg_landscape_ui.ui.video_panel, VIDEO_PANEL_WIDTH - PANEL_PADDING, LV_VER_RES - STATUS_BAR_HEIGHT);
    lv_obj_set_pos(sg_landscape_ui.ui.video_panel, 0, 0);
    lv_obj_set_style_bg_color(sg_landscape_ui.ui.video_panel, sg_landscape_ui.theme.video_bg, 0);
    lv_obj_set_style_border_color(sg_landscape_ui.ui.video_panel, sg_landscape_ui.theme.border, 0);
    lv_obj_set_style_border_width(sg_landscape_ui.ui.video_panel, 1, 0);
    lv_obj_set_style_radius(sg_landscape_ui.ui.video_panel, 8, 0);
    lv_obj_set_style_pad_all(sg_landscape_ui.ui.video_panel, 10, 0);

    // Video content area
    sg_landscape_ui.ui.video_content = lv_obj_create(sg_landscape_ui.ui.video_panel);
    lv_obj_set_size(sg_landscape_ui.ui.video_content, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_color(sg_landscape_ui.ui.video_content, sg_landscape_ui.theme.video_bg, 0);
    lv_obj_set_style_border_width(sg_landscape_ui.ui.video_content, 0, 0);
    lv_obj_set_style_radius(sg_landscape_ui.ui.video_content, 4, 0);
    lv_obj_center(sg_landscape_ui.ui.video_content);

    // 创建并显示GIF，默认暂停
    sg_landscape_ui.talking_gif = lv_gif_create(sg_landscape_ui.ui.video_content);
    lv_gif_set_src(sg_landscape_ui.talking_gif, &talking_tiny);
    lv_obj_center(sg_landscape_ui.talking_gif);
    
    // 添加圆角效果
    lv_obj_set_style_radius(sg_landscape_ui.talking_gif, 15, 0);
    lv_obj_set_style_clip_corner(sg_landscape_ui.talking_gif, true, 0);
    
    // 创建后立即暂停，等待AI回复时再播放
    lv_gif_pause(sg_landscape_ui.talking_gif);

    // 占位符设为NULL
    sg_landscape_ui.ui.video_placeholder = NULL;
}

/**
 * @brief Create chat interface panel (right side)
 */
static void __create_chat_panel(void)
{
    // Chat panel container
    sg_landscape_ui.ui.chat_panel = lv_obj_create(sg_landscape_ui.ui.content_container);
    lv_obj_set_size(sg_landscape_ui.ui.chat_panel, CHAT_PANEL_WIDTH - PANEL_PADDING, LV_VER_RES - STATUS_BAR_HEIGHT);
    lv_obj_set_pos(sg_landscape_ui.ui.chat_panel, VIDEO_PANEL_WIDTH + PANEL_PADDING, 0);
    lv_obj_set_style_bg_color(sg_landscape_ui.ui.chat_panel, sg_landscape_ui.theme.panel_bg, 0);
    lv_obj_set_style_border_color(sg_landscape_ui.ui.chat_panel, sg_landscape_ui.theme.border, 0);
    lv_obj_set_style_border_width(sg_landscape_ui.ui.chat_panel, 1, 0);
    lv_obj_set_style_radius(sg_landscape_ui.ui.chat_panel, 8, 0);
    lv_obj_set_style_pad_all(sg_landscape_ui.ui.chat_panel, 10, 0);

    // 聊天滚动区域（占满整个chat_panel，移除表情后）
    sg_landscape_ui.ui.chat_scroll_area = lv_obj_create(sg_landscape_ui.ui.chat_panel);
    lv_obj_set_size(sg_landscape_ui.ui.chat_scroll_area, lv_pct(100), lv_pct(100));
    lv_obj_set_style_border_width(sg_landscape_ui.ui.chat_scroll_area, 0, 0);
    lv_obj_set_style_bg_color(sg_landscape_ui.ui.chat_scroll_area, lv_color_white(), 0);
    lv_obj_set_style_radius(sg_landscape_ui.ui.chat_scroll_area, 4, 0);
    lv_obj_set_style_pad_all(sg_landscape_ui.ui.chat_scroll_area, 5, 0);
    lv_obj_center(sg_landscape_ui.ui.chat_scroll_area);
    
    // 启用垂直滚动
    lv_obj_set_scrollbar_mode(sg_landscape_ui.ui.chat_scroll_area, LV_SCROLLBAR_MODE_AUTO);
    lv_obj_set_scroll_dir(sg_landscape_ui.ui.chat_scroll_area, LV_DIR_VER);

    // 聊天消息标签（支持自动换行和滚动）
    sg_landscape_ui.ui.chat_message_label = lv_label_create(sg_landscape_ui.ui.chat_scroll_area);
    lv_label_set_text(sg_landscape_ui.ui.chat_message_label, "");
    lv_obj_set_width(sg_landscape_ui.ui.chat_message_label, lv_pct(100));
    lv_label_set_long_mode(sg_landscape_ui.ui.chat_message_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_align(sg_landscape_ui.ui.chat_message_label, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_style_text_font(sg_landscape_ui.ui.chat_message_label, sg_landscape_ui.font.text, 0);
    lv_obj_set_style_pad_all(sg_landscape_ui.ui.chat_message_label, 8, 0);
    
    // 移除不需要的组件
    sg_landscape_ui.ui.emotion_label = NULL;
    sg_landscape_ui.ui.chat_content = NULL;
}

/**
 * @brief Create status bar (top)
 */
static void __create_status_bar(void)
{
    // Status bar
    sg_landscape_ui.ui.status_bar = lv_obj_create(sg_landscape_ui.ui.main_container);
    lv_obj_set_size(sg_landscape_ui.ui.status_bar, LV_HOR_RES, STATUS_BAR_HEIGHT);
    lv_obj_set_pos(sg_landscape_ui.ui.status_bar, 0, 0);
    lv_obj_set_style_bg_color(sg_landscape_ui.ui.status_bar, sg_landscape_ui.theme.background, 0);
    lv_obj_set_style_border_width(sg_landscape_ui.ui.status_bar, 0, 0);
    lv_obj_set_style_pad_all(sg_landscape_ui.ui.status_bar, 2, 0);

    // Chat mode label (left)
    sg_landscape_ui.ui.chat_mode_label = lv_label_create(sg_landscape_ui.ui.status_bar);
    lv_obj_set_style_text_color(sg_landscape_ui.ui.chat_mode_label, sg_landscape_ui.theme.text, 0);
    lv_obj_set_style_text_font(sg_landscape_ui.ui.chat_mode_label, sg_landscape_ui.font.text, 0);
    lv_label_set_text(sg_landscape_ui.ui.chat_mode_label, "");
    lv_obj_align(sg_landscape_ui.ui.chat_mode_label, LV_ALIGN_LEFT_MID, 5, 0);

    // Status label (center)
    sg_landscape_ui.ui.status_label = lv_label_create(sg_landscape_ui.ui.status_bar);
    lv_obj_set_style_text_align(sg_landscape_ui.ui.status_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(sg_landscape_ui.ui.status_label, sg_landscape_ui.theme.text, 0);
    lv_obj_set_style_text_font(sg_landscape_ui.ui.status_label, sg_landscape_ui.font.text, 0);
    lv_label_set_text(sg_landscape_ui.ui.status_label, INITIALIZING);
    lv_obj_align(sg_landscape_ui.ui.status_label, LV_ALIGN_CENTER, 0, 0);

    // Network status (right)
    sg_landscape_ui.ui.network_label = lv_label_create(sg_landscape_ui.ui.status_bar);
    lv_obj_set_style_text_font(sg_landscape_ui.ui.network_label, sg_landscape_ui.font.icon, 0);
    lv_obj_set_style_text_color(sg_landscape_ui.ui.network_label, sg_landscape_ui.theme.text, 0);
    lv_obj_align(sg_landscape_ui.ui.network_label, LV_ALIGN_RIGHT_MID, -5, 0);

    // Notification label (hidden by default)
    sg_landscape_ui.ui.notification_label = lv_label_create(sg_landscape_ui.ui.status_bar);
    lv_obj_set_style_text_align(sg_landscape_ui.ui.notification_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(sg_landscape_ui.ui.notification_label, sg_landscape_ui.theme.text, 0);
    lv_obj_set_style_text_font(sg_landscape_ui.ui.notification_label, sg_landscape_ui.font.text, 0);
    lv_label_set_text(sg_landscape_ui.ui.notification_label, "");
    lv_obj_align(sg_landscape_ui.ui.notification_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_flag(sg_landscape_ui.ui.notification_label, LV_OBJ_FLAG_HIDDEN);
}

/**
 * @brief Notification timer callback
 */
static void __notification_timer_cb(lv_timer_t *timer)
{
    lv_obj_add_flag(sg_landscape_ui.ui.notification_label, LV_OBJ_FLAG_HIDDEN);
    lv_timer_del(timer);
    sg_landscape_ui.notification_tm = NULL;
}

/**
 * @brief Start playing talking animation
 */
static void __start_talking_animation(void)
{
    // 重新启动GIF播放
    if (sg_landscape_ui.talking_gif) {
        lv_gif_restart(sg_landscape_ui.talking_gif);
    }
}

/**
 * @brief Stop talking animation and show placeholder
 */
static void __stop_talking_animation(void)
{
    // 重置GIF到第一帧并暂停
    if (sg_landscape_ui.talking_gif) {
        lv_gif_restart(sg_landscape_ui.talking_gif); // 重置到第一帧
        lv_gif_pause(sg_landscape_ui.talking_gif);   // 立即暂停
    }
}

/**
 * @brief Initialize landscape UI layout
 */
int ui_init(UI_FONT_T *ui_font)
{
    if (ui_font == NULL) {
        return -1;
    }

    memcpy(&sg_landscape_ui.font, ui_font, sizeof(UI_FONT_T));
    __landscape_theme_init(&sg_landscape_ui.theme);

    // Main container (full screen)
    sg_landscape_ui.ui.main_container = lv_obj_create(lv_scr_act());
    lv_obj_set_size(sg_landscape_ui.ui.main_container, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style_bg_color(sg_landscape_ui.ui.main_container, sg_landscape_ui.theme.background, 0);
    lv_obj_set_style_border_width(sg_landscape_ui.ui.main_container, 0, 0);
    lv_obj_set_style_pad_all(sg_landscape_ui.ui.main_container, 0, 0);

    // Content container (below status bar)
    sg_landscape_ui.ui.content_container = lv_obj_create(sg_landscape_ui.ui.main_container);
    lv_obj_set_size(sg_landscape_ui.ui.content_container, LV_HOR_RES, LV_VER_RES - STATUS_BAR_HEIGHT);
    lv_obj_set_pos(sg_landscape_ui.ui.content_container, 0, STATUS_BAR_HEIGHT);
    lv_obj_set_style_bg_opa(sg_landscape_ui.ui.content_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(sg_landscape_ui.ui.content_container, 0, 0);
    lv_obj_set_style_pad_all(sg_landscape_ui.ui.content_container, PANEL_PADDING, 0);

    // Create UI components
    __create_status_bar();
    __create_video_panel();
    __create_chat_panel();

    return 0;
}

/**
 * @brief Set user message in chat panel
 */
void ui_set_user_msg(const char *text)
{
    if (sg_landscape_ui.ui.chat_message_label == NULL || text == NULL) {
        return;
    }

    lv_label_set_text(sg_landscape_ui.ui.chat_message_label, text);
    lv_obj_set_style_bg_color(sg_landscape_ui.ui.chat_message_label, sg_landscape_ui.theme.user_bubble, 0);
    lv_obj_set_style_text_color(sg_landscape_ui.ui.chat_message_label, sg_landscape_ui.theme.text, 0);
}

/**
 * @brief Set assistant message in chat panel
 */
void ui_set_assistant_msg(const char *text)
{
    if (sg_landscape_ui.ui.chat_message_label == NULL || text == NULL) {
        return;
    }

    // 只设置文字，不控制动画（动画由TTS语音播放控制）
    lv_label_set_text(sg_landscape_ui.ui.chat_message_label, text);
    lv_obj_set_style_bg_color(sg_landscape_ui.ui.chat_message_label, sg_landscape_ui.theme.assistant_bubble, 0);
    lv_obj_set_style_text_color(sg_landscape_ui.ui.chat_message_label, sg_landscape_ui.theme.text, 0);
    
    // 自动滚动到底部
    lv_obj_scroll_to_y(sg_landscape_ui.ui.chat_scroll_area, LV_COORD_MAX, LV_ANIM_ON);
}

/**
 * @brief Set system message in chat panel
 */
void ui_set_system_msg(const char *text)
{
    if (sg_landscape_ui.ui.chat_message_label == NULL || text == NULL) {
        return;
    }

    lv_label_set_text(sg_landscape_ui.ui.chat_message_label, text);
    lv_obj_set_style_bg_color(sg_landscape_ui.ui.chat_message_label, sg_landscape_ui.theme.system_bubble, 0);
    lv_obj_set_style_text_color(sg_landscape_ui.ui.chat_message_label, sg_landscape_ui.theme.text, 0);
}

/**
 * @brief Set emotion display
 */
void ui_set_emotion(const char *emotion)
{
    // 横屏模式下已移除表情显示
    (void)emotion;
}

/**
 * @brief Set status text
 */
void ui_set_status(const char *status)
{
    if (sg_landscape_ui.ui.status_label == NULL || status == NULL) {
        return;
    }

    lv_label_set_text(sg_landscape_ui.ui.status_label, status);
}

/**
 * @brief Set notification text
 */
void ui_set_notification(const char *notification)
{
    if (sg_landscape_ui.ui.notification_label == NULL || notification == NULL) {
        return;
    }

    lv_label_set_text(sg_landscape_ui.ui.notification_label, notification);
    lv_obj_clear_flag(sg_landscape_ui.ui.notification_label, LV_OBJ_FLAG_HIDDEN);

    // Hide notification after 3 seconds
    if (sg_landscape_ui.notification_tm) {
        lv_timer_del(sg_landscape_ui.notification_tm);
    }
    sg_landscape_ui.notification_tm = lv_timer_create(__notification_timer_cb, 3000, NULL);
}

/**
 * @brief Set network status icon
 */
void ui_set_network(char *wifi_icon)
{
    if (sg_landscape_ui.ui.network_label == NULL || wifi_icon == NULL) {
        return;
    }

    lv_label_set_text(sg_landscape_ui.ui.network_label, wifi_icon);
}

/**
 * @brief Set chat mode text
 */
void ui_set_chat_mode(const char *chat_mode)
{
    if (sg_landscape_ui.ui.chat_mode_label == NULL || chat_mode == NULL) {
        return;
    }

    lv_label_set_text(sg_landscape_ui.ui.chat_mode_label, chat_mode);
}

/**
 * @brief Set status bar padding (compatibility function)
 */
void ui_set_status_bar_pad(int32_t value)
{
    // Not needed in landscape layout
    (void)value;
}

/**
 * @brief Stop talking animation (public function)
 */
void ui_stop_talking_animation(void)
{
    __stop_talking_animation();
}

/**
 * @brief Start talking animation (public function)
 */
void ui_start_talking_animation(void)
{
    __start_talking_animation();
}

#if defined(ENABLE_GUI_STREAM_AI_TEXT) && (ENABLE_GUI_STREAM_AI_TEXT == 1)
void ui_set_assistant_msg_stream_start(void)
{
    if (sg_landscape_ui.ui.chat_message_label == NULL) {
        return;
    }

    // 只清空文字，动画由TTS语音播放控制
    lv_label_set_text(sg_landscape_ui.ui.chat_message_label, "");
    lv_obj_set_style_bg_color(sg_landscape_ui.ui.chat_message_label, sg_landscape_ui.theme.assistant_bubble, 0);
}

void ui_set_assistant_msg_stream_data(const char *text)
{
    if (sg_landscape_ui.ui.chat_message_label == NULL || text == NULL) {
        return;
    }

    const char *current_text = lv_label_get_text(sg_landscape_ui.ui.chat_message_label);
    char *new_text = lv_mem_alloc(strlen(current_text) + strlen(text) + 1);
    if (new_text) {
        strcpy(new_text, current_text);
        strcat(new_text, text);
        lv_label_set_text(sg_landscape_ui.ui.chat_message_label, new_text);
        lv_mem_free(new_text);
        
        // 自动滚动到底部，显示最新文字
        lv_obj_scroll_to_y(sg_landscape_ui.ui.chat_scroll_area, LV_COORD_MAX, LV_ANIM_ON);
    }
}

void ui_set_assistant_msg_stream_end(void)
{
    // 文字流结束，但动画由TTS语音播放控制
}
#endif

#endif // ENABLE_GUI_LANDSCAPE
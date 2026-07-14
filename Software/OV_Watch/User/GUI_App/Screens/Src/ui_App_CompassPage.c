#include "../Inc/ui_App_CompassPage.h"
#include "HWDataAccess.h"
#include <math.h>
#include <stdint.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

lv_obj_t * ui_App_CompassPage = NULL;
Page_t Page_App_Compass = {ui_App_CompassPage_screen_init, ui_App_CompassPage_deinit, &ui_App_CompassPage};

static lv_obj_t *needle_line = NULL;
static lv_obj_t *degree_label = NULL;
static lv_timer_t *compass_timer = NULL;
static lv_point_t needle_pts[2];

#define COMPASS_CX  120
#define COMPASS_CY  130
#define NEEDLE_LEN  60

static const char* heading_to_direction(int deg)
{
    if (deg < 23 || deg >= 338) return "N";
    if (deg < 68)  return "NE";
    if (deg < 113) return "E";
    if (deg < 158) return "SE";
    if (deg < 203) return "S";
    if (deg < 248) return "SW";
    if (deg < 293) return "W";
    return "NW";
}

static void compass_timer_cb(lv_timer_t *timer)
{
    LV_UNUSED(timer);
    #if HW_USE_LSM303
    int16_t Xa, Ya, Za, Xm, Ym, Zm;
    HWInterface.Ecompass.ReadAccel(&Xa, &Ya, &Za);
    HWInterface.Ecompass.ReadMag(&Xm, &Ym, &Zm);
    int heading = HWInterface.Ecompass.CalcAzimuth(Xa, Ya, Za, Xm, Ym, Zm);

    if (heading < 0) heading = 0;
    if (heading >= 360) heading = 359;

    /* Update needle */
    double rad = heading * M_PI / 180.0;
    needle_pts[0].x = COMPASS_CX;
    needle_pts[0].y = COMPASS_CY;
    needle_pts[1].x = COMPASS_CX + (int16_t)(NEEDLE_LEN * sin(rad));
    needle_pts[1].y = COMPASS_CY - (int16_t)(NEEDLE_LEN * cos(rad));
    lv_line_set_points(needle_line, needle_pts, 2);

    /* Update text */
    const char *dir = heading_to_direction(heading);
    lv_label_set_text_fmt(degree_label, "%d° %s", heading, dir);
    #endif
}

static void ui_event_App_CompassPage(lv_event_t *e)
{
    lv_event_code_t event_code = lv_event_get_code(e);

    if(event_code == LV_EVENT_GESTURE &&  lv_indev_get_gesture_dir(lv_indev_get_act()) == LV_DIR_RIGHT) {
        lv_indev_wait_release(lv_indev_get_act());
        Page_Back(LV_SCR_LOAD_ANIM_MOVE_RIGHT, 250, 0);
    }
}

void ui_App_CompassPage_screen_init(void)
{
    ui_App_CompassPage = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_App_CompassPage, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(ui_App_CompassPage, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_add_event_cb(ui_App_CompassPage, ui_event_App_CompassPage, LV_EVENT_ALL, NULL);

    lv_obj_t *title = lv_label_create(ui_App_CompassPage);
    lv_label_set_text(title, "Compass");
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 12);

    /* Cardinal direction labels */
    lv_obj_t *n_label = lv_label_create(ui_App_CompassPage);
    lv_label_set_text(n_label, "N");
    lv_obj_set_style_text_color(n_label, lv_color_hex(0xFF0000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_align(n_label, LV_ALIGN_CENTER, 0, -70);

    lv_obj_t *s_label = lv_label_create(ui_App_CompassPage);
    lv_label_set_text(s_label, "S");
    lv_obj_set_style_text_color(s_label, lv_color_hex(0x888888), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_align(s_label, LV_ALIGN_CENTER, 0, 70);

    lv_obj_t *e_label = lv_label_create(ui_App_CompassPage);
    lv_label_set_text(e_label, "E");
    lv_obj_set_style_text_color(e_label, lv_color_hex(0x888888), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_align(e_label, LV_ALIGN_CENTER, 70, 0);

    lv_obj_t *w_label = lv_label_create(ui_App_CompassPage);
    lv_label_set_text(w_label, "W");
    lv_obj_set_style_text_color(w_label, lv_color_hex(0x888888), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_align(w_label, LV_ALIGN_CENTER, -70, 0);

    /* Needle */
    needle_line = lv_line_create(ui_App_CompassPage);
    lv_obj_set_style_line_color(needle_line, lv_color_hex(0xFF0000), 0);
    lv_obj_set_style_line_width(needle_line, 3, 0);
    needle_pts[0].x = COMPASS_CX;
    needle_pts[0].y = COMPASS_CY;
    needle_pts[1].x = COMPASS_CX;
    needle_pts[1].y = COMPASS_CY - NEEDLE_LEN;
    lv_line_set_points(needle_line, needle_pts, 2);

    /* Center dot */
    lv_obj_t *dot = lv_obj_create(ui_App_CompassPage);
    lv_obj_set_size(dot, 6, 6);
    lv_obj_set_style_bg_color(dot, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_radius(dot, 3, 0);
    lv_obj_set_style_border_width(dot, 0, 0);
    lv_obj_align(dot, LV_ALIGN_CENTER, 0, 0);

    /* Degree label */
    degree_label = lv_label_create(ui_App_CompassPage);
    lv_label_set_text(degree_label, "0° N");
    lv_obj_set_style_text_color(degree_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(degree_label, &lv_font_montserrat_20, 0);
    lv_obj_align(degree_label, LV_ALIGN_BOTTOM_MID, 0, -20);

    #if HW_USE_LSM303
    HWInterface.Ecompass.Wakeup();
    #endif
    compass_timer = lv_timer_create(compass_timer_cb, 200, NULL);
}

void ui_App_CompassPage_deinit(void)
{
    if (compass_timer) {
        lv_timer_del(compass_timer);
        compass_timer = NULL;
    }
    #if HW_USE_LSM303
    HWInterface.Ecompass.Sleep();
    
    #endif
    ui_App_CompassPage = NULL;
    needle_line = NULL;
    degree_label = NULL;
}

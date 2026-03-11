#include "app_ui_lvgl_manager.h"
#include "esp_lv_adapter.h"

// 坡度
float TFT_GO_Start_Screen_Incline_Get(void)
{
    const char *text = lv_label_get_text(objects.go_incline_value);
    return (text != NULL) ? atof(text) : 0.0f;
}

void TFT_GO_Start_Screen_Incline_Set(float incline_value)
{
    if (esp_lv_adapter_lock(-1) == ESP_OK)
    {
        lv_label_set_text_fmt(objects.go_incline_value, "%.1f", incline_value);
        esp_lv_adapter_unlock();
    }
}

// 速度
float TFT_GO_Start_Screen_Speed_Get(void)
{
    const char *text = lv_label_get_text(objects.go_speed_value);
    return (text != NULL) ? atof(text) : 0.0f;
}

void TFT_GO_Start_Screen_Speed_Set(float speed_value)
{
    if (esp_lv_adapter_lock(-1) == ESP_OK)
    {
        lv_label_set_text_fmt(objects.go_speed_value, "%.1f", speed_value);
        esp_lv_adapter_unlock();
    }
}

// 心跳
uint8_t TFT_GO_Start_Screen_HeartRate_Get(void)
{
    const char *text = lv_label_get_text(objects.go_heart_rate_value);
    if (text == NULL)
    {
        return 0;
    }

    unsigned int value = 0;
    sscanf(text, "%u", &value);
    return (uint8_t)value;
}

void TFT_GO_Start_Screen_HeartRate_Set(uint8_t heart_rate)
{
    if (esp_lv_adapter_lock(-1) == ESP_OK)
    {
        lv_label_set_text_fmt(objects.go_heart_rate_value, "%hhu", heart_rate);
        // lv_label_set_text(objects.go_heart_rate_value, "-");
        esp_lv_adapter_unlock();
    }
}

// 卡路里
uint16_t TFT_GO_Start_Screen_Calorie_Get(void)
{
    const char *text = lv_label_get_text(objects.go_calories_value);
    if (text == NULL)
    {
        return 0;
    }

    unsigned int value = 0;
    sscanf(text, "%u", &value);
    return (uint16_t)value;
}

void TFT_GO_Start_Screen_Calorie_Set(uint16_t calorie)
{
    if (esp_lv_adapter_lock(-1) == ESP_OK)
    {
        lv_label_set_text_fmt(objects.go_calories_value, "%hu", calorie);
        esp_lv_adapter_unlock();
    }
}

// 瓦特
uint16_t TFT_GO_Start_Screen_Watt_Get(void)
{
    const char *text = lv_label_get_text(objects.go_watts_value);
    if (text == NULL)
    {
        return 0;
    }

    unsigned int value = 0;
    sscanf(text, "%u", &value);
    return (uint16_t)value;
}

void TFT_GO_Start_Screen_Watt_Set(uint16_t watt)
{
    if (esp_lv_adapter_lock(-1) == ESP_OK)
    {
        lv_label_set_text_fmt(objects.go_watts_value, "%hu", watt);
        esp_lv_adapter_unlock();
    }
}

// step
uint16_t TFT_GO_Start_Screen_Step_Get(void)
{
    const char *text = lv_label_get_text(objects.go_step_value);
    if (text == NULL)
    {
        return 0;
    }

    unsigned int value = 0;
    sscanf(text, "%u", &value);
    return (uint16_t)value;
}

void TFT_GO_Start_Screen_Step_Set(uint16_t step)
{
    if (esp_lv_adapter_lock(-1) == ESP_OK)
    {
        lv_label_set_text_fmt(objects.go_step_value, "%hu", step);
        esp_lv_adapter_unlock();
    }
}

// 时间显示
void TFT_GO_Start_Screen_Time_Set(uint8_t part1, uint8_t part2)
{
    if (esp_lv_adapter_lock(-1) == ESP_OK)
    {
        lv_label_set_text_fmt(objects.go_speed_time, "%02hhu:%02hhu", part1, part2);
        esp_lv_adapter_unlock();
    }
}

// 3、2、1依次倒数字符大小改变
void TFT_Start_Count_Number_Screen_Countdown(void)
{
    // if (esp_lv_adapter_lock(-1) == ESP_OK)
    // {
    lv_obj_set_style_text_font(objects.start_count_number_label_1, &ui_font_bold_320, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_center(objects.start_count_number_label_1);
    // esp_lv_adapter_unlock();
    // }
    // TFT_AUDIO_Player_Play_From_File(MOUNT_POINT "/pcm/%04d.pcm", THREE_AUDIO, false); // 语音播报
    vTaskDelay(pdMS_TO_TICKS(1000));
    // if (esp_lv_adapter_lock(-1) == ESP_OK)
    // {
    lv_obj_set_style_text_font(objects.start_count_number_label_1, &ui_font_bold_180, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_center(objects.start_count_number_label_1);
    // lv_timer_handler();

    lv_obj_set_style_text_font(objects.start_count_number_label_2, &ui_font_bold_320, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_center(objects.start_count_number_label_2);
    // esp_lv_adapter_unlock();
    // }
    // TFT_AUDIO_Player_Play_From_File(MOUNT_POINT "/pcm/%04d.pcm", TWO_AUDIO, true); // 语音播报
    vTaskDelay(pdMS_TO_TICKS(1000));
    // if (esp_lv_adapter_lock(-1) == ESP_OK)
    // {
    lv_obj_set_style_text_font(objects.start_count_number_label_2, &ui_font_bold_180, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_center(objects.start_count_number_label_2);
    // lv_timer_handler();

    lv_obj_set_style_text_font(objects.start_count_number_label_3, &ui_font_bold_320, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_center(objects.start_count_number_label_3);
    // lv_timer_handler();
    // esp_lv_adapter_unlock();
    // }
    // TFT_AUDIO_Player_Play_From_File(MOUNT_POINT "/pcm/%04d.pcm", ONE_AUDIO, false); // 语音播报
    vTaskDelay(pdMS_TO_TICKS(1000));
    // if (esp_lv_adapter_lock(-1) == ESP_OK)
    // {
    lv_obj_set_style_text_font(objects.start_count_number_label_3, &ui_font_bold_180, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_center(objects.start_count_number_label_3);
    // lv_timer_handler();
    // esp_lv_adapter_unlock();
    // }
}

// go_start页面，当按下暂停按键时，改变页面透明度，然后页面中间出现一个暂停图标
void TFT_GO_Start_Screen_Stop(void)
{
    if (esp_lv_adapter_lock(-1) == ESP_OK)
    {
        lv_obj_set_style_opa(objects.go_main_panel, 100, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_clear_flag(objects.go_paused, LV_OBJ_FLAG_HIDDEN);
        esp_lv_adapter_unlock();
    }
}

void TFT_GO_Start_Screen_Rstart(void)
{
    if (esp_lv_adapter_lock(-1) == ESP_OK)
    {
        lv_obj_set_style_opa(objects.go_main_panel, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_add_flag(objects.go_paused, LV_OBJ_FLAG_HIDDEN);
        esp_lv_adapter_unlock();
    }
}

// 在Display_Set_UI.c文件中将下面这个函数替换下：
void TFT_Landmarks_Select_Screen_Change(LandMark_Index_e landmark)
{
    if (esp_lv_adapter_lock(-1) == ESP_OK)
    {

        switch (landmark)
        {
        case 0:
        {
            lv_image_set_src(objects.landmarks_img, &img_elcastillo);
            lv_label_set_text(objects.landmarks_name, "Elcastillo");
            lv_label_set_text(objects.landmarks_floors, "10 Floors");
            break;
        }
        case 1:
        {
            lv_image_set_src(objects.landmarks_img, &img_sydney_opera_house);
            lv_label_set_text(objects.landmarks_name, "Sydney Opera House");
            lv_label_set_text(objects.landmarks_floors, "21 Floors");
            break;
        }
        default:
            break;
        }
        lv_obj_center(objects.landmarks_img);
        esp_lv_adapter_unlock();
    }
}
void TFT_Display_Change_Page(Page_Index_e page)
{
    if (esp_lv_adapter_lock(-1) == ESP_OK)
    {
        // ESP_LOGI("TFT_Display_Change_Page", "esp_lv_adapter_lock ");
        switch (page)
        {
        case Home_Screen:
            lv_scr_load(objects.home_screen);
            break;
        case Go_Start_Screen:
            lv_scr_load(objects.go_start_screen);
            break;
        case Summary_Screen:
            lv_scr_load(objects.summary_screen);
            break;
        case Language_Select_Screen:
            lv_scr_load(objects.language_select_screen);
            break;
        case Landmarks_Select_Screen:
            lv_scr_load(objects.landmarks_select_screen);
            break;
        case Programs_Select_Screen:
            lv_scr_load(objects.programs_select_screen);
            break;
        case Fitness_Test_Select_Screen:
            lv_scr_load(objects.fitness_test_select_screen);
            break;
        case Start_Count_Number_Screen:
            lv_scr_load(objects.start_count_number_screen);
            break;
        case Level_Set_Screen:
            lv_scr_load(objects.level_set_screen);
            break;
        case Time_Set_Screen:
            lv_scr_load(objects.time_set_screen);
            break;
        case Weight_Set_Screen:
            lv_scr_load(objects.weight_set_screen);
            break;
        case Gender_Set_Screen:
            lv_scr_load(objects.gender_set_screen);
            break;
        case Age_Set_Screen:
            lv_scr_load(objects.age_set_screen);
            break;
        case Heart_Rate_Remind_Screen:
            lv_scr_load(objects.heart_rate_remind_screen);
            break;
        case Target_HR_Screen:
            lv_scr_load(objects.target_hr_screen);
            break;
        case BT_Pair_Screen:
            lv_scr_load(objects.bt_pair_screen);
            break;
        case BT_Heart_Rate_Monitor_Screen:
            lv_scr_load(objects.bt_heart_rate_monitor_screen);
            break;
        case BT_Searching_Screen:
            lv_scr_load(objects.bt_searching_screen);
            break;
        case USB_Updata_Progress_Screen:
            lv_scr_load(objects.usb_updata_progress_screen);
            break;
        default:
            break;
        }
        // ESP_LOGI("TFT_Display_Change_Page", "esp_lv_adapter_unlock ");
        esp_lv_adapter_unlock();
    }
}

// 地标选择界面,左右两边的上三角变灰函数(变灰就是提示用户按不了了)
void TFT_Landmarks_Select_Screen_UP_Arrow_Disable(void)
{
    if (esp_lv_adapter_lock(-1) == ESP_OK)
    {
        lv_obj_set_style_image_opa(objects.landmarks_select_left_up_arrow, 100, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_image_opa(objects.landmarks_select_right_up_arrow, 100, LV_PART_MAIN | LV_STATE_DEFAULT);
        esp_lv_adapter_unlock();
    }
}

void TFT_Landmarks_Select_Screen_UP_Arrow_Enable(void)
{
    if (esp_lv_adapter_lock(-1) == ESP_OK)
    {
        lv_obj_set_style_image_opa(objects.landmarks_select_left_up_arrow, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_image_opa(objects.landmarks_select_right_up_arrow, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
        esp_lv_adapter_unlock();
    }
}

// summary总结界面,左右两边的上三角变灰函数(变灰就是提示用户按不了了)
void TFT_Summary_Screen_UP_Arrow_Disable(void)
{
    if (esp_lv_adapter_lock(-1) == ESP_OK)
    {
        lv_obj_set_style_image_opa(objects.summary_left_up_arrow, 100, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_image_opa(objects.summary_right_up_arrow, 100, LV_PART_MAIN | LV_STATE_DEFAULT);
        esp_lv_adapter_unlock();
    }
}

void TFT_Summary_Screen_UP_Arrow_Enable(void)
{
    if (esp_lv_adapter_lock(-1) == ESP_OK)
    {
        lv_obj_set_style_image_opa(objects.summary_left_up_arrow, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_image_opa(objects.summary_right_up_arrow, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
        esp_lv_adapter_unlock();
    }
}

// 语言选择界面,左右两边的上三角变灰函数(变灰就是提示用户按不了了)
void TFT_Language_Select_Screen_UP_Arrow_Disable(void)
{
    if (esp_lv_adapter_lock(-1) == ESP_OK)
    {
        lv_obj_set_style_image_opa(objects.language_select_left_up_arrow, 100, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_image_opa(objects.language_select_right_up_arrow, 100, LV_PART_MAIN | LV_STATE_DEFAULT);
        esp_lv_adapter_unlock();
    }
}

void TFT_Language_Select_Screen_UP_Arrow_Enable(void)
{
    if (esp_lv_adapter_lock(-1) == ESP_OK)
    {
        lv_obj_set_style_image_opa(objects.language_select_left_up_arrow, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_image_opa(objects.language_select_right_up_arrow, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
        esp_lv_adapter_unlock();
    }
}

// 程序选择界面,左右两边的上三角变灰函数(变灰就是提示用户按不了了)
void TFT_Programs_Select_Screen_UP_Arrow_Disable(void)
{
    if (esp_lv_adapter_lock(-1) == ESP_OK)
    {
        lv_obj_set_style_image_opa(objects.programs_select_left_up_arrow, 100, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_image_opa(objects.programs_select_right_up_arrow, 100, LV_PART_MAIN | LV_STATE_DEFAULT);
        esp_lv_adapter_unlock();
    }
}

void TFT_Programs_Select_Screen_UP_Arrow_Enable(void)
{
    if (esp_lv_adapter_lock(-1) == ESP_OK)
    {
        lv_obj_set_style_image_opa(objects.programs_select_left_up_arrow, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_image_opa(objects.programs_select_right_up_arrow, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
        esp_lv_adapter_unlock();
    }
}

// 语言选择界面,滚轮选中项索引设置和获取
void TFT_Roller_Language_Select_Set(Language_Index_e language)
{
    if (esp_lv_adapter_lock(-1) == ESP_OK)
    {
        lv_roller_set_selected(objects.language_select_roller, language, LV_ANIM_ON);
        esp_lv_adapter_unlock();
    }
}

Language_Index_e TFT_Roller_Language_Select_Get(void)
{
    return (Language_Index_e)lv_roller_get_selected(objects.language_select_roller);
}

// 程序选择界面,滚轮选中项索引设置和获取
void TFT_Roller_Programe_Select_Set(Programe_Index_e program)
{
    if (esp_lv_adapter_lock(-1) == ESP_OK)
    {
        lv_roller_set_selected(objects.programs_select_roller, program, LV_ANIM_ON);
        esp_lv_adapter_unlock();
    }
}

Programe_Index_e TFT_Roller_Programe_Select_Get(void)
{
    return (Programe_Index_e)lv_roller_get_selected(objects.programs_select_roller);
}

// fitness test界面,滚轮选中项索引设置和获取
void TFT_Roller_Fitness_Select_Set(Fitness_Test_Index_e fitness_test)
{
    if (esp_lv_adapter_lock(-1) == ESP_OK)
    {
        lv_roller_set_selected(objects.fitness_test_select_roller, fitness_test, LV_ANIM_ON);
        esp_lv_adapter_unlock();
    }
}

Fitness_Test_Index_e TFT_Roller_Fitness_Select_Get(void)
{
    return (Fitness_Test_Index_e)lv_roller_get_selected(objects.fitness_test_select_roller);
}

// USB升级进度
void TFT_USB_Updata_Progress(uint8_t progress)
{
    if (esp_lv_adapter_lock(-1) == ESP_OK)
    {
        lv_arc_set_value(objects.usb_updata_arc, progress);
        lv_label_set_text_fmt(objects.usb_updata_label, "%d%%", progress);
        if (progress < 100)
        {
            lv_label_set_text(objects.usb_updata_finish_label, "Updating");
        }
        else if (progress == 100)
        {
            lv_label_set_text(objects.usb_updata_finish_label, "Update is done\nTurn the machine off add on");
        }

        esp_lv_adapter_unlock();
    }
}



#pragma once

#include "screens.h"
#include "stdio.h"
#include "stdlib.h"
#include "fonts.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "images.h"
#include "esp_err.h"
#include "esp_check.h"
#include "esp_log.h"


typedef enum
{
    Elcastillo,
    Sydney_Opera_House,
} LandMark_Index_e; // 不同地标的索引

typedef enum
{
    Home_Screen,
    Go_Start_Screen,
    Summary_Screen,
    Language_Select_Screen,
    Landmarks_Select_Screen,
    Programs_Select_Screen,
    Fitness_Test_Select_Screen,
    Start_Count_Number_Screen,
    Level_Set_Screen,
    Time_Set_Screen,
    Weight_Set_Screen,
    Height_Set_Screen,
    Gender_Set_Screen,
    Age_Set_Screen,
    Heart_Rate_Remind_Screen,
    Target_HR_Screen,
    BT_Pair_Screen,
    BT_Heart_Rate_Monitor_Screen,
    BT_Searching_Screen,
    USB_Updata_Progress_Screen
} Page_Index_e; // 不同页面的索引

typedef enum
{
    English,
    Deutsoh,
    Mederlands,
    Franoais,
    Espanol,
    Italiano,
    Polski,
    Portugues,
    Svenska,
    Suomi,
    Turkoe,
    Dansk,
    Pyooknn
} Language_Index_e; // 语言索引

typedef enum
{
    Manual,
    Landmarks,
    Heart_Rate,
    Fat_Burn,
    Rolling_Hills,
    Intervals,
    Fitness_Test
} Programe_Index_e; // 程序索引

typedef enum
{
    Sub_Max,
    WFI,
    CPAT
} Fitness_Test_Index_e;

// 坡度
float TFT_GO_Start_Screen_Incline_Get(void);
void TFT_GO_Start_Screen_Incline_Set(float incline_value);
// 速度
float TFT_GO_Start_Screen_Speed_Get(void);
void TFT_GO_Start_Screen_Speed_Set(float speed_value);
// 心率
uint8_t TFT_GO_Start_Screen_HeartRate_Get(void);
void TFT_GO_Start_Screen_HeartRate_Set(uint8_t heart_rate);
// 卡路里
uint16_t TFT_GO_Start_Screen_Calorie_Get(void);
void TFT_GO_Start_Screen_Calorie_Set(uint16_t calorie);
// 瓦特
uint16_t TFT_GO_Start_Screen_Watt_Get(void);
void TFT_GO_Start_Screen_Watt_Set(uint16_t watt);
// step
uint16_t TFT_GO_Start_Screen_Step_Get(void);
void TFT_GO_Start_Screen_Step_Set(uint16_t step);
// 时间
void TFT_GO_Start_Screen_Time_Set(uint8_t part1, uint8_t part2);
// 321倒计时
void TFT_Start_Count_Number_Screen_Countdown(void);
// 地标切换
void TFT_Landmarks_Select_Screen_Change(LandMark_Index_e landmark);
// go start页面开始和暂停函数
void TFT_GO_Start_Screen_Stop(void);
void TFT_GO_Start_Screen_Rstart(void);
// 页面切换函数
void TFT_Display_Change_Page(Page_Index_e page);
// 地标选择界面,左右两边上三角按钮失能使能函数
void TFT_Landmarks_Select_Screen_UP_Arrow_Disable(void);
void TFT_Landmarks_Select_Screen_UP_Arrow_Enable(void);
// Summary界面,左右两边上三角按钮失能使能函数
void TFT_Summary_Screen_UP_Arrow_Disable(void);
void TFT_Summary_Screen_UP_Arrow_Enable(void);
// 语言选择界面,左右两边上三角按钮失能使能函数
void TFT_Language_Select_Screen_UP_Arrow_Disable(void);
void TFT_Language_Select_Screen_UP_Arrow_Enable(void);
// 程序选择界面,左右两边上三角按钮失能使能函数
void TFT_Programs_Select_Screen_UP_Arrow_Disable(void);
void TFT_Programs_Select_Screen_UP_Arrow_Enable(void);
// 语言选择界面,滚轮索引设置和获取
void TFT_Roller_Language_Select_Set(Language_Index_e language);
Language_Index_e TFT_Roller_Language_Select_Get(void);
// 程序选择界面,滚轮索引设置和获取
void TFT_Roller_Programe_Select_Set(Programe_Index_e program);
Programe_Index_e TFT_Roller_Programe_Select_Get(void);
// Fitness Test界面,滚轮索引设置和获取
void TFT_Roller_Fitness_Select_Set(Fitness_Test_Index_e fitness_test);
Fitness_Test_Index_e TFT_Roller_Fitness_Select_Get(void);
//USB升级界面函数
void TFT_USB_Updata_Progress(uint8_t progress);


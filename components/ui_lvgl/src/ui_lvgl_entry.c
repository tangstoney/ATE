#include "ui_lvgl.h"

void ui_lvgl_init(void)
{
    lv_obj_t *label = lv_label_create(lv_screen_active());
    lv_label_set_text(label, "Hello from ui_lvgl");
    lv_obj_center(label);

    /*
     * EEZ integration point:
     * 1) Add EEZ-generated .c files to this component's src/.
     * 2) Include EEZ header here (for example: #include "ui.h").
     * 3) Replace this demo label with the EEZ entry call (for example: ui_init()).
     */
}

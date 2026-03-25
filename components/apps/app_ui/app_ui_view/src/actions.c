#include "actions.h"

#include "app_ui_controller.h"

void action_switch_p1(lv_event_t *e)
{
    (void)e;
    (void)app_ui_controller_on_page_changed(APP_UI_PAGE_PAGE1);
}

void action_switch_main(lv_event_t *e)
{
    (void)e;
    (void)app_ui_controller_on_page_changed(APP_UI_PAGE_MAIN);
}

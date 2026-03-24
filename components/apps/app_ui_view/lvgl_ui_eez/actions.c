#include "actions.h"

#include "ui.h"

void action_switch_p1(lv_event_t *e)
{
    (void)e;
    loadScreen(SCREEN_ID_PAGE1);
}

void action_switch_main(lv_event_t *e)
{
    (void)e;
    loadScreen(SCREEN_ID_MAIN);
}

#ifndef _SCREEN_LAUNCH_H_
#define _SCREEN_LAUNCH_H_

#include "dark.h"
#include "typedefs.h"
#include "ui.h"

UIScreen * screen_show_launch();
void handle_event_launch(UIScreen *activeScreen, SDL_Event event);

#endif // _SCREEN_LAUNCH_H_

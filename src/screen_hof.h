#ifndef _SCREEN_HOF_H
#define _SCREEN_HOF_H

#include "dark.h"
#include "typedefs.h"
#include "ui.h"


UIScreen * screen_show_hof();
void handle_event_hof(UIScreen *activeScreen, SDL_Event event);

#endif // _SCREEN_HOF_H_

#ifndef _SCREEN_WINGAME_H_
#define _SCREEN_WINGAME_H_

#include "dark.h"
#include "typedefs.h"
#include "ui.h"

UIScreen * screen_show_win_game();
void handle_event_win(UIScreen *activeScreen, SDL_Event event);

#endif // _SCREEN_WINGAME_H_

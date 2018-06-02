#ifndef _SCREEN_IN_GAME_H_
#define _SCREEN_IN_GAME_H_

#include "dark.h"
#include "typedefs.h"
#include "ui.h"


UIScreen * screen_show_in_game();
void handle_event_in_game(UIScreen *activeScreen, SDL_Event event);

#endif // _SCREEN_IN_GAME_H_

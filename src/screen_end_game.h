#ifndef _SCREEN_END_GAME_H_
#define _SCREEN_END_GAME_H_

#include "dark.h"
#include "typedefs.h"
#include "ui.h"

UIScreen * screen_show_endgame();
void handle_event_endgame(UIScreen *activeScreen, SDL_Event event);


#endif // _SCREEN_END_GAME_H_

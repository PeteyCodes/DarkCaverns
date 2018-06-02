#ifndef _UI_H_
#define _UI_H_

#include <SDL2/SDL.h>

#include "list.h"
#include "typedefs.h"


// Helper macros for working with pixel colors
#define RED(c) ((c & 0xff000000) >> 24)
#define GREEN(c) ((c & 0x00ff0000) >> 16)
#define BLUE(c) ((c & 0x0000ff00) >> 8)
#define ALPHA(c) (c & 0xff)

#define COLOR_FROM_RGBA(r, g, b, a) ((r << 24) | (g << 16) | (b << 8) | a)


/* Console Helper Types */

typedef SDL_Rect UIRect;

typedef struct {
    asciiChar glyph;
    u32 fgColor;
    u32 bgColor;
} ConsoleCell;

typedef struct {
    u32 *atlas;
    u32 atlasWidth;
    u32 atlasHeight;
    u32 charWidth;
    u32 charHeight;
    asciiChar firstCharInAtlas;

    // TODO: Consider chopping the atlas into BitmapImages for each cell
    // TODO: This will optimize the ASCIIfy routine, and may even help with rendering?

} ConsoleFont;

typedef struct {
    u32 *pixels;      // in-memory representation of the screen pixels
    u32 width;
    u32 height;
    u32 rowCount;
    u32 colCount;
    u32 cellWidth;
    u32 cellHeight;
    u32 bgColor;
    bool colorize;
    ConsoleFont *font;
    ConsoleCell *cells;
} Console;


/* Image Types */
typedef struct {
    u32 *pixels;
    u32 width;
    u32 height;    
} BitmapImage;

typedef struct {
    ConsoleCell *cells;
    u32 rows;
    u32 cols;
} AsciiImage;


/* UI Types */
struct UIScreen;
typedef struct UIScreen UIScreen;

typedef void (*UIRenderFunction)(Console *);
typedef void (*UIEventHandler)(struct UIScreen *, SDL_Event);

typedef struct {
    Console *console;
    UIRect *pixelRect;
    UIRenderFunction render;
} UIView;

struct UIScreen {
    List *views;
    UIView *activeView;
    UIEventHandler handle_event;
};

/* UI State */
extern UIScreen *activeScreen;
extern bool asciiMode;


/* 
 *************************************************************************
 ***************************** Interface ********************************* 
 *************************************************************************
 */

/* UI Functions */

UIScreen *
ui_get_active_screen();

void 
ui_set_active_screen(UIScreen *screen);


void 
view_destroy(UIView *view);

UIView * 
view_new(UIRect pixelRect, u32 cellCountX, u32 cellCountY, 
         char *fontFile, asciiChar firstCharInAtlas, u32 bgColor,
         bool colorize, UIRenderFunction renderFn);

void 
view_draw_rect(Console *console, UIRect *rect, u32 color, 
            i32 borderWidth, u32 borderColor);

void
view_draw_image_at(Console *console, BitmapImage *image, i32 cellX, i32 cellY);

void
view_draw_ascii_image_at(Console *console, AsciiImage *image, i32 cellX, i32 cellY);


/* Console Functions */

void 
console_clear(Console *con);

void
console_destroy(Console *con);

Console *
console_new(i32 width, i32 height, i32 rowCount, i32 colCount, u32 bgColor, bool colorize);

void 
console_put_char_at(Console *con, asciiChar c, 
                    i32 cellX, i32 cellY,
                    u32 fgColor, u32 bgColor);

void 
console_put_string_at(Console *con, char *string, 
                      i32 x, i32 y,
                      u32 fgColor, u32 bgColor);

void
console_put_string_in_rect(Console *con, char *string,
                           UIRect rect, bool wrap, 
                           u32 fgColor, u32 bgColor);

void 
console_set_bitmap_font(Console *con, char *filename, 
                        asciiChar firstCharInAtlas,
                        i32 charWidth, i32 charHeight);


/* Image Functions */

AsciiImage*
asciify_bitmap(Console *con, BitmapImage *image);

BitmapImage*
image_load_from_file(char *filename);

BitmapImage *
image_slice(BitmapImage *img, i32 rows, i32 cols);

void 
image_analyze_colors(BitmapImage *image, u32 *primaryColor, u32 *secondaryColor);

BitmapImage *
image_mask_create(BitmapImage *origImage, u32 primaryColor, u32 secondaryColor);

asciiChar
image_match_glyph(Console *console, BitmapImage *maskImage);


/* Utility Functions */

void
ui_copy_blend(u32 *destPixels, UIRect *destRect, u32 destPixelsPerRow,
           u32 *srcPixels, UIRect *srcRect, u32 srcPixelsPerRow,
           bool colorize, u32 *newColor);

void
ui_fill(u32 *pixels, u32 pixelsPerRow, UIRect *destRect, u32 color);

void
ui_fill_blend(u32 *pixels, u32 pixelsPerRow, UIRect *destRect, u32 color);

UIRect 
rect_get_for_glyph(asciiChar c, ConsoleFont *font);


#endif // _UI_H_

/*
    Hybrid Console/GUI Library

    Written by Peter de Tagyos
    Started: 1/28/2017
*/


// Include the STB image library - only the PNG support
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#include "stb_image.h"


// Helper macros for working with pixel colors
#define RED(c) ((c & 0xff000000) >> 24)
#define GREEN(c) ((c & 0x00ff0000) >> 16)
#define BLUE(c) ((c & 0x0000ff00) >> 8)
#define ALPHA(c) (c & 0xff)

#define COLOR_FROM_RGBA(r, g, b, a) ((r << 24) | (g << 16) | (b << 8) | a)


/* Console Helper Types */

typedef unsigned char asciiChar;
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
} ConsoleFont;

typedef struct {
    u32 *pixels;      // in-memory representation of the screen pixels
    u32 width;
    u32 height;
    u32 rowCount;
    u32 colCount;
    u32 cellWidth;
    u32 cellHeight;
    ConsoleFont *font;
    ConsoleCell *cells;
} Console;


/* UI Types */

typedef void (*UIRenderFunction)(Console *);

typedef struct {
    List *views;
} UIScreen;

typedef struct {
    Console *console;
    UIRect *pixelRect;
    UIRenderFunction render;
} UIView;


/* 
 *************************************************************************
 ***************************** Interface ********************************* 
 *************************************************************************
 */

/* UI Functions */

internal void 
view_destroy(UIView *view);

internal UIView * 
view_new(UIRect pixelRect, u32 cellCountX, u32 cellCountY, 
         char *fontFile, asciiChar firstCharInAtlas, 
         UIRenderFunction renderFn);

internal void 
view_draw_rect(Console *console, UIRect *rect, u32 color, 
            i32 borderWidth, u32 borderColor);


/* Console Functions */

internal void 
console_clear(Console *con);

internal void
console_destroy(Console *con);

internal Console *
console_new(i32 width, i32 height, i32 rowCount, i32 colCount);

internal void 
console_put_char_at(Console *con, asciiChar c, 
                    i32 cellX, i32 cellY,
                    u32 fgColor, u32 bgColor);

internal void 
console_put_string_at(Console *con, char *string, 
                      i32 x, i32 y,
                      u32 fgColor, u32 bgColor);

internal void
console_put_string_in_rect(Console *con, char *string,
                           UIRect rect, bool wrap, 
                           u32 fgColor, u32 bgColor);

internal void 
console_set_bitmap_font(Console *con, char *filename, 
                        asciiChar firstCharInAtlas,
                        i32 charWidth, i32 charHeight);


/* Utility Functions */

internal inline u32
ui_colorize_pixel(u32 dest, u32 src); 

internal void
ui_copy_blend(u32 *destPixels, UIRect *destRect, u32 destPixelsPerRow,
           u32 *srcPixels, UIRect *srcRect, u32 srcPixelsPerRow,
           u32 *newColor);

internal void
ui_fill(u32 *pixels, u32 pixelsPerRow, UIRect *destRect, u32 color);

internal void
ui_fill_blend(u32 *pixels, u32 pixelsPerRow, UIRect *destRect, u32 color);

internal UIRect 
rect_get_for_glyph(asciiChar c, ConsoleFont *font);


/* 
 ******************************************************************************
 ***************************** Implementation ********************************* 
 ******************************************************************************
 */

/* Console Function Implementation */

internal void 
console_clear(Console *con) {
    UIRect r = {0, 0, con->width, con->height};
    ui_fill(con->pixels, con->width, &r, 0x000000ff);
}

internal Console *
console_new(i32 width, i32 height, 
            i32 rowCount, i32 colCount) {
    
    Console *con = malloc(sizeof(Console));

    con->pixels = calloc(width * height, sizeof(u32));
    con->width = width;
    con->height = height;
    con->rowCount = rowCount;
    con->colCount = colCount;
    con->cellWidth = width / colCount;
    con->cellHeight = height / rowCount;
    con->font = NULL;
    con->cells = calloc(rowCount * colCount, sizeof(ConsoleCell));

    return con;
}

internal void
console_destroy(Console *con) {
    if (con->pixels) { free(con->pixels); }
    if (con->cells) { free(con->cells); }
    if (con) { free(con); }
}

internal void 
console_put_char_at(Console *con, asciiChar c, 
                    i32 cellX, i32 cellY,
                    u32 fgColor, u32 bgColor) {

    i32 x = cellX * con->cellWidth;
    i32 y = cellY * con->cellHeight;
    UIRect destRect = {x, y, con->cellWidth, con->cellHeight};

    // Fill the background with alpha blending
    ui_fill_blend(con->pixels, con->width, &destRect, bgColor);

    // Copy the glyph with alpha blending and desired coloring
    UIRect srcRect = rect_get_for_glyph(c, con->font);
    ui_copy_blend(con->pixels, &destRect, con->width, 
                 con->font->atlas, &srcRect, con->font->atlasWidth,
                 &fgColor);
}

internal void 
console_put_string_at(Console *con, char *string, 
                      i32 x, i32 y,
                      u32 fgColor, u32 bgColor) {
    i32 len = strlen(string);
    for (i32 i = 0; i < len; i++) {
        console_put_char_at(con, (asciiChar)string[i], x+i, y, fgColor, bgColor);
    }
}

internal void
console_put_string_in_rect(Console *con, char *string,
                           UIRect rect, bool wrap, 
                           u32 fgColor, u32 bgColor) {
    u32 len = strlen(string);
    i32 x = rect.x;
    i32 x2 = x + rect.w;
    i32 y = rect.y;
    i32 y2 = y + rect.h;
    for (u32 i = 0; i < len; i++) {
        bool shouldPut = true;
        if (x >= x2) {
            if (wrap) {
                x = rect.x;
                y += 1;
            } else {
                shouldPut = false;
            }
        }
        if (y >= y2) {
            shouldPut = false;
        }
        if (shouldPut) {
            console_put_char_at(con, (asciiChar)string[i], 
                                  x, y, fgColor, bgColor);
            x += 1;
        }
    }
}

internal void 
console_set_bitmap_font(Console *con, char *filename, 
                        asciiChar firstCharInAtlas,
                        i32 charWidth, i32 charHeight) {

    // Load the image data
    int imgWidth, imgHeight, numComponents;
    unsigned char *imgData = stbi_load(filename, 
                                    &imgWidth, &imgHeight, 
                                    &numComponents, STBI_rgb_alpha);

    // Copy the image data so we can hold onto it
    u32 imgDataSize = imgWidth * imgHeight * sizeof(u32);
    u32 *atlasData = malloc(imgDataSize);
    memcpy(atlasData, imgData, imgDataSize);

    // Create and configure the font
    ConsoleFont *font = malloc(sizeof(ConsoleFont));
    font->atlas = atlasData;
    font->charWidth = charWidth;
    font->charHeight = charHeight;
    font->atlasWidth = imgWidth;
    font->atlasHeight = imgHeight;
    font->firstCharInAtlas = firstCharInAtlas;    

    stbi_image_free(imgData);

    if (con->font != NULL) {
        free(con->font->atlas);
        free(con->font);
    }
    con->font = font;
}


/* UI Function Implementation */

internal UIView * 
view_new(UIRect pixelRect, u32 cellCountX, u32 cellCountY, 
         char *fontFile, asciiChar firstCharInAtlas, 
         UIRenderFunction renderFn) {

    UIView *view = malloc(sizeof(UIView));
    UIRect *rect = malloc(sizeof(UIRect));

    memcpy(rect, &pixelRect, sizeof(UIRect));
    Console *console = console_new(rect->w, rect->h, cellCountY, cellCountX);

    i32 cellWidthPixels = pixelRect.w / cellCountX;
    i32 cellHeightPixels = pixelRect.h / cellCountY;
    console_set_bitmap_font(console, fontFile, firstCharInAtlas, cellWidthPixels, cellHeightPixels);

    view->console = console;
    view->pixelRect = rect;
    view->render = renderFn;

    return view;
}

internal void 
view_destroy(UIView *view) {
    if (view) {
        free(view->pixelRect);
        console_destroy(view->console);
    }
}


/* UI Utility Functions **/

internal void 
view_draw_rect(Console *console, UIRect *rect, u32 color, 
               i32 borderWidth, u32 borderColor)
{
    for (i32 y = rect->y; y < rect->y + rect->h; y++) {
        for (i32 x = rect->x; x < rect->x + rect->w; x++) {
            char c = ' ';
            // If we have a border, then go down that rabbit hole
            if (borderWidth > 0) {               
                // Sides
                if ((x == rect->x) || (x == rect->x + rect->w - 1)) {
                    c = (borderWidth == 1) ? 179 : 186;
                }

                // Top
                if (y == rect->y) {
                    if (x == rect->x) {
                        // Top left corner
                        c = (borderWidth == 1) ? 218 : 201;
                    } else if (x == rect->x + rect->w - 1) {
                        // Top right corner
                        c = (borderWidth == 1) ? 191 : 187;
                    } else {
                        // Top border
                        c = (borderWidth == 1) ? 196 : 205;
                    }
                }
                
                // Bottom
                if (y == rect->y + rect->h - 1) {
                    if (x == rect->x) {
                        // Bottom left corner
                        c = (borderWidth == 1) ? 192 : 200;
                    } else if (x == rect->x + rect->w - 1) {
                        // Bottom right corner
                        c = (borderWidth == 1) ? 217 : 188;
                    } else {
                        // Bottom border
                        c = (borderWidth == 1) ? 196 : 205;
                    }
                }
            }
            console_put_char_at(console, c, x, y, borderColor, color);
        }
    }
}


/* Utility Function Implementation */

internal inline u32
ui_colorize_pixel(u32 dest, u32 src) 
{
    // Colorize the destination pixel using the source color
    if (ALPHA(dest) == 255) {
        return src;
    } else if (ALPHA(dest) > 0) {
        // Scale the final alpha based on both dest & src alphas
        return COLOR_FROM_RGBA(RED(src), 
                               GREEN(src), 
                               BLUE(src), 
                               (u8)(ALPHA(src) * (ALPHA(dest) / 255.0)));
    } else {
        return dest;
    }
}

internal void
ui_copy_blend(u32 *destPixels, UIRect *destRect, u32 destPixelsPerRow,
              u32 *srcPixels, UIRect *srcRect, u32 srcPixelsPerRow,
              u32 *newColor)
{
    // If src and dest rects are not the same size ==> bad things
    assert(destRect->w == srcRect->w && destRect->h == srcRect->h);

    // For each pixel in the destination rect, alpha blend to it the 
    // corresponding pixel in the source rect.
    // ref: https://en.wikipedia.org/wiki/Alpha_compositing

    u32 stopX = destRect->x + destRect->w;
    u32 stopY = destRect->y + destRect->h;

    for (u32 dstY = destRect->y, srcY = srcRect->y; 
         dstY < stopY; 
         dstY++, srcY++) {
        for (u32 dstX = destRect->x, srcX = srcRect->x; 
             dstX < stopX; 
             dstX++, srcX++) {

            u32 srcColor = srcPixels[(srcY * srcPixelsPerRow) + srcX];
            u32 *destPixel = &destPixels[(dstY * destPixelsPerRow) + dstX];
            u32 destColor = *destPixel;

            // Colorize our source pixel before we blend it
            srcColor = ui_colorize_pixel(srcColor, *newColor);

            if (ALPHA(srcColor) == 0) {
                // Source is transparent - so do nothing
                continue;
            } else if (ALPHA(srcColor) == 255) {
                // Just copy the color, no blending necessary
                *destPixel = srcColor;
            } else {
                // Do alpha blending
                float srcA = ALPHA(srcColor) / 255.0;
                float invSrcA = (1.0 - srcA);
                float destA = ALPHA(destColor) / 255.0;

                float outAlpha = srcA + (destA * invSrcA);
                u8 fRed = ((RED(srcColor) * srcA) + (RED(destColor) * destA * invSrcA)) / outAlpha;
                u8 fGreen = ((GREEN(srcColor) * srcA) + (GREEN(destColor) * destA * invSrcA)) / outAlpha;
                u8 fBlue = ((BLUE(srcColor) * srcA) + (BLUE(destColor) * destA * invSrcA)) / outAlpha;
                u8 fAlpha = outAlpha * 255;

                *destPixel = COLOR_FROM_RGBA(fRed, fGreen, fBlue, fAlpha);
            }
        }
    }
}

internal void
ui_fill(u32 *pixels, u32 pixelsPerRow, UIRect *destRect, u32 color)
{
    u32 stopX = destRect->x + destRect->w;
    u32 stopY = destRect->y + destRect->h;

    for (u32 dstY = destRect->y; dstY < stopY; dstY++) {
        for (u32 dstX = destRect->x; dstX < stopX; dstX++) {
            pixels[(dstY * pixelsPerRow) + dstX] = color;
        }
    }
}

internal void
ui_fill_blend(u32 *pixels, u32 pixelsPerRow, UIRect *destRect, u32 color)
{
    // For each pixel in the destination rect, alpha blend the 
    // bgColor to the existing color.
    // ref: https://en.wikipedia.org/wiki/Alpha_compositing

    u32 stopX = destRect->x + destRect->w;
    u32 stopY = destRect->y + destRect->h;

    // If the color we're trying to blend is transparent, then bail
    if (ALPHA(color) == 0) return;

    float srcA = ALPHA(color) / 255.0;
    float invSrcA = 1.0 - srcA;

    // Otherwise, blend each pixel in the dest rect
    for (u32 dstY = destRect->y; dstY < stopY; dstY++) {
        for (u32 dstX = destRect->x; dstX < stopX; dstX++) {
            u32 *pixel = &pixels[(dstY * pixelsPerRow) + dstX];

            if (ALPHA(color) == 255) {
                // Just copy the color, no blending necessary
                *pixel = color;
            } else {
                // Do alpha blending
                u32 pixelColor = *pixel;

                float destA = ALPHA(pixelColor) / 255.0;

                float outAlpha = srcA + (destA * invSrcA);
                u8 fRed = ((RED(color) * srcA) + (RED(pixelColor) * destA * invSrcA)) / outAlpha;
                u8 fGreen = ((GREEN(color) * srcA) + (GREEN(pixelColor) * destA * invSrcA)) / outAlpha;
                u8 fBlue = ((BLUE(color) * srcA) + (BLUE(pixelColor) * destA * invSrcA)) / outAlpha;
                u8 fAlpha = outAlpha * 255;

                *pixel = COLOR_FROM_RGBA(fRed, fGreen, fBlue, fAlpha);
            }
        }
    }
}

internal UIRect 
rect_get_for_glyph(asciiChar c, ConsoleFont *font) {
    i32 idx = c - font->firstCharInAtlas;
    i32 charsPerRow = (font->atlasWidth / font->charWidth);
    i32 xOffset = (idx % charsPerRow) * font->charWidth;
    i32 yOffset = (idx / charsPerRow) * font->charHeight;

    UIRect glyphRect = {xOffset, yOffset, font->charWidth, font->charHeight};
    return glyphRect;
}













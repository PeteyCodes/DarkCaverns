/*
    Console Library

    Written by Peter de Tagyos
    Started: 12/24/2015
*/

#ifndef PT_CONSOLE
#define PT_CONSOLE


typedef unsigned char asciiChar;


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

typedef struct {
    asciiChar glyph;
    u32 fgColor;
    u32 bgColor;
} PT_Cell;

typedef struct {
    u32 *atlas;
    u32 atlasWidth;
    u32 atlasHeight;
    u32 charWidth;
    u32 charHeight;
    asciiChar firstCharInAtlas;
} PT_Font;

typedef struct {
    i32 x; i32 y; i32 w; i32 h;
} PT_Rect;

typedef struct {
    u32 *pixels;      // the screen pixels
    u32 width;
    u32 height;
    u32 rowCount;
    u32 colCount;
    u32 cellWidth;
    u32 cellHeight;
    PT_Font *font;
    PT_Cell *cells;
} PT_Console;


/* Console Functions */

internal void 
PT_ConsoleClear(PT_Console *con);

internal PT_Console *
PT_ConsoleInit(i32 width, i32 height, 
               i32 rowCount, i32 colCount);

internal void 
PT_ConsoleSetBitmapFont(PT_Console *con, char *filename, 
                        asciiChar firstCharInAtlas,
                        int charWidth, int charHeight);

internal void 
PT_ConsolePutCharAt(PT_Console *con, asciiChar c, 
                    i32 cellX, i32 cellY,
                    u32 fgColor, u32 bgColor);


/* Utility Functions */

internal inline u32
PT_ColorizePixel(u32 dest, u32 src); 

internal void
PT_CopyBlend(u32 *destPixels, PT_Rect *destRect, u32 destPixelsPerRow,
             u32 *srcPixels, PT_Rect *srcRect, u32 srcPixelsPerRow,
             u32 *newColor);

internal void
PT_Fill(u32 *pixels, u32 pixelsPerRow, PT_Rect *destRect, u32 color);

internal void
PT_FillBlend(u32 *pixels, u32 pixelsPerRow, PT_Rect *destRect, u32 color);

internal PT_Rect 
PT_RectForGlyph(asciiChar c, PT_Font *font);


/* Console Function Implementation */

internal void 
PT_ConsoleClear(PT_Console *con) {
    PT_Rect r = {0, 0, con->width, con->height};
    PT_Fill(con->pixels, con->width, &r, 0x000000ff);
}

internal PT_Console *
PT_ConsoleInit(i32 width, i32 height, 
               i32 rowCount, i32 colCount) {
    
    PT_Console *con = malloc(sizeof(PT_Console));

    con->pixels = calloc(width * height, sizeof(u32));
    con->width = width;
    con->height = height;
    con->rowCount = rowCount;
    con->colCount = colCount;
    con->cellWidth = width / colCount;
    con->cellHeight = height / rowCount;
    con->font = NULL;
    con->cells = calloc(rowCount * colCount, sizeof(PT_Cell));

    return con;
}

internal void 
PT_ConsolePutCharAt(PT_Console *con, asciiChar c, 
                    i32 cellX, i32 cellY,
                    u32 fgColor, u32 bgColor) {

    i32 x = cellX * con->cellWidth;
    i32 y = cellY * con->cellHeight;
    PT_Rect destRect = {x, y, con->cellWidth, con->cellHeight};

    // Fill the background with alpha blending
    PT_FillBlend(con->pixels, con->width, &destRect, bgColor);

    // Copy the glyph with alpha blending and desired coloring
    PT_Rect srcRect = PT_RectForGlyph(c, con->font);
    PT_CopyBlend(con->pixels, &destRect, con->width, 
                 con->font->atlas, &srcRect, con->font->atlasWidth,
                 &fgColor);
}

internal void 
PT_ConsoleSetBitmapFont(PT_Console *con, char *filename, 
                        asciiChar firstCharInAtlas,
                        int charWidth, int charHeight) {

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
    PT_Font *font = malloc(sizeof(PT_Font));
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


/* Utility Function Implementation */

internal inline u32
PT_ColorizePixel(u32 dest, u32 src) 
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
PT_CopyBlend(u32 *destPixels, PT_Rect *destRect, u32 destPixelsPerRow,
             u32 *srcPixels, PT_Rect *srcRect, u32 srcPixelsPerRow,
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
            srcColor = PT_ColorizePixel(srcColor, *newColor);

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
PT_Fill(u32 *pixels, u32 pixelsPerRow, PT_Rect *destRect, u32 color)
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
PT_FillBlend(u32 *pixels, u32 pixelsPerRow, PT_Rect *destRect, u32 color)
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

internal PT_Rect 
PT_RectForGlyph(asciiChar c, PT_Font *font) {
    i32 idx = c - font->firstCharInAtlas;
    i32 charsPerRow = (font->atlasWidth / font->charWidth);
    i32 xOffset = (idx % charsPerRow) * font->charWidth;
    i32 yOffset = (idx / charsPerRow) * font->charHeight;

    PT_Rect glyphRect = {xOffset, yOffset, font->charWidth, font->charHeight};
    return glyphRect;
}

#endif

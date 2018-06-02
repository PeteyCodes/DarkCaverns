/*
    Hybrid Console/GUI Library

    Written by Peter de Tagyos
    Started: 1/28/2017
*/

#include "ui.h"
#include "util.h"


// Include the STB image library - only the PNG support
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#include "stb_image.h"


#define SWAP_U32(x) (((x) >> 24) | (((x) & 0x00ff0000) >> 8) | (((x) & 0x0000ff00) << 8) | ((x) << 24))


/* UI State */
UIScreen *activeScreen = NULL;
bool asciiMode = true;


/* 
 ******************************************************************************
 ***************************** Implementation ********************************* 
 ******************************************************************************
 */

UIScreen *
ui_get_active_screen() {
    return activeScreen;
}

void 
ui_set_active_screen(UIScreen *screen) {
    if (activeScreen != NULL) { free(activeScreen); }
    activeScreen = screen;
}

/* Console Function Implementation */

void 
console_clear(Console *con) {
    UIRect r = {0, 0, con->width, con->height};
    ui_fill(con->pixels, con->width, &r, con->bgColor);
}

Console *
console_new(i32 width, i32 height, 
            i32 rowCount, i32 colCount,
            u32 bgColor, bool colorize) {
    
    Console *con = calloc(1, sizeof(Console));

    con->pixels = calloc(width * height, sizeof(u32));
    con->width = width;
    con->height = height;
    con->rowCount = rowCount;
    con->colCount = colCount;
    con->cellWidth = width / colCount;
    con->cellHeight = height / rowCount;
    con->font = NULL;
    con->bgColor = bgColor;
    con->colorize = colorize;
    con->cells = calloc(rowCount * colCount, sizeof(ConsoleCell));

    return con;
}

void
console_destroy(Console *con) {
    if (con->pixels) { free(con->pixels); }
    if (con->cells) { free(con->cells); }
    if (con) { free(con); }
}

void 
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
                con->colorize, &fgColor);
}

void 
console_put_string_at(Console *con, char *string, 
                      i32 x, i32 y,
                      u32 fgColor, u32 bgColor) {
    i32 len = strlen(string);
    for (i32 i = 0; i < len; i++) {
        console_put_char_at(con, (asciiChar)string[i], x+i, y, fgColor, bgColor);
    }
}

void
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

void 
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
    u32 *atlasData = calloc(imgDataSize, sizeof(u32));
    memcpy(atlasData, imgData, imgDataSize);

    // Swap endianness of data if we need to
    u32 pixelCount = imgWidth * imgHeight;
    if (system_is_little_endian()) {
        for (u32 i = 0; i < pixelCount; i++) {
            atlasData[i] = SWAP_U32(atlasData[i]);
        }        
    }

    // Turn all black pixels into transparent pixels
    for (u32 i = 0; i < pixelCount; i++) {
        if (atlasData[i] == 0x000000ff) { 
            atlasData[i] = 0x00000000; 
        }
    }        

    // Create and configure the font
    ConsoleFont *font = calloc(1, sizeof(ConsoleFont));
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


/* Image Functions */

AsciiImage*
asciify_bitmap(Console *con, BitmapImage *image) {
    assert(image->height % con->cellHeight == 0);
    assert(image->width % con->cellWidth == 0);

    i32 rows = image->height / con->cellHeight;
    i32 cols = image->width / con->cellWidth;

    AsciiImage *asciiImg = calloc(1, sizeof(AsciiImage));
    asciiImg->cells = calloc(rows * cols, sizeof(ConsoleCell));
    asciiImg->rows = rows;
    asciiImg->cols = cols;

    // Break the given bitmap image into cells
    BitmapImage *cells = image_slice(image, rows, cols);

    // Loop through all cells
    for (i32 r = 0; r < rows; r++) {
        for (i32 c = 0; c < cols; c++) {
            // Analyze each cell to determine primary & secondary colors
            u32 primaryColor = 0;
            u32 secondaryColor = 0;

            BitmapImage *cellImage = &cells[r * cols + c];
            image_analyze_colors(cellImage, &primaryColor, &secondaryColor);

            if (primaryColor == 0x00000000) {
                u32 tmp = secondaryColor;
                secondaryColor = primaryColor;
                primaryColor = tmp;
            }

            // Create a "1-bit" representation of the graphic indicating the "shape" of the cell
            BitmapImage *maskImage = image_mask_create(cellImage, primaryColor, secondaryColor);

            // Determine the best fit glyph for the cell shape
            asciiChar glyph = image_match_glyph(con, maskImage);

            // We're done with the mask
            free(maskImage);

            // printf("Best glyph: %c\n", glyph);

            // Render that glyph into a cell of the ascii image
            ConsoleCell *cCell = &asciiImg->cells[r * cols + c];
            cCell->glyph = glyph;
            if (glyph == ' ') {
                // Single color cell
                cCell->fgColor = secondaryColor;
                cCell->bgColor = primaryColor;
            } else {
                cCell->fgColor = primaryColor;
                cCell->bgColor = secondaryColor;
            }

        }
    }

    // Free the memory in each cell's pixels
	// TODO: - DEBUG
    // for (i32 c = 0; c < (rows * cols); c++) {
    //     BitmapImage *bm = &cells[c];
    //     free(bm->pixels);
    // }
    free(cells);
    return asciiImg;
}

BitmapImage *
image_slice(BitmapImage *img, i32 rows, i32 cols) {
    // Slice the given image into a 2D array of image cells
    i32 cellWidth = img->width / cols;
    i32 cellHeight = img->height / rows;
    BitmapImage *cells = calloc(rows * cols, sizeof(BitmapImage));
    for (i32 cellY = 0; cellY < rows; cellY++) {
        for (i32 cellX = 0; cellX < cols; cellX++) {
            BitmapImage *cellBM = &cells[(cellY * cols) + cellX];
            cellBM->width = cellWidth;
            cellBM->height = cellHeight;
            cellBM->pixels = calloc(cellWidth * cellHeight, sizeof(u32));

            for (i32 y = 0; y < cellHeight; y++) {
                memcpy(&cellBM->pixels[y * cellWidth], 
                    &img->pixels[(((cellY * cellHeight) + y) * img->width) + (cellX * cellWidth)], 
                    cellWidth * sizeof(u32));
            }
        }
    }
    return cells;
}

u32
rgbdist(u32 color1, u32 color2) {
    i32 dr = RED(color1) - RED(color2);
    i32 dg = GREEN(color1) - GREEN(color2);
    i32 db = BLUE(color1) - BLUE(color2);

    return dr*dr + dg*dg + db*db;    
}

bool
colors_are_distinct(u32 color1, u32 color2) {
    if (rgbdist(color1, color2) > 2000) {
        return true;
    }
    return false;
}

void 
image_analyze_colors(BitmapImage *image, u32 *primaryColor, u32 *secondaryColor) {
    // Determine primary and secondary colors for the given image.
    // The colors should be distinct enough to be distiguishable.

    // Step one - count color occurrences
    u32 *colors = calloc(image->width * image->height, sizeof(u32));
    u32 *counts = calloc(image->width * image->height, sizeof(u32));
    u32 numColors = 0;

    for (u32 y = 0; y < image->height; y++) {
        for (u32 x = 0; x < image->width; x++) {
            // Grab the pixel color
            u32 pixelColor = image->pixels[(y * image->width) + x];

            bool alreadySeen = false;
            for (u32 i = 0; i < numColors; i++) {
                if (colors[i] == pixelColor) {
                    alreadySeen = true;
                    counts[i] += 1;
                    break;
                }
            }

            if (!alreadySeen) {
                colors[numColors] = pixelColor;
                counts[numColors] = 1;
                numColors += 1;
            }
        }
    }

    // Step two - distill this list into distinct primary/secondary colors
    // Primary
    i32 pIdx = -1;
    u32 pCount = 0;
    for (u32 i = 0; i < numColors; i++) {
        if (counts[i] > pCount) {
            pIdx = i;
            pCount = counts[i];
        }
    }

    *primaryColor = colors[pIdx];

    // Secondary
    i32 sIdx = -1;
    u32 sCount = 0;
    for (u32 i = 0; i < numColors; i++) {
        if ((colors[i] != *primaryColor) && 
            (counts[i] > sCount) && 
            (colors_are_distinct(*primaryColor, colors[i]))) {
            sIdx = i;
            sCount = counts[i];
        }
    }

    if (sIdx != -1) {
        *secondaryColor = colors[sIdx];
    } else {
        // We only have one color
        *secondaryColor = 0x00000000;
    }

    free(colors);
    free(counts);
}

BitmapImage*
image_load_from_file(char *filename) {
    // Load the image data
    int imgWidth, imgHeight, numComponents;
    unsigned char *imgData = stbi_load(filename, 
                                    &imgWidth, &imgHeight, 
                                    &numComponents, STBI_rgb_alpha);

    // Copy the image data so we can hold onto it
    u32 imgDataSize = imgWidth * imgHeight * sizeof(u32);
    u32 *imageData = calloc(imgDataSize, sizeof(u32));
    memcpy(imageData, imgData, imgDataSize);

    // Swap endianness of data if we need to
    if (system_is_little_endian()) {
        u32 pixelCount = imgWidth * imgHeight;
        for (u32 i = 0; i < pixelCount; i++) {
            imageData[i] = SWAP_U32(imageData[i]);
        }        
    }

    BitmapImage *bmi = calloc(1, sizeof(BitmapImage));
    bmi->pixels = imageData;
    bmi->width = imgWidth;
    bmi->height = imgHeight;

    stbi_image_free(imgData);

    return bmi;
}

BitmapImage *
image_mask_create(BitmapImage *origImage, u32 primaryColor, u32 secondaryColor) 
{
    // Create a "1-bit" version of the given image
    BitmapImage *maskImage = calloc(1, sizeof(BitmapImage));
    maskImage->width = origImage->width;
    maskImage->height = origImage->height;
    maskImage->pixels = calloc(origImage->width * origImage->height, sizeof(u32));

    for (u32 y = 0; y < origImage->height; y++) {
        for (u32 x = 0; x < origImage->width; x++) {
            u32 pixelColor = origImage->pixels[y * origImage->width + x];
            if (rgbdist(pixelColor, primaryColor) <= rgbdist(pixelColor, secondaryColor)) {
                maskImage->pixels[y * origImage->width + x] = 0xFFFFFFFF;
            } else {
                maskImage->pixels[y * origImage->width + x] = 0x00000000;
            }
        }
    }

    return maskImage;
}

asciiChar
image_match_glyph(Console *console, BitmapImage *maskImage) {
    ConsoleFont *font = console->font;

    i32 matchCount = 0;
    asciiChar bestMatch = 0;

    asciiChar drawingGlyphs[] = {0, 219, 220, 221, 222, 223, 226, 227, 228, 229, 230, 231};
    u32 drawingGlyphCount = 12;

    // Loop through all drawing glyphs, looking for the best match to the mask image
    for (u32 dgi = 0; dgi < drawingGlyphCount; dgi++) {
        u32 cellY = drawingGlyphs[dgi] / 16;
        u32 cellX = drawingGlyphs[dgi] % 16;

        // Compare all the pixels in the glyph to all the pixels in the mask
        i32 matches = 0; 
        for (u32 y = 0; y < font->charHeight; y++) {
            for (u32 x = 0; x < font->charWidth; x++) {
                u32 glyphPixel = font->atlas[((cellY * font->charHeight + y) * font->atlasWidth) + (cellX * font->charWidth) + x];
                u32 maskPixel = maskImage->pixels[y * font->charWidth + x];

                if (glyphPixel == maskPixel) { 
                    matches += 1; 
                } else {
                    matches -= 1;
                }
            }
        }

        if (matches > matchCount) {
            matchCount = matches;
            bestMatch = drawingGlyphs[dgi];
        }
    }

    return bestMatch;
}


/* UI Function Implementation */

UIView * 
view_new(UIRect pixelRect, u32 cellCountX, u32 cellCountY, 
         char *fontFile, asciiChar firstCharInAtlas, u32 bgColor,
         bool colorize, UIRenderFunction renderFn) {

    UIView *view = calloc(1, sizeof(UIView));
    UIRect *rect = calloc(1, sizeof(UIRect));

    memcpy(rect, &pixelRect, sizeof(UIRect));
    Console *console = console_new(rect->w, rect->h, cellCountY, cellCountX, 
        bgColor, colorize);

    i32 cellWidthPixels = pixelRect.w / cellCountX;
    i32 cellHeightPixels = pixelRect.h / cellCountY;
    console_set_bitmap_font(console, fontFile, firstCharInAtlas, cellWidthPixels, cellHeightPixels);

    view->console = console;
    view->pixelRect = rect;
    view->render = renderFn;

    return view;
}

void 
view_destroy(UIView *view) {
    if (view) {
        free(view->pixelRect);
        console_destroy(view->console);
    }
}


/* UI Utility Functions **/

void 
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
                    c = (borderWidth == 1) ? (char)179 : (char)186;
                }

                // Top
                if (y == rect->y) {
                    if (x == rect->x) {
                        // Top left corner
                        c = (borderWidth == 1) ? (char)218 : (char)201;
                    } else if (x == rect->x + rect->w - 1) {
                        // Top right corner
                        c = (borderWidth == 1) ? (char)191 : (char)187;
                    } else {
                        // Top border
                        c = (borderWidth == 1) ? (char)196 : (char)205;
                    }
                }
                
                // Bottom
                if (y == rect->y + rect->h - 1) {
                    if (x == rect->x) {
                        // Bottom left corner
                        c = (borderWidth == 1) ? (char)192 : (char)200;
                    } else if (x == rect->x + rect->w - 1) {
                        // Bottom right corner
                        c = (borderWidth == 1) ? (char)217 : (char)188;
                    } else {
                        // Bottom border
                        c = (borderWidth == 1) ? (char)196 : (char)205;
                    }
                }
            }
            console_put_char_at(console, c, x, y, borderColor, color);
        }
    }
}

void
view_draw_image_at(Console *console, BitmapImage *image, i32 cellX, i32 cellY) {
    // Loop through all the pixels in the bitmap, one row at a time and write them into 
    // the console's pixel buffer at the proper location.
    u32 dstX = cellX * console->cellWidth;
    for (u32 srcY = 0; srcY < image->height; srcY++) {
        // Copy row of pixels from image to console
        u32 dstY = (cellY * console->cellHeight) + srcY;
        memcpy(&console->pixels[(dstY * console->width) + dstX], &image->pixels[srcY * image->width], image->width * sizeof(u32));
    }
}

void
view_draw_ascii_image_at(Console *console, AsciiImage *image, i32 cellX, i32 cellY)
{
    for (u32 y = 0; y < image->rows; y++) {
        for (u32 x = 0; x < image->cols; x++) {
            ConsoleCell cc = image->cells[y * image->cols + x];
            console_put_char_at(console, cc.glyph, cellX + x, cellY + y, cc.fgColor, cc.bgColor);
        }
    }
}

/* Utility Function Implementation */

u32
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

void
ui_copy_blend(u32 *destPixels, UIRect *destRect, u32 destPixelsPerRow,
              u32 *srcPixels, UIRect *srcRect, u32 srcPixelsPerRow,
              bool colorize, u32 *newColor)
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

            // If source pixel is true black (0,0,0,255) then treat it as alpha = 0
            if ((RED(srcColor) == 0) && (GREEN(srcColor) == 0) && 
                (BLUE(srcColor) == 0) && (ALPHA(srcColor) == 255)) {
                    srcColor = COLOR_FROM_RGBA(RED(srcColor), GREEN(srcColor), BLUE(srcColor), 0);
                }

            // Colorize our source pixel before we blend it
            if (colorize) {
                srcColor = ui_colorize_pixel(srcColor, *newColor);
            } else {
                // Just apply the alpha value from the newColor, unless the src is transparent
                if (ALPHA(srcColor) > 0) {
                    srcColor = COLOR_FROM_RGBA(RED(srcColor), GREEN(srcColor), BLUE(srcColor), ALPHA(*newColor));
                }
            }

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

void
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

void
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

UIRect 
rect_get_for_glyph(asciiChar c, ConsoleFont *font) {
    i32 idx = c - font->firstCharInAtlas;
    i32 charsPerRow = (font->atlasWidth / font->charWidth);
    i32 xOffset = (idx % charsPerRow) * font->charWidth;
    i32 yOffset = (idx / charsPerRow) * font->charHeight;

    UIRect glyphRect = {xOffset, yOffset, font->charWidth, font->charHeight};
    return glyphRect;
}













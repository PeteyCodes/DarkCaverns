/*
    UI Library

    by Peter de Tagyos
    Started: 12/28/2015

    Implementation of an immediate-mode GUI.
    ref: mollyrocket.com/861, sol.gfxile.net/imgui/ch02.html
*/

#define UNAVAILABLE 999999999 

typedef struct {
    i32 mouseX;
    i32 mouseY;
    bool mouse1Down;
    bool mouse2Down;
} InputState;

typedef struct {
    PT_Console *console;
    InputState *input;
    u32 hotItem;
    u32 activeItem;    
} UIState;

typedef struct {
    List *views;
} UIScreen;

typedef void (*RenderFunction)(PT_Console *);
typedef struct {
    PT_Console *console;
    PT_Rect *pixelRect;
    RenderFunction render;
} UIView;



/* Data Structure Helper Functions */

internal UIView * view_new(PT_Rect pixelRect, u32 cellCountX, u32 cellCountY, 
                           char *fontFile, asciiChar firstCharInAtlas,
                           RenderFunction renderFn) {

    UIView *view = malloc(sizeof(UIView));
    PT_Rect *rect = malloc(sizeof(PT_Rect));

    memcpy(rect, &pixelRect, sizeof(PT_Rect));
    PT_Console *console = PT_ConsoleInit(rect->w, rect->h, cellCountY, cellCountX);

    i32 cellWidthPixels = pixelRect.w / cellCountX;
    i32 cellHeightPixels = pixelRect.h / cellCountY;
    PT_ConsoleSetBitmapFont(console, fontFile, firstCharInAtlas, cellWidthPixels, cellHeightPixels);

    view->console = console;
    view->pixelRect = rect;
    view->render = renderFn;

    return view;
}

internal void view_destroy(UIView *view) {
    // TODO
}

/* UI Utility Functions **/

internal void 
UI_DrawRect(PT_Console *console, PT_Rect *rect, u32 color, 
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
            PT_ConsolePutCharAt(console, c, x, y, borderColor, color);
        }
    }
}

internal bool
UI_PointInRect(i32 x, i32 y, PT_Rect *rect) 
{
    if (x < rect->x || 
        y < rect->y || 
        x >= rect->x + rect->w || 
        y >= rect->y + rect->h) {
        return false;
    }

    return true;
}


/* UI Lifecycle Functions **/

internal void 
UI_StartFrame(UIState *ui, PT_Console *con, InputState *input)
{
    ui->console = con;
    ui->input = input;
    ui->hotItem = 0;
}

internal void
UI_FinishFrame(UIState *ui)
{
    if (ui->input->mouse1Down == 0) {
        ui->activeItem = 0;
    } else {
        if (ui->activeItem == 0) {
            ui->activeItem = UNAVAILABLE;
        }
    }
}

/* Button **/

internal bool
UI_Button(UIState *ui, u32 id, PT_Rect *rect)
{
    // Check if the button should be hot
    PT_Rect pixelRect = { 
        rect->x * ui->console->cellWidth,
        rect->y * ui->console->cellHeight,
        rect->w * ui->console->cellWidth,
        rect->h * ui->console->cellHeight
    }; 
    if (UI_PointInRect(ui->input->mouseX, ui->input->mouseY, &pixelRect)) {
        ui->hotItem = id;
        if (ui->activeItem == 0 && ui->input->mouse1Down) {
            ui->activeItem = id;
        }
    }

    // Now render the button
    if (ui->hotItem == id && ui->activeItem == id) {
        UI_DrawRect(ui->console, rect, 0x00ff00ff, 1, 0xffffffff);
    } else if (ui->hotItem == id) {
        UI_DrawRect(ui->console, rect, 0x00ff0099, 1, 0xffffffff);
    } else {
        UI_DrawRect(ui->console, rect, 0x00ff0055, 1, 0xffffffff);
    }


    // Check if the button has been triggered (if hot and active, but
    // button is not down, then user must have finished click)
    if (ui->input->mouse1Down == 0 && ui->hotItem == id && ui->activeItem == id) {
        return true;
    }

    return false;
}




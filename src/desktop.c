#include "desktop.h"
#include "canvas.h"
#include "serial.h"
#include "vmm.h"
#include "types.h"

#define MAX_ICONS 32
#define ICON_IMG_W 32
#define ICON_IMG_H 32
#define ICON_LABEL_H 14
#define DBLCLICK_TIMEOUT 30

typedef struct {
    int x, y, w, h;
    char name[32];
    desktop_icon_dclick_fn on_dclick;
} desktop_icon_t;

static desktop_icon_t icons[MAX_ICONS];
static int icon_count = 0;

static int  dblclick_timer = 0;
static int  last_clicked_icon = -1;
static bool desktop_active = false;

#define C_WHITE    COLOR_WHITE
#define C_BLACK    COLOR_BLACK
#define C_GREY     COLOR_SILVER
#define C_DGRAY    COLOR_DARK_GREY
#define C_LGRAY    COLOR_LIGHT_GREY
#define C_BLUE     COLOR_RGB(34, 102, 204)
#define C_DBLUE    COLOR_RGB(16, 52, 120)
#define C_GREEN    COLOR_GREEN
#define C_YELLOW   COLOR_YELLOW
#define C_CYAN     COLOR_CYAN
#define C_ORANGE   COLOR_ORANGE
#define C_LABEL_BG COLOR_RGB(58, 110, 165)

static void draw_pc_icon(canvas_t *c, int bx, int by)
{
    canvas_fill_rect(c, bx + 5,  by + 2,  22, 16, C_DGRAY);
    canvas_fill_rect(c, bx + 7,  by + 4,  18, 12, C_BLUE);
    canvas_fill_rect(c, bx + 9,  by + 6,  14, 1,  C_CYAN);
    canvas_fill_rect(c, bx + 9,  by + 8,  14, 1,  C_CYAN);
    canvas_fill_rect(c, bx + 9,  by + 10, 14, 1,  C_CYAN);
    canvas_fill_rect(c, bx + 14, by + 18, 4,  4,  C_DGRAY);
    canvas_fill_rect(c, bx + 10, by + 22, 12, 3,  C_DGRAY);
    canvas_fill_rect(c, bx + 2,  by + 1,  1,  1,  C_LGRAY);
    canvas_fill_rect(c, bx + 27, by + 1,  1,  1,  C_LGRAY);
}

static void draw_recycle_icon(canvas_t *c, int bx, int by)
{
    canvas_fill_rect(c, bx + 6,  by + 4,  20, 5,  C_ORANGE);
    canvas_fill_rect(c, bx + 12, by + 0,  8,  4,  C_ORANGE);
    canvas_fill_rect(c, bx + 8,  by + 9,  16, 16, C_GREY);
    canvas_fill_rect(c, bx + 8,  by + 9,  16, 1,  C_DGRAY);
    canvas_fill_rect(c, bx + 8,  by + 14, 16, 1,  C_DGRAY);
    canvas_fill_rect(c, bx + 8,  by + 19, 16, 1,  C_DGRAY);
    canvas_fill_rect(c, bx + 6,  by + 7,  4,  4,  C_GREEN);
    canvas_fill_rect(c, bx + 22, by + 7,  4,  4,  C_GREEN);
    canvas_fill_rect(c, bx + 10, by + 25, 12, 4,  C_GREY);
}

static void draw_settings_icon(canvas_t *c, int bx, int by)
{
    int cx = bx + 16;
    int cy = by + 16;
    canvas_fill_rect(c, cx - 4,  cy - 4,  8, 8, C_DGRAY);
    canvas_fill_rect(c, cx - 2,  cy - 2,  4, 4, C_LGRAY);
    canvas_fill_rect(c, cx - 2,  by + 0,  4, 8,  C_DGRAY);
    canvas_fill_rect(c, cx - 2,  by + 24, 4, 8,  C_DGRAY);
    canvas_fill_rect(c, bx + 0,  cy - 2,  8, 4,  C_DGRAY);
    canvas_fill_rect(c, bx + 24, cy - 2,  8, 4,  C_DGRAY);
    canvas_fill_rect(c, cx + 5,  by + 5,  6, 6,  C_DGRAY);
    canvas_fill_rect(c, cx - 11, by + 5,  6, 6,  C_DGRAY);
    canvas_fill_rect(c, cx + 5,  by + 21, 6, 6,  C_DGRAY);
    canvas_fill_rect(c, cx - 11, by + 21, 6, 6,  C_DGRAY);
}

static void draw_terminal_icon(canvas_t *c, int bx, int by)
{
    canvas_fill_rect(c, bx + 4,  by + 3,  24, 24, C_BLACK);
    canvas_draw_rect(c, bx + 3,  by + 2,  26, 26, C_DGRAY);
    canvas_fill_rect(c, bx + 3,  by + 2,  26, 1,  C_LGRAY);
    canvas_fill_rect(c, bx + 3,  by + 2,  1,  26, C_LGRAY);
    canvas_fill_rect(c, bx + 7,  by + 7,  4,  2,  C_GREEN);
    canvas_fill_rect(c, bx + 12, by + 7,  2,  2,  C_GREEN);
    canvas_fill_rect(c, bx + 7,  by + 11, 2,  2,  C_CYAN);
    canvas_fill_rect(c, bx + 10, by + 11, 2,  2,  C_CYAN);
    canvas_fill_rect(c, bx + 7,  by + 16, 2,  2,  C_WHITE);
    canvas_fill_rect(c, bx + 10, by + 16, 2,  2,  C_WHITE);
    canvas_fill_rect(c, bx + 13, by + 16, 2,  2,  C_WHITE);
    canvas_fill_rect(c, bx + 7,  by + 20, 8,  1,  C_WHITE);
}

static void draw_icon_image(canvas_t *c, int idx, int bx, int by)
{
    switch (idx) {
    case 0: draw_pc_icon(c, bx, by);       break;
    case 1: draw_recycle_icon(c, bx, by);  break;
    case 2: draw_settings_icon(c, bx, by); break;
    case 3: draw_terminal_icon(c, bx, by); break;
    default: break;
    }
}

void desktop_init(void)
{
    icon_count = 0;
    dblclick_timer = 0;
    last_clicked_icon = -1;
    desktop_active = true;
    serial_write("[DESKTOP] 桌面图标管理器初始化\n");
}

int desktop_add_icon(int x, int y, int w, int h, const char *name, desktop_icon_dclick_fn fn)
{
    if (icon_count >= MAX_ICONS) return -1;

    desktop_icon_t *ic = &icons[icon_count];
    ic->x = x;
    ic->y = y;
    ic->w = w;
    ic->h = h;
    ic->on_dclick = fn;

    int i = 0;
    while (name[i] && i < 31) { ic->name[i] = name[i]; i++; }
    ic->name[i] = '\0';

    serial_write("[DESKTOP] 添加图标: ");
    serial_write(ic->name);
    serial_write("\n");
    return icon_count++;
}

void desktop_draw(canvas_t *c)
{
    if (!c || !desktop_active) return;

    for (int i = 0; i < icon_count; i++) {
        desktop_icon_t *ic = &icons[i];
        int ix = ic->x + (ic->w - ICON_IMG_W) / 2;
        int iy = ic->y + 2;

        draw_icon_image(c, i, ix, iy);

        int strw = 0;
        while (ic->name[strw]) strw++;
        strw *= CANVAS_CHAR_W;

        int lx = ic->x + (ic->w - strw) / 2;
        int ly = ic->y + ICON_IMG_H + 4;

        canvas_fill_rect(c, lx - 1, ly, strw + 2, CANVAS_CHAR_H, C_LABEL_BG);
        canvas_draw_string(c, lx, ly, ic->name, C_WHITE, C_LABEL_BG);
    }
}

bool desktop_handle_click(int x, int y)
{
    if (!desktop_active) return false;

    for (int i = icon_count - 1; i >= 0; i--) {
        desktop_icon_t *ic = &icons[i];
        if (x >= ic->x && x < ic->x + ic->w &&
            y >= ic->y && y < ic->y + ic->h) {

            if (dblclick_timer > 0 && last_clicked_icon == i) {
                serial_write("[DESKTOP] 双击图标: ");
                serial_write(ic->name);
                serial_write("\n");
                if (ic->on_dclick) {
                    ic->on_dclick();
                }
                dblclick_timer = 0;
                last_clicked_icon = -1;
            } else {
                dblclick_timer = DBLCLICK_TIMEOUT;
                last_clicked_icon = i;
                serial_write("[DESKTOP] 单击图标: ");
                serial_write(ic->name);
                serial_write("\n");
            }
            return true;
        }
    }

    dblclick_timer = 0;
    last_clicked_icon = -1;
    return false;
}

void desktop_destroy(void)
{
    icon_count = 0;
    desktop_active = false;
    serial_write("[DESKTOP] 桌面图标管理器销毁\n");
}
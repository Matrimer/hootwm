#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <signal.h>
#include <xcb/xcb.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#define XCB_MOVE XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y
#define XCB_RESIZE XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT
#define XCB_MOVE_RESIZE XCB_MOVE | XCB_RESIZE
#define move_resize(win, x, y, w, h) \
    uint32_t tmpvals[4] = { x, y, w, h }; \
    xcb_configure_window(conn, win, XCB_MOVE_RESIZE, tmpvals);

#define p(s) printf("hootwm: %s\n", s)

xcb_connection_t *conn;
xcb_screen_t     *screen;
int16_t           pipe_fd;
char             *pipe_f;

uint16_t screen_w, screen_h,
         master_size;

uint8_t gap, bord;

uint32_t win_focus, win_unfocus;

bool run;

#include "nodes.c"

#include "windows.c"

// Move windows
void tile(void) {
    if (!head) {
        return;
    } else if (!head->next) {
        move_resize(head->win, gap, gap,
            screen_w - gap*2 - bord*2,
            screen_h - gap*2 - bord*2);
    } else {
        move_resize(head->win, gap, gap,
            master_size - gap*2 - bord*2,
            screen_h - gap*2 - bord*2);

        node *w = head;
        uint16_t y = 0; uint8_t n = 0;
        while (w->next) { ++n; w = w->next; };
        uint16_t height = screen_h / n;
        for (w = head->next ; n > 0 ;
                --n, w = w->next, y += height) {
            move_resize(w->win,
                master_size + gap, y + gap,
                screen_w - master_size - gap*2 - bord*2,
                ((w->next) ? height : (screen_h - y)) -
                gap*2 - bord*2);
        }
    }
}

void update_current(void) {
    node *w = head;

    while (w) {
        uint32_t b[1] = { bord };
        xcb_configure_window(conn, w->win, XCB_CONFIG_WINDOW_BORDER_WIDTH, b);

        if (w == current) {
            xcb_change_window_attributes(conn, w->win, XCB_CW_BORDER_PIXEL,
                &win_focus);
            xcb_set_input_focus(conn, XCB_INPUT_FOCUS_POINTER_ROOT,
                w->win, XCB_CURRENT_TIME);
        } else {
            xcb_change_window_attributes(conn, w->win, XCB_CW_BORDER_PIXEL,
                &win_unfocus);
        }

        w = w->next;
    }
}

// Handle events
void map_request(xcb_map_request_event_t *ev) {
    win_create(ev->window);
    xcb_map_window(conn, ev->window);

    tile();
    update_current();
}

void destroy_notify(xcb_destroy_notify_event_t *ev) {
    win_destroy(ev->window);

    tile();
    update_current();
}

// Manage commands
#include "commands.c"

// Run
void setup(void) {
    screen = xcb_setup_roots_iterator(xcb_get_setup(conn)).data;

    pipe_f = "/tmp/hoot";
    mkfifo(pipe_f, 0666);
    pipe_fd = open(pipe_f, O_RDONLY | O_NONBLOCK);

    screen_w = screen->width_in_pixels;
    screen_h = screen->height_in_pixels;

    head = NULL;
    current = NULL;

    master_size = screen_w * 0.6;
    gap = 1;
    bord = 3;

    win_focus = 52260;
    win_unfocus = 34184;

    run = true;

    uint32_t mask[1] = {
        XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY | // destroy notify
        XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT }; // map request
    xcb_change_window_attributes(conn, screen->root, XCB_CW_EVENT_MASK, mask);

    xcb_flush(conn);
}

void quit() {
    run = false;
    p("Thanks for using!");

    close(pipe_fd); unlink(pipe_f);
    xcb_disconnect(conn);

    exit(0);
}

void event_loop(void) {
    uint32_t length;
    xcb_generic_event_t *ev;

    do {
        if ((ev = xcb_poll_for_event(conn))) {
            switch(ev->response_type & ~0x80) {
            case XCB_MAP_REQUEST: {
                map_request((xcb_map_request_event_t*)ev);
                xcb_flush(conn);
                } break; 
            case XCB_DESTROY_NOTIFY: {
                destroy_notify((xcb_destroy_notify_event_t*)ev);
                xcb_flush(conn);
                } break; 
            }
        }

        char buffer[255];

        if ((length = read(pipe_fd, buffer, sizeof(buffer)))) {
            buffer[length-1] = '\0';
            dispatch_command(buffer);
            xcb_flush(conn);
        }

        //length = 0;
        free(ev);

    } while (run);
}

int main(void) {
    conn = xcb_connect(NULL, NULL);
    if (xcb_connection_has_error(conn)) return 1;

    setup();
    signal(SIGINT, quit);
    p("Welcome to hootwm.");
    event_loop();

    return 0;
}

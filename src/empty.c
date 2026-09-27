#include "window.h"

int main()
{
    window_t *window = window_create(PIXEL_FORMAT_RGB888);
    if (!window) return 1;

    window_set_size(window, 800, 600);
    window_set_title(window, "empty");
    window_set_visible(window, true);
    window_set_flags(window, WINDOW_FLAG_UNRESIZABLE);

    while (1) {
        window_event_t ev;
        while ((ev = window_poll_event(window)) != WINDOW_EVENT_NONE) {
            if (ev == WINDOW_EVENT_CLOSE) goto done;
        }
    }
done:
    window_destroy(window);
    return 0;
}
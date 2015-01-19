#include <pebble.h>
#include "xprintf.h"
  
static Window *window;
static TextLayer *text_layer;


static bool is_running;
static char text_output[20];
static uint32_t seconds;
static uint32_t minutes;
static int select_count;

/* For vibration when time is up */
static const uint32_t const timer_segments[] = { 1000, 500, 1000 };
VibePattern timer_pattern = {
    .durations = timer_segments,
    .num_segments = ARRAY_LENGTH(timer_segments),
};

/*
 * update_text
 *
 * Arguments:   void
 * Returns:     void
 * Description:
 *  Updates the text of the app, called every tick
 */
static void update_text() {
    /* Print the time left */
    snprintf(text_output, 20, "%02lu:%02lu", minutes, seconds);
    text_layer_set_text(text_layer, text_output);
}

/*
 * tick_handler
 *
 * Arguments:
 * Returns:     void
 * Description:
 *  Callback previously registered to tick every second
 */

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
    /* Only update if running */
    if (is_running) {
        if (seconds > 0) {
            seconds--;
        } else if (minutes <= 0) {
            is_running = false;
            vibes_enqueue_custom_pattern(timer_pattern);
        } else if (seconds <= 0) {
            minutes--;
            seconds = 59;
        }
        update_text();
    }
}

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
    // Start/stop
    if (select_count == 0) {
        select_count++;
    } else {
        if (is_running) {
            is_running = false;
        } else {
            is_running = true;
        }
    }
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
    if (is_running) {
        return;
    }
    if (select_count == 0) {
        if (minutes < 60) {
            minutes++;
        }
    } else {
        if (seconds < 59) {
            seconds++;
        }
        else {
            seconds = 0;
            minutes++;
        }
    }
    snprintf(text_output, 20, "%02lu:%02lu", minutes, seconds);
    text_layer_set_text(text_layer, text_output);
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
    if (is_running) {
        return;
    }
    if (select_count == 0) {
        if (minutes > 0) {
            minutes--;
        }
    } else {
        if (seconds > 0) {
            seconds--;
        }
        else {
            if (minutes > 0) {
                seconds = 59;
                minutes--;
            } else {
                seconds = 0;
            }
        }
    }
    snprintf(text_output, 20, "%02lu:%02lu", minutes, seconds);
    text_layer_set_text(text_layer, text_output);
}

static void click_config_provider(void *context) {
    window_single_repeating_click_subscribe(BUTTON_ID_UP, 200, up_click_handler);
    window_single_repeating_click_subscribe(BUTTON_ID_DOWN, 200, down_click_handler);
    window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
}

static void window_load(Window *window) {
    Layer *window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(window_layer);

    text_layer = text_layer_create((GRect) { .origin = { 0, bounds.size.h/2-30 }, .size = { bounds.size.w, bounds.size.h } });
    text_layer_set_font(text_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_MEDIUM_NUMBERS));
    text_layer_set_text(text_layer, "00:00");
    text_layer_set_text_alignment(text_layer, GTextAlignmentCenter);
    layer_add_child(window_layer, text_layer_get_layer(text_layer));
}

static void window_unload(Window *window) {
    text_layer_destroy(text_layer);
}

static void init(void) {
    is_running = false;
    minutes = 0;
    seconds = 0;
    window = window_create();
    window_set_click_config_provider(window, click_config_provider);
    window_set_window_handlers(window, (WindowHandlers) {
        .load = window_load,
        .unload = window_unload,
    });
    const bool animated = true;
    window_stack_push(window, animated);
    tick_timer_service_subscribe(SECOND_UNIT, tick_handler);
}

static void deinit(void) {
    window_destroy(window);
}

int main(void) {
    init();

    APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", window);

    app_event_loop();
    deinit();
}

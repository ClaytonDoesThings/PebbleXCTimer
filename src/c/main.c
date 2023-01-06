#include <pebble.h>

static bool debug = true;

static Window *s_main_window;

static TextLayer *s_time_layer;
static TextLayer *s_timer_layer;
static time_t last_time_minutes;

static int interval = 20;
static int elapsed = 0;
static int lap = 0;
static bool playing = false;

static void update_view() {
  static char buf[6];
  if (interval == 0) {
    int mins = elapsed/60;
    snprintf(buf, sizeof(buf), "%02d:%02d", mins, elapsed-mins*60);
  } else {
    snprintf(buf, sizeof(buf), "%02d:%02d", interval, elapsed);
  }
  if (debug) {
    app_log(APP_LOG_LEVEL_DEBUG, "main", 15, buf);
  }

  // Display this time on the TextLayer
  text_layer_set_text(s_timer_layer, buf);
}

static void update_time() {
  // Get a tm structure
  time_t global_time = time(NULL);
  time_t global_time_minutes = global_time/60;
  if (last_time_minutes == global_time_minutes) {
    return;
  } else {
    last_time_minutes = global_time_minutes;
  }
  struct tm *local_time = localtime(&global_time);

  // Write the current hours and minutes into a buffer
  static char s_buffer[8];
  strftime(s_buffer, sizeof(s_buffer), clock_is_24h_style() ?
                                          "%H:%M" : "%I:%M", local_time);

  // Display this time on the TextLayer
  text_layer_set_text(s_time_layer, s_buffer);
}

static void update_timer() {
  if (playing) {
    if (interval == 0) {
      elapsed++;
    } else {
      if (interval-1 == elapsed) {
        elapsed = 0;
        static const uint32_t segments[] = {200, 50, 300, 50, 400};
        VibePattern pat = {
          .durations = segments,
          .num_segments = ARRAY_LENGTH(segments),
        };
        vibes_enqueue_custom_pattern(pat);
      } else {
        elapsed++;
        lap++;
      }
    }
  }
  update_view();
}

static void second_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_timer();
  update_time();
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (interval < 60) {
    interval++;
    playing = false;
    elapsed = 0;
    update_view();
  }
}

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (playing) {
    playing = false;
  } else {
    playing = true;
  }
  update_view();
}

static void select_long_click_handler(ClickRecognizerRef recognizer, void *context) {
  playing = false;
  elapsed = 0;
  lap = 0;
  update_view();
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (interval > 0) {
    interval--;
    playing = false;
    elapsed = 0;
    update_view();
  }
}

static void click_config_provider(void *context) {
  // Subcribe to button click events here
  uint16_t delay_ms = 500;

  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);

  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_long_click_subscribe(BUTTON_ID_SELECT, delay_ms, select_long_click_handler, NULL);

  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
}

static void main_window_load(Window *window) {
  // Get information about the Window
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  // Create the TextLayer with specific bounds
  s_time_layer = text_layer_create(
      GRect(0, 0, bounds.size.w, bounds.size.h)
  );
  s_timer_layer = text_layer_create(
      GRect(0, PBL_IF_ROUND_ELSE(58, 52), bounds.size.w, bounds.size.h)
  );

  // Improve the layout to be more like a watchface
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, GColorDarkGray);
  text_layer_set_text(s_time_layer, "00:00");
  text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_LECO_20_BOLD_NUMBERS));
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);

  text_layer_set_background_color(s_timer_layer, GColorClear);
  text_layer_set_text_color(s_timer_layer, GColorBlack);
  text_layer_set_text(s_timer_layer, "00:00");
  text_layer_set_font(s_timer_layer, fonts_get_system_font(FONT_KEY_LECO_36_BOLD_NUMBERS));
  text_layer_set_text_alignment(s_timer_layer, GTextAlignmentCenter);

  // Add it as a child layer to the Window's root layer
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));
  layer_add_child(window_layer, text_layer_get_layer(s_timer_layer));


  // Make sure the time & timer is displayed from the start
  update_time();
  update_timer();

  // Register with TickTimerService
  tick_timer_service_subscribe(SECOND_UNIT, second_handler);

  // Use this provider to add button click subscriptions
  window_set_click_config_provider(window, click_config_provider);
}

static void main_window_unload(Window *window) {
  // Destroy TextLayer
  text_layer_destroy(s_timer_layer);
}

static void init() {
  // Create main Window element and assign to pointer
  s_main_window = window_create();

  // Set handlers to manage the elements inside the Window
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });

  // Show the Window on the watch, with animated=true
  window_stack_push(s_main_window, true);
}

static void deinit() {
  // Destroy Window
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}

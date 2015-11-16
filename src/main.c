#include <pebble.h>
#include <numbers3x5.h>
#include <keys.h>

#define DEBUG

#define BLOCK_SPACE 1

typedef struct {
	int (*numberMask)[NUM_LENGTH];
	GColor (*color)(void);
} Config;

typedef void (*DrawRoutine)(GContext *, Config, int, int, int, int);

static Window *my_window = NULL; 
static Layer  *my_layer = NULL;

static GPathInfo s_path_rect;
static GPath *my_path_rect = NULL;

static GColor s_fg_color;
static int s_block_size;
static DrawRoutine s_draw_routine;

static GColor foreground_color() {
		return s_fg_color;
}

static GColor random_color() {
		#if defined(PBL_COLOR)
	  // TODO implement
		return GColorFromRGB(0, 180, 0);
		#elif defined(PBL_BW)
	  return GColorWhite;
		#endif	
}

static void draw_circle(GContext *ctx, const Config config, int digit, int index, int x, int y) {
	int pixel = config.numberMask[digit][index];
	if(pixel > 0) {
		graphics_context_set_fill_color(ctx, config.color());
		int r = s_block_size / 2;
		GPoint point = GPoint(x + r, y + r);
		graphics_fill_circle(ctx, point, r - 1);
	}
}

static void draw_block(GContext *ctx, const Config config, int digit, int index, int x, int y) {
	int pixel = config.numberMask[digit][index];
	if(pixel > 0) {
		graphics_context_set_fill_color(ctx, config.color());
		GRect rect = GRect(x, y, s_block_size, s_block_size);
		graphics_fill_rect(ctx, rect, 0, GCornerNone);
	}
}

static void draw_niceblock(GContext *ctx, const Config config, int digit, int index, int x, int y) {
	graphics_context_set_fill_color(ctx, config.color());
	int pixel = config.numberMask[digit][index];
	if(pixel == 5) {
		GRect rect = GRect(x, y, s_block_size, s_block_size);
		graphics_fill_rect(ctx, rect, 0, GCornerNone);
	} else if(pixel == 7) {
		
		// TODO stopped here...
		my_path_rect = gpath_create(&s_path_rect);
		gpath_move_to(my_path_rect, GPoint(x, y-1)); // CURRENTLY NOT SURE WHY THERE IS THIS SHIFT OF ONE PIXEL
		gpath_draw_filled(ctx, my_path_rect);
		// gpath_destroy(my_path_rect);
	}
}

static void draw_number(Layer *layer, GContext *ctx, const Config config, int digit, int posX, int posY, DrawRoutine draw) {
	for(int i = 0; i < NUM_LENGTH; i++) {
		int row = i / NUM_COLS;
		int col = i % NUM_COLS;
		int y = posY + (row * s_block_size) + (row * BLOCK_SPACE);
		int x = posX + (col * s_block_size) + (col * BLOCK_SPACE);
		draw(ctx, config, digit, i, x, y);
	}
}

static void my_layer_draw(Layer *layer, GContext *ctx) {
	time_t temp = time(NULL); 
  struct tm *tick_time = localtime(&temp);
	
	int d1 = tick_time->tm_hour / 10;
	int d2 = tick_time->tm_hour % 10;
	int d3 = tick_time->tm_min / 10;
	int d4 = tick_time->tm_min % 10;

	Config config = { NUMBERS, foreground_color };
		
	draw_number(layer, ctx, config, d1, 40, 30, s_draw_routine);
	draw_number(layer, ctx, config, d2, 80, 30, s_draw_routine);
	draw_number(layer, ctx, config, d3, 40, 90, s_draw_routine);
	draw_number(layer, ctx, config, d4, 80, 90, s_draw_routine);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
	layer_mark_dirty(my_layer);
}

static void read_configuration() {

#ifdef PBL_SDK_2
	window_set_background_color(my_window, GColorBlack);
	s_fg_color = GColorWhite;
#elif PBL_SDK_3
  int bg_red = persist_read_int(KEY_BG_RED);
  int bg_green = persist_read_int(KEY_BG_GREEN);
  int bg_blue = persist_read_int(KEY_BG_BLUE);
  GColor bg_color = GColorFromRGB(bg_red, bg_green, bg_blue);
	window_set_background_color(my_window, bg_color);
	APP_LOG(APP_LOG_LEVEL_DEBUG, "Read Background Color: %d %d %d", bg_red, bg_green, bg_blue);

	int fg_red = persist_read_int(KEY_FG_RED);
  int fg_green = persist_read_int(KEY_FG_GREEN);
  int fg_blue = persist_read_int(KEY_FG_BLUE);
  s_fg_color = GColorFromRGB(fg_red, fg_green, fg_blue);
	APP_LOG(APP_LOG_LEVEL_DEBUG, "Read Foreground Color: %d %d %d", fg_red, fg_green, fg_blue);
	
	if(gcolor_equal(bg_color, GColorBlack) && gcolor_equal(s_fg_color, GColorBlack)) {
		s_fg_color = GColorFromRGB(0, 180, 0);
	  APP_LOG(APP_LOG_LEVEL_DEBUG, "Applying default Foreground Color");
	}
#endif

	int block_style = -1;
	if(persist_exists(KEY_BLOCK_STYLE)) {
    block_style = persist_read_int(KEY_BLOCK_STYLE);
  	APP_LOG(APP_LOG_LEVEL_DEBUG, "Read Block Style: %d", block_style);
	}
	if(block_style == STYLE_BLOCK) {
		s_draw_routine = draw_block;
	} else if(block_style == STYLE_CIRCLE) {
		s_draw_routine = draw_circle;			
	} else if(block_style == STYLE_NICE_BLOCK) {
		s_draw_routine = draw_niceblock;		
	} else {
	  s_draw_routine = draw_block;
	}
	
	if(persist_exists(KEY_BLOCK_STYLE)) {
	  s_block_size = persist_read_int(KEY_BLOCK_SIZE);
  	APP_LOG(APP_LOG_LEVEL_DEBUG, "Read Block Size: %d", s_block_size);
		if(s_block_size < 1) {
		  s_block_size = DEFAULT_SIZE;
	  }
	} else {
		  s_block_size = DEFAULT_SIZE;
	}
	s_path_rect = (GPathInfo){ 3, (GPoint []){ {s_block_size,0}, {s_block_size,s_block_size}, {0,s_block_size} } };

}

static void inbox_received_handler(DictionaryIterator *iter, void *context) {

#ifdef PBL_SDK_3
	Tuple *bg_red_t = dict_find(iter, KEY_BG_RED);
  Tuple *bg_green_t = dict_find(iter, KEY_BG_GREEN);
  Tuple *bg_blue_t = dict_find(iter, KEY_BG_BLUE);

	if(bg_red_t && bg_green_t && bg_blue_t) {
    int red = bg_red_t->value->int32;
    int green = bg_green_t->value->int32;
    int blue = bg_blue_t->value->int32;

    persist_write_int(KEY_BG_RED, red);
    persist_write_int(KEY_BG_GREEN, green);
    persist_write_int(KEY_BG_BLUE, blue);

#ifdef DEBUG
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Configured Background Color: %d %d %d", red, green, blue);
# endif
	}
	
	Tuple *fg_red_t = dict_find(iter, KEY_FG_RED);
  Tuple *fg_green_t = dict_find(iter, KEY_FG_GREEN);
  Tuple *fg_blue_t = dict_find(iter, KEY_FG_BLUE);

	if(fg_red_t && fg_green_t && fg_blue_t) {
    int red = fg_red_t->value->int32;
    int green = fg_green_t->value->int32;
    int blue = fg_blue_t->value->int32;

    persist_write_int(KEY_FG_RED, red);
    persist_write_int(KEY_FG_GREEN, green);
    persist_write_int(KEY_FG_BLUE, blue);

		APP_LOG(APP_LOG_LEVEL_DEBUG, "Configured Foreground Color: %d %d %d", red, green, blue);
	}
#endif

	Tuple *block_size_t = dict_find(iter, KEY_BLOCK_SIZE);
  if(block_size_t && block_size_t->value->int32 > 0) {
    int block_size =block_size_t->value->int32;
		persist_write_int(KEY_BLOCK_SIZE, block_size);
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Configured Block Size: %d", s_block_size);
	}

	Tuple *block_style_t = dict_find(iter, KEY_BLOCK_STYLE);
  if(block_style_t && block_style_t->value->int32 >= 0 && block_style_t->value->int32 <=1) {
		int block_style = block_style_t->value->int32;
		persist_write_int(KEY_BLOCK_STYLE, block_style);
  	APP_LOG(APP_LOG_LEVEL_DEBUG, "Configured Block Style: %d", block_style);
	}

	read_configuration();
	layer_mark_dirty(my_layer);
}

static void my_window_load(Window *window) {
	Layer *window_layer = window_get_root_layer(window);
  GRect window_bounds = layer_get_bounds(window_layer);
	
	read_configuration();
	
	my_layer = layer_create(window_bounds);
  layer_add_child(window_get_root_layer(my_window), my_layer);
	layer_set_update_proc(my_layer, my_layer_draw);
}

static void my_window_unload(Window *window) {
	layer_destroy(my_layer);
}

void handle_init(void) {
	my_window = window_create();
  window_set_window_handlers(my_window, (WindowHandlers) {
    .load = my_window_load,
    .unload = my_window_unload
  });
	window_stack_push(my_window, true);

	app_message_register_inbox_received(inbox_received_handler);
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
	
	tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);

}

void handle_deinit(void) {
	window_destroy(my_window);
}

int main(void) {
	handle_init();
	app_event_loop();
	handle_deinit();
}

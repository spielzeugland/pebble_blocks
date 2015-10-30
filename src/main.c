#include <pebble.h>
#include <numbers3x5.h>

#define BLOCK_SIZE 10
#define BLOCK_SPACE 1

typedef struct {
	int (*numberMask)[NUM_LENGTH];
	GColor (*color)(GContext *);
} Config;

typedef void (*DrawRoutine)(GContext *, Config, int, int, int, int);

static const GPathInfo PATH_RECT = { 3, (GPoint []){ {BLOCK_SIZE,0}, {BLOCK_SIZE,BLOCK_SIZE}, {0,BLOCK_SIZE} } };

static Window *my_window; 
static Layer  *my_layer;

static GPath *my_path_rect;

static GColor random_color(GContext *ctx) {
		#if defined(PBL_COLOR)
		return GColorFromRGB(0, 180, 0);
		#elif defined(PBL_BW)
	  return GColorWhite;
		#endif	
}

static void draw_plainblock(GContext *ctx, const Config config, int digit, int index, int x, int y) {
	int pixel = config.numberMask[digit][index];
	if(pixel > 0) {
		GRect rect = GRect(x, y, BLOCK_SIZE, BLOCK_SIZE);
		graphics_context_set_fill_color(ctx, config.color(ctx));
		graphics_fill_rect(ctx, rect, 0, GCornerNone);
	}
}

static void draw_niceblock(GContext *ctx, const Config config, int digit, int index, int x, int y) {
	graphics_context_set_fill_color(ctx, config.color(ctx));
	int pixel = config.numberMask[digit][index];
	if(pixel == 5) {
		GRect rect = GRect(x, y, BLOCK_SIZE, BLOCK_SIZE);
		graphics_fill_rect(ctx, rect, 0, GCornerNone);
	} else if(pixel == 7) {
		
		// TODO stopped here...
		my_path_rect = gpath_create(&PATH_RECT);
		gpath_move_to(my_path_rect, GPoint(x, y-1)); // CURRENTLY NOT SURE WHY THERE IS THIS SHIFT OF ONE PIXEL
		gpath_draw_filled(ctx, my_path_rect);
		// gpath_destroy(my_path_rect);
	}
}

static void draw_number(Layer *layer, GContext *ctx, const Config config, int digit, int posX, int posY, DrawRoutine draw) {
	for(int i = 0; i < NUM_LENGTH; i++) {
		int row = i / NUM_COLS;
		int col = i % NUM_COLS;
		int y = posY + (row * BLOCK_SIZE) + (row * BLOCK_SPACE);
		int x = posX + (col * BLOCK_SIZE) + (col * BLOCK_SPACE);
		draw(ctx, config, digit, i, x, y);
	}
}

static Config config = { NUMBERS, random_color };

static void my_layer_draw(Layer *layer, GContext *ctx) {
	
	GRect bounds = layer_get_bounds(layer);
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

	time_t temp = time(NULL); 
  struct tm *tick_time = localtime(&temp);
	
	int d1 = tick_time->tm_hour / 10;
	int d2 = tick_time->tm_hour % 10;
	int d3 = tick_time->tm_min / 10;
	int d4 = tick_time->tm_min % 10;
	
	DrawRoutine drawRoutine = draw_plainblock;
	
	draw_number(layer, ctx, config, d1, 40, 30, drawRoutine);
	draw_number(layer, ctx, config, d2, 80, 30, drawRoutine);
	draw_number(layer, ctx, config, d3, 40, 90, drawRoutine);
	draw_number(layer, ctx, config, d4, 80, 90, drawRoutine);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
	layer_mark_dirty(my_layer);
}

static void my_window_load(Window *window) {
	Layer *window_layer = window_get_root_layer(window);
  GRect window_bounds = layer_get_bounds(window_layer);
	
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

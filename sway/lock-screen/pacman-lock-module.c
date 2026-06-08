#include <gdk/gdkkeysyms.h>
#include <math.h>
#include <pwd.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "gtklock-module.h"

const gchar module_name[] = "pacman-lock";
const guint module_major_version = 4;
const guint module_minor_version = 0;

#define ROWS 17
#define COLS 21
#define GHOSTS 4

static const char *level[ROWS] = {
	"#####################",
	"#.........#.........#",
	"#.###.###.#.###.###.#",
	"#...................#",
	"#.###.#.#####.#.###.#",
	"#.....#...#...#.....#",
	"#####.### # ###.#####",
	"    #.#       #.#    ",
	"#####.# ## ## #.#####",
	"     .         .     ",
	"#####.# ##### #.#####",
	"    #.#       #.#    ",
	"#####.# ##### #.#####",
	"#.........#.........#",
	"#.###.###.#.###.###.#",
	"#o..#..... .....#..o#",
	"#####################",
};

static int self_id = -1;

struct Actor {
	int x;
	int y;
	int dx;
	int dy;
};

struct PacmanLockState {
	GtkWidget *area;
	struct Window *win;
	guint tick_source;
	guint hide_source;
	gulong key_handler;
	gint64 last_us;
	gint64 start_us;
	double pause_time;
	double step_accum;
	gboolean form_visible;
	gboolean pellets[ROWS][COLS];
	int pellet_count;
	int score;
	int deaths;
	struct Actor pacman;
	struct Actor ghosts[GHOSTS];
};

static void rgba(cairo_t *cr, double r, double g, double b, double a) {
	cairo_set_source_rgba(cr, r / 255.0, g / 255.0, b / 255.0, a);
}

static void circle(cairo_t *cr, double x, double y, double r) {
	cairo_arc(cr, x, y, r, 0, G_PI * 2);
	cairo_fill(cr);
}

static void rounded_rect(cairo_t *cr, double x, double y, double w, double h, double r) {
	cairo_new_sub_path(cr);
	cairo_arc(cr, x + w - r, y + r, r, -G_PI / 2, 0);
	cairo_arc(cr, x + w - r, y + h - r, r, 0, G_PI / 2);
	cairo_arc(cr, x + r, y + h - r, r, G_PI / 2, G_PI);
	cairo_arc(cr, x + r, y + r, r, G_PI, G_PI * 1.5);
	cairo_close_path(cr);
}

static int wrap_x(int x) {
	if (x < 0) {
		return COLS - 1;
	}
	if (x >= COLS) {
		return 0;
	}
	return x;
}

static gboolean wall_at(int x, int y) {
	if (y < 0 || y >= ROWS) {
		return TRUE;
	}
	x = wrap_x(x);
	return level[y][x] == '#';
}

static int manhattan_wrap(int ax, int ay, int bx, int by) {
	int dx = abs(ax - bx);
	dx = MIN(dx, COLS - dx);
	return dx + abs(ay - by);
}

static void init_actors(struct PacmanLockState *state) {
	state->pacman = (struct Actor){10, 13, 1, 0};
	state->ghosts[0] = (struct Actor){9, 8, -1, 0};
	state->ghosts[1] = (struct Actor){10, 8, 1, 0};
	state->ghosts[2] = (struct Actor){11, 8, 0, -1};
	state->ghosts[3] = (struct Actor){10, 9, 0, 1};
}

static void init_level(struct PacmanLockState *state) {
	state->pellet_count = 0;
	memset(state->pellets, 0, sizeof(state->pellets));
	for (int y = 0; y < ROWS; y++) {
		for (int x = 0; x < COLS; x++) {
			if (level[y][x] == '.' || level[y][x] == 'o') {
				state->pellets[y][x] = TRUE;
				state->pellet_count++;
			}
		}
	}
	state->score = 0;
	state->deaths = 0;
	init_actors(state);
}

static int nearest_pellet_distance(struct PacmanLockState *state, int x, int y) {
	int best = 999;
	for (int py = 0; py < ROWS; py++) {
		for (int px = 0; px < COLS; px++) {
			if (state->pellets[py][px]) {
				best = MIN(best, manhattan_wrap(x, y, px, py));
			}
		}
	}
	return best;
}

static int ghost_danger(struct PacmanLockState *state, int x, int y) {
	int danger = 0;
	for (int i = 0; i < GHOSTS; i++) {
		int d = manhattan_wrap(x, y, state->ghosts[i].x, state->ghosts[i].y);
		if (d == 0) {
			danger += 1000;
		} else if (d < 5) {
			danger += (5 - d) * 28;
		}
	}
	return danger;
}

static gboolean can_move(int x, int y, int dx, int dy) {
	return !wall_at(x + dx, y + dy);
}

static void choose_pacman_dir(struct PacmanLockState *state) {
	int dirs[4][2] = {{1, 0}, {0, 1}, {-1, 0}, {0, -1}};
	int best = 100000;
	int best_dx = state->pacman.dx;
	int best_dy = state->pacman.dy;

	for (int i = 0; i < 4; i++) {
		int dx = dirs[i][0];
		int dy = dirs[i][1];
		if (!can_move(state->pacman.x, state->pacman.y, dx, dy)) {
			continue;
		}
		int nx = wrap_x(state->pacman.x + dx);
		int ny = state->pacman.y + dy;
		int score = nearest_pellet_distance(state, nx, ny) * 12 + ghost_danger(state, nx, ny);
		if (dx == -state->pacman.dx && dy == -state->pacman.dy) {
			score += 18;
		}
		if (state->pellets[ny][nx]) {
			score -= 24;
		}
		if (score < best) {
			best = score;
			best_dx = dx;
			best_dy = dy;
		}
	}

	state->pacman.dx = best_dx;
	state->pacman.dy = best_dy;
}

static void choose_ghost_dir(struct PacmanLockState *state, int index) {
	struct Actor *ghost = &state->ghosts[index];
	int dirs[4][2] = {{1, 0}, {0, 1}, {-1, 0}, {0, -1}};
	int best = 100000;
	int best_dx = ghost->dx;
	int best_dy = ghost->dy;

	for (int i = 0; i < 4; i++) {
		int dx = dirs[i][0];
		int dy = dirs[i][1];
		if (!can_move(ghost->x, ghost->y, dx, dy)) {
			continue;
		}
		if (dx == -ghost->dx && dy == -ghost->dy) {
			continue;
		}
		int nx = wrap_x(ghost->x + dx);
		int ny = ghost->y + dy;
		int target = manhattan_wrap(nx, ny, state->pacman.x, state->pacman.y) * 10;
		int personality = ((nx * 17 + ny * 31 + index * 19 + state->score) % 7);
		int score = target + personality;
		if (score < best) {
			best = score;
			best_dx = dx;
			best_dy = dy;
		}
	}

	if (!can_move(ghost->x, ghost->y, best_dx, best_dy)) {
		for (int i = 0; i < 4; i++) {
			if (can_move(ghost->x, ghost->y, dirs[i][0], dirs[i][1])) {
				best_dx = dirs[i][0];
				best_dy = dirs[i][1];
				break;
			}
		}
	}

	ghost->dx = best_dx;
	ghost->dy = best_dy;
}

static void move_actor(struct Actor *actor) {
	if (!can_move(actor->x, actor->y, actor->dx, actor->dy)) {
		return;
	}
	actor->x = wrap_x(actor->x + actor->dx);
	actor->y += actor->dy;
}

static void game_step(struct PacmanLockState *state) {
	choose_pacman_dir(state);
	move_actor(&state->pacman);

	if (state->pellets[state->pacman.y][state->pacman.x]) {
		state->pellets[state->pacman.y][state->pacman.x] = FALSE;
		state->pellet_count--;
		state->score += level[state->pacman.y][state->pacman.x] == 'o' ? 50 : 10;
	}

	for (int i = 0; i < GHOSTS; i++) {
		choose_ghost_dir(state, i);
		move_actor(&state->ghosts[i]);
	}

	for (int i = 0; i < GHOSTS; i++) {
		if (state->pacman.x == state->ghosts[i].x && state->pacman.y == state->ghosts[i].y) {
			state->deaths++;
			init_actors(state);
			break;
		}
	}

	if (state->pellet_count <= 0) {
		init_level(state);
	}
}

static void advance_game(struct PacmanLockState *state) {
	gint64 now = g_get_monotonic_time();
	double dt = (now - state->last_us) / 1000000.0;
	state->last_us = now;
	state->step_accum += dt;
	while (state->step_accum >= 0.105) {
		game_step(state);
		state->step_accum -= 0.105;
	}
}

static double actor_angle(struct Actor actor) {
	if (actor.dx > 0) {
		return 0;
	}
	if (actor.dx < 0) {
		return G_PI;
	}
	if (actor.dy > 0) {
		return G_PI / 2;
	}
	return -G_PI / 2;
}

static void draw_pacman(cairo_t *cr, double x, double y, double radius, struct Actor actor, double t) {
	double angle = actor_angle(actor);
	double mouth = 0.16 + 0.24 * fabs(sin(t * G_PI * 8));
	rgba(cr, 255, 212, 62, 1);
	cairo_move_to(cr, x, y);
	cairo_arc(cr, x, y, radius, angle + mouth * G_PI, angle + (2 - mouth) * G_PI);
	cairo_close_path(cr);
	cairo_fill(cr);

	rgba(cr, 12, 12, 18, 1);
	circle(cr, x + cos(angle - G_PI / 3) * radius * 0.45, y + sin(angle - G_PI / 3) * radius * 0.45, radius * 0.09);
}

static void draw_ghost(cairo_t *cr, double x, double y, double size, double r, double g, double b, double t) {
	double bob = sin(t * 8 + x * 0.01) * size * 0.08;
	y += bob;
	rgba(cr, r, g, b, 1);
	cairo_arc(cr, x, y - size * 0.22, size * 0.42, G_PI, G_PI * 2);
	cairo_rectangle(cr, x - size * 0.42, y - size * 0.22, size * 0.84, size * 0.58);
	cairo_fill(cr);
	for (int i = 0; i < 4; i++) {
		circle(cr, x - size * 0.32 + i * size * 0.21, y + size * 0.36, size * 0.12);
	}

	rgba(cr, 238, 245, 255, 1);
	circle(cr, x - size * 0.15, y - size * 0.22, size * 0.10);
	circle(cr, x + size * 0.16, y - size * 0.22, size * 0.10);
	rgba(cr, 24, 45, 120, 1);
	circle(cr, x - size * 0.11, y - size * 0.20, size * 0.045);
	circle(cr, x + size * 0.20, y - size * 0.20, size * 0.045);
}

static void draw_board(struct PacmanLockState *state, cairo_t *cr, int width, int height, double t) {
	cairo_pattern_t *bg = cairo_pattern_create_linear(0, 0, 0, height);
	cairo_pattern_add_color_stop_rgba(bg, 0, 6 / 255.0, 10 / 255.0, 22 / 255.0, 1);
	cairo_pattern_add_color_stop_rgba(bg, 1, 2 / 255.0, 18 / 255.0, 30 / 255.0, 1);
	cairo_set_source(cr, bg);
	cairo_paint(cr);
	cairo_pattern_destroy(bg);

	double cell = MIN((width - 120.0) / COLS, (height - 165.0) / ROWS);
	double board_w = cell * COLS;
	double board_h = cell * ROWS;
	double ox = (width - board_w) / 2.0;
	double oy = (height - board_h) / 2.0 - 18;

	rgba(cr, 4, 8, 18, 0.78);
	rounded_rect(cr, ox - 18, oy - 18, board_w + 36, board_h + 36, 10);
	cairo_fill(cr);

	for (int y = 0; y < ROWS; y++) {
		for (int x = 0; x < COLS; x++) {
			double px = ox + x * cell;
			double py = oy + y * cell;
			if (level[y][x] == '#') {
				rgba(cr, 34, 94, 224, 1);
				rounded_rect(cr, px + cell * 0.08, py + cell * 0.08, cell * 0.84, cell * 0.84, cell * 0.18);
				cairo_fill(cr);
				rgba(cr, 74, 140, 255, 0.45);
				cairo_rectangle(cr, px + cell * 0.18, py + cell * 0.16, cell * 0.64, cell * 0.10);
				cairo_fill(cr);
			} else if (state->pellets[y][x]) {
				double radius = level[y][x] == 'o' ? cell * 0.18 : cell * 0.07;
				rgba(cr, 246, 220, 156, 0.92);
				circle(cr, px + cell / 2, py + cell / 2, radius + sin(t * 5 + x) * cell * 0.012);
			}
		}
	}

	double actor_size = cell * 0.78;
	double px = ox + state->pacman.x * cell + cell / 2;
	double py = oy + state->pacman.y * cell + cell / 2;
	draw_pacman(cr, px, py, actor_size * 0.45, state->pacman, t);

	double colors[GHOSTS][3] = {{235, 59, 72}, {67, 221, 237}, {255, 160, 52}, {255, 120, 190}};
	for (int i = 0; i < GHOSTS; i++) {
		double gx = ox + state->ghosts[i].x * cell + cell / 2;
		double gy = oy + state->ghosts[i].y * cell + cell / 2;
		draw_ghost(cr, gx, gy, actor_size, colors[i][0], colors[i][1], colors[i][2], t + i);
	}

	cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
	cairo_set_font_size(cr, 18);
	char stats[96];
	snprintf(stats, sizeof(stats), "score %05d   pellets %03d   resets %d", state->score, state->pellet_count, state->deaths);
	cairo_text_extents_t extents;
	cairo_text_extents(cr, stats, &extents);
	rgba(cr, 244, 247, 255, 0.92);
	cairo_move_to(cr, ox + board_w - extents.width, oy - 32);
	cairo_show_text(cr, stats);

	if (!state->form_visible) {
		const char *hint = "Press any key to unlock";
		cairo_text_extents(cr, hint, &extents);
		rgba(cr, 7, 11, 22, 0.82);
		rounded_rect(cr, width / 2.0 - 156, height - 92, 312, 44, 6);
		cairo_fill(cr);
		rgba(cr, 244, 247, 255, 0.88);
		cairo_move_to(cr, width / 2.0 - extents.width / 2, height - 64);
		cairo_show_text(cr, hint);
	}
}

static gboolean draw_game(GtkWidget *widget, cairo_t *cr, gpointer user_data) {
	struct PacmanLockState *state = user_data;
	GtkAllocation alloc;
	gtk_widget_get_allocation(widget, &alloc);
	double t = state->form_visible ? state->pause_time : (g_get_monotonic_time() - state->start_us) / 1000000.0;
	draw_board(state, cr, alloc.width, alloc.height, t);
	return FALSE;
}

static gboolean tick(gpointer user_data) {
	struct PacmanLockState *state = user_data;
	if (!state->form_visible) {
		advance_game(state);
	}
	if (GTK_IS_WIDGET(state->area)) {
		gtk_widget_queue_draw(state->area);
		return G_SOURCE_CONTINUE;
	}
	return G_SOURCE_REMOVE;
}

static GtkWidget *find_named(GtkWidget *root, const char *name) {
	if (g_strcmp0(gtk_widget_get_name(root), name) == 0) {
		return root;
	}
	if (!GTK_IS_CONTAINER(root)) {
		return NULL;
	}
	GList *children = gtk_container_get_children(GTK_CONTAINER(root));
	for (GList *l = children; l != NULL; l = l->next) {
		GtkWidget *match = find_named(GTK_WIDGET(l->data), name);
		if (match) {
			g_list_free(children);
			return match;
		}
	}
	g_list_free(children);
	return NULL;
}

static const char *username(void) {
	struct passwd *pw = getpwuid(getuid());
	if (pw && pw->pw_name) {
		return pw->pw_name;
	}
	const char *user = g_getenv("USER");
	return user ? user : "user";
}

static void hide_form(struct PacmanLockState *state) {
	if (!state || state->form_visible || !state->win) {
		return;
	}
	gtk_revealer_set_reveal_child(GTK_REVEALER(state->win->body_revealer), FALSE);
	gtk_widget_hide(state->win->window_box);
	gtk_widget_queue_draw(state->area);
}

static gboolean hide_form_later(gpointer user_data) {
	struct PacmanLockState *state = user_data;
	state->hide_source = 0;
	hide_form(state);
	return G_SOURCE_REMOVE;
}

static gboolean reveal_form(GtkWidget *widget, GdkEventKey *event, gpointer user_data) {
	(void)widget;
	(void)event;
	struct PacmanLockState *state = user_data;
	if (!state->form_visible) {
		state->pause_time = (g_get_monotonic_time() - state->start_us) / 1000000.0;
		state->form_visible = TRUE;
		gtk_widget_show_all(state->win->window_box);
		gtk_revealer_set_reveal_child(GTK_REVEALER(state->win->body_revealer), TRUE);
		gtk_entry_grab_focus_without_selecting(GTK_ENTRY(state->win->input_field));
		gtk_widget_queue_draw(state->area);
		return TRUE;
	}
	return FALSE;
}

void on_activation(struct GtkLock *gtklock, int id) {
	(void)gtklock;
	self_id = id;
}

void on_window_create(struct GtkLock *gtklock, struct Window *win) {
	(void)gtklock;
	struct PacmanLockState *state = g_malloc0(sizeof(*state));
	state->win = win;
	state->start_us = g_get_monotonic_time();
	state->last_us = state->start_us;
	init_level(state);

	state->area = gtk_drawing_area_new();
	gtk_widget_set_name(state->area, "pacman-game");
	gtk_widget_set_hexpand(state->area, TRUE);
	gtk_widget_set_vexpand(state->area, TRUE);
	gtk_widget_set_halign(state->area, GTK_ALIGN_FILL);
	gtk_widget_set_valign(state->area, GTK_ALIGN_FILL);
	g_signal_connect(state->area, "draw", G_CALLBACK(draw_game), state);

	g_object_ref(win->window_box);
	gtk_container_remove(GTK_CONTAINER(win->overlay), win->window_box);
	gtk_container_add(GTK_CONTAINER(win->overlay), state->area);
	gtk_overlay_add_overlay(GTK_OVERLAY(win->overlay), win->window_box);
	g_object_unref(win->window_box);
	gtk_widget_show(state->area);

	gtk_revealer_set_reveal_child(GTK_REVEALER(win->body_revealer), FALSE);
	gtk_widget_hide(win->window_box);
	gtk_widget_add_events(win->window, GDK_KEY_PRESS_MASK);
	state->key_handler = g_signal_connect(win->window, "key-press-event", G_CALLBACK(reveal_form), state);

	state->tick_source = g_timeout_add(33, tick, state);
	win->module_data[self_id] = state;

	GtkWidget *user_label = find_named(win->window_box, "user-label");
	if (user_label && GTK_IS_LABEL(user_label)) {
		gtk_label_set_text(GTK_LABEL(user_label), username());
	}

	state->hide_source = g_idle_add(hide_form_later, state);
}

void on_idle_show(struct GtkLock *gtklock) {
	(void)gtklock;
	if (self_id < 0) {
		return;
	}
	for (guint i = 0; gtklock && i < gtklock->windows->len; i++) {
		struct Window *win = g_array_index(gtklock->windows, struct Window *, i);
		if (win && win->module_data[self_id]) {
			hide_form(win->module_data[self_id]);
		}
	}
}

void on_window_destroy(struct GtkLock *gtklock, struct Window *win) {
	(void)gtklock;
	if (self_id < 0 || !win->module_data[self_id]) {
		return;
	}
	struct PacmanLockState *state = win->module_data[self_id];
	if (state->tick_source) {
		g_source_remove(state->tick_source);
	}
	if (state->hide_source) {
		g_source_remove(state->hide_source);
	}
	if (state->key_handler && GTK_IS_WIDGET(win->window)) {
		g_signal_handler_disconnect(win->window, state->key_handler);
	}
	g_free(state);
	win->module_data[self_id] = NULL;
}

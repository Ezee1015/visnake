#include <stdlib.h>
#include <time.h>
#include <ncurses.h>
#include <unistd.h>

#define NOKEY -1
#define BIND_EXIT 'q'
#define BIND_UP 'k'
#define BIND_DOWN 'j'
#define BIND_LEFT 'h'
#define BIND_RIGHT 'l'
#define BIND_PAUSE 27 // Escape
#define BIND_RELOAD 'r'

#define SIZE_X 25
#define SIZE_Y 25

#define SLEEP_PER_FRAME 100
#define START_POINTS 5

typedef struct {
  int x;
  int y;
} point;

struct point_list {
  point point;

  struct point_list *next;
  struct point_list *prev;
};
typedef struct point_list point_list;

typedef struct {
  enum { DIR_UP, DIR_DOWN, DIR_LEFT, DIR_RIGHT } direction;
  point_list *head; // cyclical list
} Snake;

void add_head(Snake *s, point p) {
  if (!s) return;

  point_list *new = malloc(sizeof(point_list));
  if (!new) return;
  new->point = p;

  if (!s->head) {
    s->head = new;
    new->next = NULL;
    new->prev = NULL;
    return;
  }

  if (!s->head->prev) {
    new->next = s->head;
    new->prev = s->head;
    s->head->prev = new;
    s->head->next = new;
    s->head = new;
    return;
  }

  new->next = s->head;
  new->prev = s->head->prev;

  s->head->prev->next = new;
  s->head->prev = new;
  s->head = new;
}

void init_game(Snake *s) {
  if (!s) return;

  point head = {
    .x = START_POINTS + rand() % (SIZE_X - START_POINTS*2),
    .y = START_POINTS + rand() % (SIZE_Y - START_POINTS*2),
  };

  bool horizontal = rand() % 2;
  int side;
  if (horizontal) {
    if (head.x > SIZE_X - head.x) {
      side = 1;
      s->direction = DIR_LEFT;
    } else {
      side = -1;
      s->direction = DIR_RIGHT;
    }

    for (int i=START_POINTS; i>=0; i--) {
      point p = head;
      p.x = p.x + side * i;
      add_head(s, p);
    }
  } else {
    if (head.y > SIZE_Y - head.y) {
      side = 1;
      s->direction = DIR_UP;
    } else {
      side = -1;
      s->direction = DIR_DOWN;
    }

    for (int i=START_POINTS; i>=0; i--) {
      point p = head;
      p.y = p.y + side * i;
      add_head(s, p);
    }
  }
}

void move_snake(Snake *s) {
  if (!s || !s->head || !s->head->prev) return;

  // Use the tail of the snake as the new head
  s->head = s->head->prev;

  s->head->point = s->head->next->point;
  switch (s->direction) {
    case DIR_UP:    s->head->point.y--; break;
    case DIR_DOWN:  s->head->point.y++; break;
    case DIR_LEFT:  s->head->point.x--; break;
    case DIR_RIGHT: s->head->point.x++; break;
  }
}

void check_boundaries(Snake *s) {
  if (!s || !s->head) return;

  if (s->head->point.x < 0) s->head->point.x = SIZE_X-1;
  if (s->head->point.y < 0) s->head->point.y = SIZE_Y-1;

  if (s->head->point.x >= SIZE_X) s->head->point.x = 0;
  if (s->head->point.y >= SIZE_Y) s->head->point.y = 0;
}

void draw_snake(Snake s, int start_x, int start_y) {
  if (!s.head) return;

  // Head
  mvprintw(start_y + s.head->point.y, start_x + s.head->point.x, "@");
  // Body
  point_list *list = s.head->next;
  while (list != s.head) {
    mvprintw(start_y + list->point.y, start_x + list->point.x, "#");
    list = list->next;
  }
}

char read_key() {
  timeout(SLEEP_PER_FRAME);
  return getch();
}

void free_snake(Snake *s) {
  if (!s || !s->head) return;

  point_list *l = s->head->next;
  while (l != s->head) {
    point_list *p = l;
    l = l->next;
    free(p);
  }
  free(s->head);
  s->head = NULL;
}

void draw_frame(int start_x, int start_y) {
  // Top
  move(start_y-1, start_x-1);
  for (int i=0; i<SIZE_X+2; i++) printw("-");

  // Left & Right
  for (int i = 0; i<SIZE_Y+1; i++) {
    mvprintw(start_y + i, start_x - 1, "|");
    mvprintw(start_y + i, start_x + SIZE_X, "|");
  }

  // Bottom
  move(start_y+SIZE_Y, start_x-1);
  for (int i=0; i<SIZE_X+2; i++) printw("-");
}

int str_len(char* str) {
  char *end = str;
  while (*end) end++;
  return end - str;
}

void show_message(char *msg, int scr_width, int scr_height) {
  size_t msg_len = str_len(msg);
  point p = {
    .x = (scr_width-msg_len)/2 - 2,
    .y = scr_height/2 - 1,
  };

  // Top
  move(p.y, p.x);
  for (int i=0; i<msg_len+4; i++) printw("-");

  mvprintw(p.y+1, p.x, "| %s |", msg);

  // Bottom
  move(p.y+2, p.x);
  for (int i=0; i<msg_len+4; i++) printw("-");
}

int main() {
  srand(time(NULL));
  WINDOW *win = initscr();
  noecho();
  curs_set(0);

  Snake snake = {0};
  init_game(&snake);

  bool paused = false;
  char c = 0;
  while ( (c = read_key()) != BIND_EXIT ) {
    clear();
    int width = getmaxx(win), height = getmaxy(win);

    if (width < SIZE_X+2) {
      printw("Window is not width enough...");
    } else if (height < SIZE_Y+2) {
      printw("Window is not height enough...");
    } else {
      int start_x = (width-SIZE_X)/2, start_y = (height-SIZE_Y)/2;
      if (c == BIND_PAUSE) paused = !paused;

      if (!paused) {
        switch (c) {
          case BIND_UP:     if (snake.direction != DIR_DOWN) snake.direction = DIR_UP; break;
          case BIND_LEFT:   if (snake.direction != DIR_RIGHT) snake.direction = DIR_LEFT; break;
          case BIND_RIGHT:  if (snake.direction != DIR_LEFT) snake.direction = DIR_RIGHT; break;
          case BIND_DOWN:   if (snake.direction != DIR_UP) snake.direction = DIR_DOWN; break;
          case BIND_RELOAD:
            free_snake(&snake);
            init_game(&snake);
            break;
        }

        move_snake(&snake);
        check_boundaries(&snake);
      }

      draw_frame(start_x, start_y);
      draw_snake(snake, start_x, start_y);

      if (paused) show_message("PAUSED", width, height);
    }
  }

  free_snake(&snake);
  if (endwin() == ERR) return 1;
  return 0;
}

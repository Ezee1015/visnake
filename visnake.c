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
  point_list *head;
  point_list *tail;
} Snake;

void init_game(Snake *s) {
  if (!s) return;

  s->head = malloc(sizeof(point_list));
  if (!s->head) return;
  s->head->next = malloc(sizeof(point_list));
  if (!s->head->next) return;
  s->head->next->next = malloc(sizeof(point_list));
  if (!s->head->next->next) return;

  s->head->next->next->next = NULL;
  s->head->prev = NULL;
  s->head->next->prev = s->head;
  s->head->next->next->prev = s->head->next;

  s->head->point.x = 3 + rand() % (SIZE_X-6);
  s->head->point.y = 3 + rand() % (SIZE_Y-6);

  bool horizontal = rand() % 2;
  if (horizontal) {
    s->head->next->point.y = s->head->point.y;
    s->head->next->next->point.y = s->head->point.y;

    int side = (s->head->point.x > SIZE_X - s->head->point.x)
               ? 1
               : -1;

    s->head->next->point.x = s->head->point.x + side;
    s->head->next->next->point.x = s->head->next->point.x + side;

    if (side == 1) s->direction = DIR_LEFT;
    else s->direction = DIR_RIGHT;
  } else {
    s->head->next->point.x = s->head->point.x;
    s->head->next->next->point.x = s->head->point.x;

    int side = (s->head->point.y > SIZE_Y - s->head->point.y)
               ? 1
               : -1;

    s->head->next->point.y = s->head->point.y + side;
    s->head->next->next->point.y = s->head->next->point.y + side;

    if (side == 1) s->direction = DIR_UP;
    else s->direction = DIR_DOWN;
  }

  s->tail = s->head->next->next;
}

void move_snake(Snake *s) {
  if (!s || !s->head || !s->tail || s->head == s->tail) return;

  // Use the tail of the snake as the new head
  point_list *new_head = s->tail;
  s->tail = new_head->prev;
  s->tail->next = NULL;

  s->head->prev = new_head;
  new_head->prev = NULL;
  new_head->next = s->head;
  s->head = new_head;

  new_head->point = new_head->next->point;
  switch (s->direction) {
    case DIR_UP:    new_head->point.y--; break;
    case DIR_DOWN:  new_head->point.y++; break;
    case DIR_LEFT:  new_head->point.x--; break;
    case DIR_RIGHT: new_head->point.x++; break;
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
  while (list) {
    mvprintw(start_y + list->point.y, start_x + list->point.x, "#");
    list = list->next;
  }
}

char read_key() {
  timeout(SLEEP_PER_FRAME);
  return getch();
}

void free_snake(Snake *s) {
  while (s->head) {
    point_list *p = s->head;

    s->head = s->head->next;
    free(p);
  }
  s->tail = NULL;
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

      if (paused) {
        paused = (c != BIND_PAUSE);
      } else {
        switch (c) {
          case BIND_UP: snake.direction = DIR_UP; break;
          case BIND_LEFT: snake.direction = DIR_LEFT; break;
          case BIND_RIGHT: snake.direction = DIR_RIGHT; break;
          case BIND_DOWN: snake.direction = DIR_DOWN; break;
          case BIND_PAUSE: paused = true; break;
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
    }
  }

  free_snake(&snake);
  if (endwin() == ERR) return 1;
  return 0;
}

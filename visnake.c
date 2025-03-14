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

#define SIZE_X 50
#define SIZE_Y 25

#define SLEEP_PER_FRAME 90
#define START_POINTS 5 // Maximum value = minimum{SIZE_X, SIZE_Y}/2 - 1

typedef struct {
  int x;
  int y;
} Point;

typedef struct {
  Point body[SIZE_X*SIZE_Y];
  size_t head_pos;
  size_t length;

  Point food;

  enum { DIR_UP, DIR_DOWN, DIR_LEFT, DIR_RIGHT } direction;
  bool dead;
  bool won;
} Snake;

#define POINT_CMP(p1, p2) ( (p1.x == p2.x) && (p1.y == p2.y) )

void generate_food(Snake *s) {
  if (!s) return;

  bool empty_spot = false;
  Point spot;
  while (!empty_spot) {
    spot = (Point) {
      .x = rand() % SIZE_X,
      .y = rand() % SIZE_Y
    };

    int i=0;
    while (!empty_spot && i<s->length) {
      int pos = s->head_pos-i;
      if (pos < 0) pos += SIZE_X*SIZE_Y;
      empty_spot = !POINT_CMP(spot, s->body[pos]);
      i++;
    }
  }
  s->food = spot;
}

void init_game(Snake *s) {
  if (!s) return;

  s->head_pos = START_POINTS-1;
  s->length = START_POINTS;
  s->dead = false;
  s->won = false;

  Point head = {
    .x = START_POINTS + rand() % (SIZE_X - START_POINTS*2),
    .y = START_POINTS + rand() % (SIZE_Y - START_POINTS*2),
  };

  int side;
  bool horizontal = rand() % 2;
  if (horizontal) {
    if (head.x > SIZE_X - head.x) {
      side = 1;
      s->direction = DIR_LEFT;
    } else {
      side = -1;
      s->direction = DIR_RIGHT;
    }

    for (int i=0; i<START_POINTS; i++) {
      Point p = head;
      p.x = p.x + side * i;
      s->body[START_POINTS-1-i] = p;
    }
  } else {
    if (head.y > SIZE_Y - head.y) {
      side = 1;
      s->direction = DIR_UP;
    } else {
      side = -1;
      s->direction = DIR_DOWN;
    }

    for (int i=0; i<START_POINTS; i++) {
      Point p = head;
      p.y = p.y + side * i;
      s->body[START_POINTS-1-i] = p;
    }
  }

  generate_food(s);
}

void move_snake(Snake *s) {
  if (!s) return;

  Point new_pos = s->body[s->head_pos];
  switch (s->direction) {
    case DIR_UP:    new_pos.y--; break;
    case DIR_DOWN:  new_pos.y++; break;
    case DIR_LEFT:  new_pos.x--; break;
    case DIR_RIGHT: new_pos.x++; break;
  }

  // Check boundaries
  if (new_pos.x < 0) new_pos.x = SIZE_X-1;
  if (new_pos.y < 0) new_pos.y = SIZE_Y-1;
  if (new_pos.x >= SIZE_X) new_pos.x = 0;
  if (new_pos.y >= SIZE_Y) new_pos.y = 0;

  bool food = POINT_CMP(new_pos, s->food);
  if (food) {
    if (++s->length == SIZE_X*SIZE_Y) {
      s->won = true;
      return;
    }
    generate_food(s);
  }

  // If the snake eats the food, as we are adding one element to the snake, we
  // should check if that piece collides with the other elements of the body.
  //
  // If it doesn't eat the food, we are "deleting" the last piece of the snake
  // in order to put the new head. We should check if that new piece collides
  // with the other elements of the body, except the last one that gets "deleted"
  const int length_to_check = (food) ? s->length : s->length-1;
  int i=0;
  while (!s->dead && i<length_to_check) {
    int pos = s->head_pos-i;
    if (pos < 0) pos += SIZE_X*SIZE_Y;
    if (POINT_CMP(new_pos, s->body[pos])) s->dead = true;
    i++;
  }

  s->body[++s->head_pos] = new_pos;
  if (s->head_pos >= SIZE_X*SIZE_Y) s->head_pos = 0;
}

void draw_snake(Snake s, int start_x, int start_y) {
  // Body
  for (int i=s.length-1; i>0; i--) {
    int pos = s.head_pos-i;
    if (pos < 0) pos += SIZE_X*SIZE_Y;
    mvprintw(start_y + s.body[pos].y, start_x + s.body[pos].x, "#");
  }

  // Head
  char h;
  switch (s.direction) {
    case DIR_UP:    h = '^'; break;
    case DIR_LEFT:  h = '<'; break;
    case DIR_RIGHT: h = '>'; break;
    case DIR_DOWN:  h = 'v'; break;
  }
  mvprintw(start_y + s.body[s.head_pos].y, start_x + s.body[s.head_pos].x, "%c", h);
}

char read_key() {
  timeout(SLEEP_PER_FRAME);
  return getch();
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

void draw_message(char *msg, int scr_width, int scr_height) {
  size_t msg_len = str_len(msg);
  Point p = {
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

void draw_points(Snake s, int start_x, int start_y) {
  char *msg = "Points : ";
  const int msg_len = str_len(msg);

  int points_len = 0;
  int p = s.length;
  while (p > 0) {
    p /= 10;
    points_len++;
  }

  mvprintw(start_y-1, start_x + (SIZE_X - msg_len - points_len - 4)/2, "| %s%ld |", msg, s.length);
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

      switch (c) {
        case BIND_PAUSE:  if (!snake.dead && !snake.won) paused = !paused; break;
        case BIND_RELOAD: init_game(&snake); break;
      }

      if (!paused && !snake.dead && !snake.won) {
        switch (c) {
          case BIND_UP:     if (snake.direction != DIR_DOWN) snake.direction = DIR_UP; break;
          case BIND_LEFT:   if (snake.direction != DIR_RIGHT) snake.direction = DIR_LEFT; break;
          case BIND_RIGHT:  if (snake.direction != DIR_LEFT) snake.direction = DIR_RIGHT; break;
          case BIND_DOWN:   if (snake.direction != DIR_UP) snake.direction = DIR_DOWN; break;
        }

        move_snake(&snake);
      }

      mvprintw(start_y + snake.food.y, start_x + snake.food.x, "*");
      draw_snake(snake, start_x, start_y);
      draw_frame(start_x, start_y);
      draw_points(snake, start_x, start_y);

      if (paused) draw_message("PAUSED", width, height);
      if (snake.dead) draw_message("Game over", width, height);
      if (snake.won) draw_message("You won!", width, height);
    }
  }

  if (endwin() == ERR) return 1;
  return 0;
}

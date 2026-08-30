#include <stdio.h>
#include <termios.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <string.h>
/* Initializing the data */

typedef struct erow
{
    int size;
    char *render;
} erow;

struct gameWindow
{
    int window_rows;
    int window_cols;
    erow *row;
    struct termios orig_termios;
};

struct gameWindow E;

struct segment
{
    char symbol;
    int pos_x;
    int pos_y;
    struct segment *next;
};

typedef struct snake
{
    struct segment *body;
    int length;
} snake_data;

snake_data snake;

enum Control
{
    ARROW_UP = 700,
    ARROW_DOWN,
    ARROW_LEFT,
    ARROW_RIGHT
};

/* Append Buffer */
struct abuf
{
    char *b;
    int len;
};

void abBuff(struct abuf *ab, char *c, int len)
{
    char *new = realloc(ab->b, ab->len + len);
    memcpy(&new[ab->len], c, len);
    ab->b = new;
    ab->len += len;
}

void freeBuff(struct abuf *ab)
{
    free(ab->b);
}
/* Setting the terminal */
void disableRawMode()
{
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_termios);
}

void setRawMode()
{
    tcgetattr(STDIN_FILENO, &E.orig_termios);
    atexit(disableRawMode);
    struct termios raw;
    raw.c_iflag &= ~(ICRNL | BRKINT | IXON | ISTRIP | INPCK);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= (CS8);
    raw.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN);

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

/* Get terminal data */
void getWindowSize()
{
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    E.window_rows = w.ws_row;
    E.window_cols = w.ws_col;
}

int readKey()
{
    char c;
    read(STDOUT_FILENO, &c, 1);
    if (c == 'q')
    {
        return 'q';
    }
    else if (c == 32)
    {
        return 32;
    }
    if (c == '\x1b')
    {
        char ch[2];
        if (read(STDOUT_FILENO, &ch[0], 1) != 1)
            return '\x1b';
        if (ch[0] == '[')
        {
            if (read(STDOUT_FILENO, &ch[1], 1) != 1)
                return '\x1b';
            switch (ch[1])
            {
            case 'A':
                return ARROW_UP;
            case 'B':
                return ARROW_DOWN;
            case 'C':
                return ARROW_RIGHT;
            case 'D':
                return ARROW_LEFT;
            }
        }
    }
    return '\x1b';
}

/* Manipulate the Snake*/
void moveSnake(int dir_x, int dir_y)
{
    static int x = 1;
    static int y = 0;

    if (dir_x && x == 0)
    {
        x = dir_x;
        y = 0;
    }
    else if (dir_y && y == 0)
    {
        y = dir_y;
        x = 0;
    }

    struct segment *part = snake.body;
    while (part->next != NULL)
    {
        part->pos_x = part->next->pos_x;
        part->pos_y = part->next->pos_y;
        part = part->next;
    }
    // Now the part is obviously the head
    part->pos_x += x;
    part->pos_y += y;
}

void addSnakeSeg()
{
    struct segment *newSeg = malloc(sizeof(struct segment));
    struct segment *temp = snake.body;
    int pos[2] = {temp->pos_x, temp->pos_y};
    moveSnake(0, 0);
    snake.body = newSeg;
    newSeg->next = temp;
    newSeg->pos_x = pos[0];
    newSeg->pos_y = pos[1];
    newSeg->symbol = 'O';
}

/* Handling Input */
void processKey()
{
    int chc = readKey();
    switch (chc)
    {
    case ARROW_UP:
        moveSnake(0, -1);
        break;
    case ARROW_DOWN:
        moveSnake(0, 1);
        break;
    case ARROW_LEFT:
        moveSnake(-1, 0);
        break;
    case ARROW_RIGHT:
        moveSnake(1, 0);
        break;
    case 'q':
        write(STDOUT_FILENO, "\x1b[2J", 4);
        write(STDOUT_FILENO, "\x1b[H", 3);
        exit(0);
    }
}
/* Drawing on the screen */
void emptyTheGame()
{
    for (int i = 0; i < E.window_rows - 3; i++)
    {
        for (int j = 0; j < E.window_cols - 2; j++)
        {
            E.row[i].render[j] = ' ';
        }
    }
}

void renderSnake()
{
    struct segment *part = snake.body;
    emptyTheGame();
    while (part != NULL)
    {
        E.row[part->pos_y].render[part->pos_x] = part->symbol;
        part = part->next;
    }
}
void drawWindow(struct abuf *ab)
{
    // Drawing the head of the box
    abBuff(ab, "\x1b[100m", 6);
    for (int header = 0; header < E.window_cols; header++)
    {
        abBuff(ab, " ", 1);
    }
    abBuff(ab, "\x1b[49m", 5);
    abBuff(ab, "\r\n", 2);

    // The main box
    for (int row = 0; row < E.window_rows - 3; row++)
    {
        // Left side
        abBuff(ab, "\x1b[100m", 6);
        abBuff(ab, " ", 1);
        abBuff(ab, "\x1b[49m", 5);

        abBuff(ab, E.row[row].render, E.row[row].size - 1);

        // Right side
        abBuff(ab, "\x1b[100m", 6);
        abBuff(ab, " ", 1);
        abBuff(ab, "\x1b[49m", 5);
    }
    // Drawing the bottom of the box
    abBuff(ab, "\x1b[100m", 6);
    for (int footer = 0; footer < E.window_cols; footer++)
    {
        abBuff(ab, " ", 1);
    }
    abBuff(ab, "\x1b[49m", 5);
    abBuff(ab, "\r\n", 2);
}

void refreshScreen()
{
    struct abuf ab = {NULL, 0};
    moveSnake(0, 0);
    renderSnake();
    abBuff(&ab, "\x1b[H", 3);
    drawWindow(&ab);
    write(STDIN_FILENO, ab.b, ab.len);
    freeBuff(&ab);
}

/* MAIN */
void init()
{
    getWindowSize();

    /* Initializing the rows*/
    E.row = malloc(sizeof(erow) * (E.window_rows - 3));
    for (int i = 0; i < E.window_rows - 3; i++)
    {
        E.row[i].render = malloc((E.window_cols - 1) * sizeof(char));
        E.row[i].size = E.window_cols - 1;
        for (int j = 0; j < E.window_cols - 2; j++)
        {
            E.row[i].render[j] = ' ';
        }
        E.row[i].render[E.window_cols - 2] = '\0';
    }

    /* Initiazlizing the snake head */
    snake.length = 1;
    snake.body = malloc(sizeof(struct segment) * 1);
    snake.body->next = NULL;
    snake.body->pos_x = (E.window_cols / 2) - 1;
    snake.body->pos_y = ((E.window_rows - 1) / 2) - 1;
    snake.body->symbol = '@';
}

int main()
{
    setRawMode();
    init();
    while (1)
    {
        refreshScreen();
        processKey();
        usleep(100000);
    }
    disableRawMode();
}
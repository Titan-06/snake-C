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

enum Control
{
    ARROW_UP = 700,
    ARROW_DOWN,
    ARROW_LEFT,
    ARROW_RIGHT
};

/* Setting the terminal */
void disableRawMode()
{
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_termios);
}
void setRawMode()
{
    tcgetattr(STDIN_FILENO, &E.orig_termios);

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
    while (read(STDOUT_FILENO, &c, 1) != 1)
    {
    }
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
/* Drawing on the screen */
void drawWindow()
{
    struct abuf ab = {NULL, 0};
    for (int i = 0; i < E.window_rows - 1; i++)
    {
        for (int j = 0; j < E.window_cols; j++)
        {
            if (i == 0 && j == 0)
            {
                abBuff(&ab, "\x1b[100m", 6);
            }
            abBuff(&ab, " ", 1);
        }
        if (i == 0)
        {
            abBuff(&ab, "\x1b[49m", 5);
        }
        abBuff(&ab, "\r\n", 2);
    }
    write(STDIN_FILENO, ab.b, ab.len);
    freeBuff(&ab);
}

/* MAIN */
int main()
{
    getWindowSize();
    setRawMode();
    drawWindow();
    int chc = readKey();
    printf("%d %d \n", E.window_cols, chc);
    disableRawMode();
}
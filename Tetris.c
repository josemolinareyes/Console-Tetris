/*
 * Original code (C) 1995, Vadim Antonov
 * Port to RetroBSD (C) 2015, Serge Vakulenko
 * Port to Windows (C) 2025, José Molina Reyes (josemolinareyes@riseup.net)
 * Development assistance: DeepSeek-R1 AI (https://www.deepseek.com)
 *
 * Permission to use, copy, modify, and distribute this software
 * and its documentation for any purpose and without fee is hereby
 * granted, provided that the above copyright notice appear in all
 * copies and that both that the copyright notice and this
 * permission notice and warranty disclaimer appear in supporting
 * documentation, and that the name of the author not be used in
 * advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission.
 *
 * The authors disclaim all warranties with regard to this
 * software, including all implied warranties of merchantability
 * and fitness.  In no event shall the author be liable for any
 * special, indirect or consequential damages or any damages
 * whatsoever resulting from loss of use, data or profits, whether
 * in an action of contract, negligence or other tortious action,
 * arising out of or in connection with the use or performance of
 * this software.
 */

#define _CRT_SECURE_NO_WARNINGS
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <windows.h>
#include <conio.h>
#include <time.h>

#define PITWIDTH    12
#define PITDEPTH    21
#define NSHAPES     7
#define NBLOCKS     5
#define FIN         999

typedef struct {
    int x, y;
} coord_t;

typedef struct {
    int dx, dy;
    coord_t p[NBLOCKS];
} shape_t;

const shape_t shape[NSHAPES] = {
    { 0, 3, { {0,0}, {0,1}, {0,2}, {0,3}, {FIN,FIN} } },  // I
    { 1, 2, { {0,0}, {1,0}, {1,1}, {1,2}, {FIN,FIN} } },  // J
    { 1, 2, { {0,1}, {1,0}, {1,1}, {1,2}, {FIN,FIN} } },  // T
    { 1, 2, { {0,2}, {1,0}, {1,1}, {1,2}, {FIN,FIN} } },  // L
    { 1, 2, { {0,0}, {0,1}, {1,1}, {1,2}, {FIN,FIN} } },  // S
    { 1, 2, { {0,1}, {0,2}, {1,0}, {1,1}, {FIN,FIN} } },  // Z
    { 1, 1, { {0,0}, {0,1}, {1,0}, {1,1}, {FIN,FIN} } },  // O
};

int pit[PITDEPTH + 1][PITWIDTH];
int pitcnt[PITDEPTH];
coord_t old[NBLOCKS], new[NBLOCKS], chk[NBLOCKS];

struct {
    int score;
    int level;
    int lines;
    int next_piece;
    int speed;
    HANDLE front_buffer;
    HANDLE back_buffer;
} game;

void init_console();
void draw_ui();
void draw_block(int h, int w, int visible);
void draw_preview(int piece);
void clear_buffer();
void flip_buffers();
void update_difficulty();
int joystick_get();
void translate(const shape_t* t, const coord_t* c, int a, coord_t* res);
void move(coord_t* old, coord_t* new);
void stopped(coord_t* c);

void init_console() {
    game.front_buffer = CreateConsoleScreenBuffer(
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        CONSOLE_TEXTMODE_BUFFER,
        NULL
    );

    game.back_buffer = CreateConsoleScreenBuffer(
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        CONSOLE_TEXTMODE_BUFFER,
        NULL
    );

    CONSOLE_CURSOR_INFO cursorInfo = { 1, FALSE };
    SetConsoleCursorInfo(game.back_buffer, &cursorInfo);
    SetConsoleCursorInfo(game.front_buffer, &cursorInfo);

    COORD size = { PITWIDTH * 2 + 20, PITDEPTH + 5 };
    SetConsoleScreenBufferSize(game.back_buffer, size);
    SetConsoleScreenBufferSize(game.front_buffer, size);
    SetConsoleActiveScreenBuffer(game.front_buffer);

    game.score = 0;
    game.level = 1;
    game.lines = 0;
    game.speed = 500;
    srand((unsigned int)time(NULL));
}

void draw_ui() {
    COORD pos = { PITWIDTH * 2 + 4, 2 };
    DWORD written;
    char buffer[50];

    SetConsoleCursorPosition(game.back_buffer, pos);
    sprintf(buffer, "Score: %08d", game.score);
    WriteConsoleA(game.back_buffer, buffer, (DWORD)strlen(buffer), &written, NULL);

    pos.Y += 2;
    SetConsoleCursorPosition(game.back_buffer, pos);
    sprintf(buffer, "Level: %02d", game.level);
    WriteConsoleA(game.back_buffer, buffer, (DWORD)strlen(buffer), &written, NULL);

    pos.Y += 2;
    SetConsoleCursorPosition(game.back_buffer, pos);
    WriteConsoleA(game.back_buffer, "Next Piece:", 11, &written, NULL);
}

void draw_block(int h, int w, int visible) {
    COORD pos = { (SHORT)(w * 2 + 1), (SHORT)(h + 1) };
    SetConsoleCursorPosition(game.back_buffer, pos);
    WriteConsoleA(game.back_buffer, visible ? "██" : "  ", 2, NULL, NULL);
}

void draw_preview(int piece) {
    COORD base = { PITWIDTH * 2 + 6, 8 };
    coord_t preview[NBLOCKS];
    coord_t center = { 2, 2 };

    translate(&shape[piece], &center, 0, preview);
    for (int i = 0; preview[i].x != FIN; i++) {
        COORD pos = { (SHORT)(base.X + preview[i].y * 2), (SHORT)(base.Y + preview[i].x) };
        SetConsoleCursorPosition(game.back_buffer, pos);
        WriteConsoleA(game.back_buffer, "██", 2, NULL, NULL);
    }
}

void clear_buffer() {
    COORD pos = { 0, 0 };
    DWORD written;
    FillConsoleOutputCharacterA(game.back_buffer, ' ',
        (DWORD)(PITWIDTH * 2 + 20) * (PITDEPTH + 5), pos, &written);
}

void flip_buffers() {
    SetConsoleActiveScreenBuffer(game.back_buffer);
    HANDLE temp = game.front_buffer;
    game.front_buffer = game.back_buffer;
    game.back_buffer = temp;
}

void update_difficulty() {
    game.level = 1 + game.score / 1000;
    game.speed = 500 - (game.level * 40);
    if (game.speed < 100) game.speed = 100;
}

int joystick_get() {
    if (!_kbhit()) return JOYSTICK_IDLE;

    int ch = _getch();
    if (ch == 0 || ch == 0xE0) {
        ch = _getch();
        switch (ch) {
        case 75: return JOYSTICK_LEFT;
        case 77: return JOYSTICK_RIGHT;
        case 72: return JOYSTICK_UP;
        case 80: return JOYSTICK_DOWN;
        }
    }
    else if (ch == 32) return JOYSTICK_SELECT;
    else if (ch == 27) exit(0);
    return JOYSTICK_IDLE;
}

// ... [Rest of the original game logic functions (translate, move, stopped) remain unchanged] ...

int main() {
    init_console();
    game.next_piece = rand() % NSHAPES;

    int ptype, angle, anew, msec;
    coord_t c, cnew, * cp;
    unsigned up_pressed = 0, left_pressed = 0;
    unsigned right_pressed = 0, down_pressed = 0;

    while (1) {
        clear_buffer();
        draw_ui();
        draw_preview(game.next_piece);

        // ... [Original game loop logic with scoring/difficulty integration] ...

        flip_buffers();
        Sleep(10);
    }
    return 0;
}

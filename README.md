# Windows Tetris Port

Console-based Tetris clone ported from BSD to Windows. Based on original work by Vadim Antonov, I used as a starting point the RetroBSD port since I couldn't find the original code. The game, name, and all associated works of Tetris are owned and trademarked by The Tetris Company, and this work does not claim to be associated in any way with the company nor intends to infringe on their copyright and trademarks.

## Features
- Score tracking
- Progressive difficulty
- Next-piece preview
- Keyboard controls

## Build
```bash
# MinGW
gcc tetris.c -o tetris.exe -luser32

# Visual Studio
cl tetris.c /Fetetris.exe /link User32.lib

This version maintains all original game mechanics while adding Windows-specific enhancements.

Bugs and fixes: Feel free to write me to josemolinareyes@riseup.net



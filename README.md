# Tron Lightcycle Game — DE10-Lite

A single-player Tron-inspired light cycle game developed in C with inline assembly for a RISC-V processor on the DE1/DE10-lite FPGA board.

## Features

- **VGA Graphics:** Draws the game arena, obstacles, and player trails with 16-bit RGB565 pixels written directly to the memory-mapped VGA pixel buffer..
- **AI Opponent:** Includes a two layer deep algorithm opponent.
- **Interrupt-Driven Controls:** Uses push-button interrupts for player steering and machine-timer interrupts for periodic game updates.
- **Adjustable Speed:** Tick speed of the game can be controlled with switches on the DE10 board, for up to 64x tick speed by shortening the machine-timer period.
- **Hardware Interface:** Integrates push buttons, switches, LEDs, and seven-segment displays through memory-mapped I/O.
- **Scoring System:** Tracks player scores and displays them on the seven-segment displays.

## Technical Implementation

- **Languages:** C and inline RISC-V assembly
- **Platform:** Intel DE10-Lite FPGA board
- **Concepts:** Memory-mapped I/O, CPU interrupts, timer configuration, VGA graphics, collision detection, and embedded game logic

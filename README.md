# Final Project for SDAI
This repository contains the implementation of a MAS chess engine for the final project of the course "Symbolic and Distributed AI". The project includes a chess engine that can play against you and perform perft tests to validate move generation.

## Features
- A chess engine that can play against a human opponent.
- Perft tests to validate the correctness of move generation.
- A bot test that pits the MAS against a simple chess bot for 100 games, tracking wins, losses, draws, and trust values of the agents.
- A Makefile to build and run the project.

## Building and Running
To run the chess engine, you need to have a C++ compiler and the necessary libraries installed on your system. The project uses a Makefile to manage the build process, so you can simply run the following command in the terminal:
```bash
make
```
This will compile the source code and create the executable files for the MAS, the bot test and the perft test.

To run the MAS and play against the chess engine, you can run the following command:
```bash
make run # with the default mode
make run-debug # run the debug version (more output and checks)
make run-release # run the release version (no debug output and optimizations)
```
This will start the MAS and allow you to play against it through the terminal; the board will be displayed in ASCII format, and you can enter your moves in UCI format (i.e., <from_square><to_square>[<promotion_piece>], "e7e5" for a normal move, "e7e8q" for a promotion to queen).
Alternatively, you can compile and run it manually by running the following commands:
```bash
g++ -o play_mas src/play_mas.cpp src/chess.cpp src/agents.cpp src/polyglot.cpp -std=c++20 -Isrc -O3 -flto -march=native
./play_mas
```

To run the bot test, that makes the MAS play against a simple chess bot for 100 games, you can run the following command:
```bash
make run-bot-test
```
This will start the test and print the results of the games in the terminal. Attention: this test can take a long time to run, since it needs to play 100 games and that can take a couple of hours.
Alternatively, you can compile and run it manually by running the following commands:
```bash
g++ -o bot_test src/bot_test.cpp src/chess.cpp src/agents.cpp src/bot.cpp src/polyglot.cpp src/timer.cpp -std=c++20 -Isrc -O3 -flto -march=native
./bot_test
```

To run the perft test, that checks the correctness of the move generation and the search algorithm, you can run the following command:
```bash
make run-perft-release # maximum performance, no debug output
make run-perft # slower
```
This will start the test and print the results of the perft test in the terminal, including the number of nodes generated at each depth and the performance in nodes per second.
Alternatively, you can compile and run it manually by running the following commands:
```bash
g++ -o perft_test src/perft_test.cpp src/chess.cpp src/timer.cpp -std=c++20 -Isrc -O3 -flto -march=native
./perft_test
```

## Project Structure
- `src/`: Contains the source code for the chess engine and perft tests.
- `build/`: The directory where compiled object files and executables are stored.
- `books/`: Contains the opening book files used by the chess engine.
- `Makefile`: The build script for compiling and running the project.
- `README.md`: This file, containing instructions and information about the project.
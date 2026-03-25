# Final Project for SDAI
This repository contains the implementation of a MAS chess engine for the final project of the course "Symbolic and Distributed AI". The project includes a chess engine that can play against you and perform perft tests to validate move generation.

## Features
- A chess engine that can play against a human opponent.
- Perft tests to validate the correctness of move generation.
- A Makefile to build and run the project.

## Building and Running
To build the project, simply run:
```bash
make
```
This will compile the chess engine and the perft test. You can then run the chess engine with:
```bash
make run-play-mas
```
And run the perft tests with:
```bash
make run-perft
```

## Project Structure
- `src/`: Contains the source code for the chess engine and perft tests.
- `build/`: The directory where compiled object files and executables are stored.
- `Makefile`: The build script for compiling and running the project.
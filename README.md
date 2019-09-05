# img2tex
A simple program that converts images of LaTeX equations from https://main.mimuw.edu.pl to the LaTeX source code.

## Dependencies
- A C++ compiler with C++17 support
- OpenCV library

## Build
To build just run
```sh
make img2tex
```

## Usage
`generated_symbols.db` and `manual_symbols.db` files are essential for the untexing to work -- they need to be placed in the working directory where the `img2tex` is run. Assuming you are satisfied this requirement (e.g. you are in the project directory) you can run:
```sh
./img2tex untex main/3896.png
```
to see decoded LaTeX of the file `main/3896.png`.

 On success `img2tex` prints decoded LaTeX code and exits with 0. Otherwise, it exits with 1 if any error occurs or the image cannot be decoded. It also prints a lot of information about untexing process to `stderr` that can be simply ignored by redirecting it to `/dev/null` e.g.
```sh
./img2tex untex main/3896.png 2> /dev/null
```

There are also other commands you can learn about by running `img2tex` without arguments:
```sh
./img2tex
```

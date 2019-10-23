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

## Helper scripts
Helper scripts like `test_on` are useful when you want to check that every image in a given set of images untexes correctly and if not, then teach `img2tex` how to untex the unrecognized symbols.

Examples using `test_on`:
```sh
# Selected images
./test_on main 3.png 4.png 1000.png
# All images
for a in $(ls main -1v); do ./test_on main $a; done
```

You press enter to go to the next symbol, or type "skip" to omit the current symbol if it is not untexable in any way.
If it stops on an unrecognizable symbol you either teach `img2tex` to recognize one of symbols saved in files `symbol_*` or type "skip" to omit it.
To make teaching easier you can use the below command to teach how to recognize symbol (here `symbol_2`):
```sh
read -r a && echo -E "$a" | ./img2tex learn symbol_2
```
And then type source Tex of symbol saved in file `symbol_2`.

It is also useful to look at `comparison.png` during usage of `test_on` as there are placed: original image and image generated form the result of untexing the original image -- it is easy to visually compare results this way.

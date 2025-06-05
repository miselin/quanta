# Quanta, a Lisp

Quanta is a lightweight Lisp interpreter implemented as an exercise in building interpreters.

## Features

- **Core Lisp Semantics**:

  - Supports conses and atoms.
  - Implements special forms like `quote`, `lambda`, `let`, `cond`, and `define`.
  - Provides primitives for arithmetic (`+`, `-`, `*`, `/`), equality (`eq?`), and list manipulation (`cons`, `car`, `cdr`).

- **Interactive REPL**:

  - Run Quanta interactively to evaluate expressions and explore its features.

## Syntax Overview

Quanta uses a Lisp syntax where programs are composed of expressions. Here are some examples:

### Atoms

```lisp
5          ; Integer
"hello"    ; String
t          ; True
nil        ; False
:keyword   ; Keyword
```

### Conses

```lisp
()         ; Empty list
(1 2)      ; List with two elements
(1 (2))    ; Nested list
(1 (2 3))  ; Nested list with multiple elements
(1 (2 (3))) ; Deeply nested list
(1 . 2)    ; Explicit construction from CAR/CDR pair
```

### Functions

```lisp
(defun sqr (x) (* x x)) ; Defines a function `sqr`
(define squared (let ((x 2)) (sqr x))) ; Uses `let` to bind and call `sqr`
(print squared) ; Prints the value of `squared`
(cond ((eq? squared 4) 0) (t 1)) ; Conditional expression
```

### REPL

```lisp
(defun repl ()
  (begin
    (print "lisp> ")
    (let ((input (read-line)))
      (begin
        (print (eval (read input)))
        (repl)))))
(repl) ; Starts the REPL
```

### Tail Call Optimization

```lisp
(define sum (lambda (n acc)
    (cond ((eq? n 0) acc)
          (t (sum (- n 1) (+ n acc))))))
(print (sum 10000 0)) ; Ensures TCO works
```

## Getting Started

### Building the Project

Quanta uses CMake for building. To build the project, run:

```sh
mkdir build
cd build
cmake ..
make
```

### Running the Interpreter

After building, you can run the interpreter:

```sh
./build/quanta
```

### Running Tests

Quanta includes a suite of tests using GoogleTest. To run the tests:

```sh
cd build
ctest
```

## License

Quanta is licensed under the MIT License. See the LICENSE file for details.

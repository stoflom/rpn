# rpn

<strong>HP25 type calculator</strong> 

Debugged together by  C Lombard February 1991, October 1994

The idea was to replace my old HP25 (whose batteries are now always flat) with
something just as convenient and with similar keystrokes on the numeric pad.
Also, I want to always see what's in the registers and stack. No mice please,
they make me scream. This uses curses text base display.

Accuracy: >12 digits (sometimes), actually C-double precision types are used.

You can't program the calculator because it would be silly to do so. Rather use
BASIC.

On my linux setup, with a standard IBM enhanced keyboard, the keypad
works fine with Num Lock on. You might have to fix the  get_ch function to do
some character translation.

If you want to hack this to change the command
definitions or add features, or do anything else: be my guest. Just don't ask me
to explain the logic, I don't know anymore.  What I do remember is that no
commands can start with 'e' or _EEX character since these characters signal an
exponent.

If you divide by zero or pull the root of a negative number, your
house will cave in --- serves you right!

For unix/linux: link with '-lm -lncurses'. This means on fedora you need to
first do:

```
sudo dnf install ncurses ncurses-devel
```

then build using:

```
gcc -O rpn.c -o rpn -lm -lncurses 
```
If you have 'make' installed you can just say:

```
make
```
I normally copy the resulting binary to the execution path:
```
sudo cp ./rpn /usr/local/bin
```

For windows you can compile with Visual Studio Community (a project/solution is provided), or use the TinyC compiler.



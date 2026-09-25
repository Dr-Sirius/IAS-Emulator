# IAS-Emulator
This is an emulator for the Von Neumann IAS computer. The entire instruction set has been implemented, though not fully tested.

## Why?
During one of my college courses going over the IAS, I noticed a severe lack of IAS emulators to test my code out on. So I decided to solve the problem myself. Also because it seemed like a fun project idea.

## How do you use it?
### Assembling
The emulator reads `.imm` & `.iim` files which require the [IAS-Assembler](https://github.com/Dr-Sirius/IAS-Assembler)
> IAS memory map & IAS instruction map

The assembler reads `.is` files and outputs the `.imm` & `.iim`

Examples programs can be found [here](https://github.com/Dr-Sirius/IAS-Assembler/tree/main/examples)

### Running A Program
Once the required files have been obtained, a program can be loaded an ran simply by doing
```
./main <source-file>.imm <source-file>.iim
```

Currently the program will output the values stored in the registers at the end of each cycle, as well as the values stored in memory

## Building
The program can be built by calling
```
make
```

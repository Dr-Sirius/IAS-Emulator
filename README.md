# IAS-Emulator
This is an emulator for the Von Neumann IAS computer. The entire instruction set has been implemented, though not fully tested.

## Why?
During one of my college courses going over the IAS, I noticed a severe lack of IAS emulators to test my code out on. So I decided to solve the problem myself. Also because it seemed like a fun project idea.

## How do you use it?
currently programs need to be 'hand' loaded into memory. This includes the loading of any numerical values into memory for something like variables and constants. As well as the instructions themselves. 

For example, here is a simple program to add two variables together and store in a third (c = a + b):
```cpp
MEMORY[0] = 10; // a
MEMORY[1] = 20; // b

// first instr on left block of memory addr 3
INSTRUCTION l3 = {LOAD_M,0};  // loads value at memory address 0 into Accumulator

// second instr on right block of memory addr 3
INSTRUCTION r3 = {ADD_M,1};   // adds value at memory address 1 with Accumulator, stores result in AC

// first instr on left block of memory addr 4
INSTRUCTION l4 = {STOR_M,2};  // Stores value in Accumulator into memory address 2

loadInstrIntoMEM(3,l3,r3); // loads first and second instructions into memory addr 3
loadInstrIntoMEM(4,l4);    // loads first instr and HALT instr into memory addr 4

CU.PC = 3; // specifies location of first instruction, Program Counter
```

This program will put the sum of `a` and `b` and store the result into `c`

Currently the memory is not printed out, however it will be added as an option in the future

## Building
If you've built a correct program, you can simply run 

```
make all
```

or 
```
make && ./main
```

The emulator is written with c++ 23, due to use of std::println

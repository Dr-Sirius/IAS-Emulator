#include <print>
#include <cstdint>
#include <iostream>
#include <stack>

using std::string;
using std::stack;

uint32_t MEMORY[999];

struct CPU_CU {
    uint32_t PC; // program counter
    uint32_t IBR; // Instruction Buffer Register
    uint32_t IR; // Instruction Register
    uint32_t MAR; // Memory Address Buffer
};

struct CPU_ALU {
    uint32_t AC; // Accumulator
    uint32_t MQ; 
    uint32_t MBR; // Memory Buffer Register
};

enum OPCODES: uint8_t {
    LOAD_M     =  0b00000001, // 1 - load M into AC
    LOAD_NEGM  =  0b00000010, // 2 - load -M into AC
    LOAD_ABSM  =  0b00000011, // 3 - load |M| into AC
    LOAD_NABSM =  0b00000100, // 4 - load -|M| into AC
    ADD_M      =  0b00000101, // 5 - Add M to AC, load result into AC
    SUB_M      =  0b00000110, // 6 - Sub M from AC, load result into AC
    ADD_MABS   =  0b00000111, // 7 - Add |M| to AC, load result into AC
    SUB_MABS   =  0b00001000, // 8 - Sub |M| from AC, load result into AC
};


void baseConverter(int n, int base) {
    string symb = "0123456789ABCDEF";
    stack<char> s;
    for (;n/base > 0; n/=base)
        s.push(symb[n%base]);
    s.push(symb[n%base]);
    for (; !s.empty();s.pop())
        std::print("{}",s.top());
    std::println();
}

int main() {

    MEMORY[0] = (SUB_M << 24) | (0b01 << 16) | (ADD_M << 8) | 0b10;
    baseConverter(MEMORY[0],2);

    uint8_t opcode1 = MEMORY[0] >> 24;
    uint8_t opcode2 = MEMORY[0] >> 8;
    switch (opcode2) {
        case ADD_M:
            std::println("CHEESE");   
    }

    return 0;
}


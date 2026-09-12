#include <cstdint>
#include <iostream>
#include <print>
#include <stack>

using std::stack;
using std::string;

int32_t MEMORY[999];
uint cycleCount = 0;

bool shouldHalt = false;
bool right = false;

struct CPU_CU {
  int32_t PC;  // program counter
  int32_t IBR; // Instruction Buffer Register
  int32_t IR;  // Instruction Register
  int32_t MAR; // Memory Address Buffer
};

struct CPU_ALU {
  int32_t AC; // Accumulator
  int32_t MQ;
  int32_t MBR; // Memory Buffer Register
};

enum OPCODES : int8_t {
  HALT = 0b0,              // 0 HALT
  LOAD_M = 0b00000001,     // 1  - load M into AC
  LOAD_NEGM = 0b00000010,  // 2  - load -M into AC
  LOAD_ABSM = 0b00000011,  // 3  - load |M| into AC
  LOAD_NABSM = 0b00000100, // 4  - load -|M| into AC
  ADD_M = 0b00000101,      // 5  - Add M to AC, load result into AC
  SUB_M = 0b00000110,      // 6  - Sub M from AC, load result into AC
  ADD_MABS = 0b00000111,   // 7  - Add |M| to AC, load result into AC
  SUB_MABS = 0b00001000,   // 8  - Sub |M| from AC, load result into AC
  STOR_M = 0b00100001,     // 33 - Store AC at M
};

struct INSTRUCTION {
  OPCODES opcode;
  int8_t operand = 0b0;
};

void baseConverter(int n, int base) {
  string symb = "0123456789ABCDEF";
  stack<char> s;
  for (; n / base > 0; n /= base)
    s.push(symb[n % base]);
  s.push(symb[n % base]);
  for (; !s.empty(); s.pop())
    std::print("{}", s.top());
  std::println();
}

string binaryString(int n) {
  string symb = "01";
  stack<char> s;
  string bin = "";
  for (; n / 2 > 0; n /= 2)
    s.push(symb[n % 2]);
  s.push(symb[n % 2]);
  for (; !s.empty(); s.pop())
    bin += s.top();
  return bin;
}

void loadInstrIntoMEM(int32_t address, INSTRUCTION instr1 = {HALT},
                      INSTRUCTION instr2 = {HALT}) {
  MEMORY[address] = (instr1.opcode << 24) | (instr1.operand << 16) |
                    (instr2.opcode << 8) | instr2.operand;
}

void fetchInstr(CPU_ALU &ALU, CPU_CU &CU) {
  CU.MAR = CU.PC;
  ALU.MBR = MEMORY[CU.MAR];
  int8_t op2 = (ALU.MBR >> 8) & 0b11111111;
  int8_t mem = ALU.MBR & 0b11111111;
  CU.IBR = (op2 << 8) | mem;
  CU.IR = (ALU.MBR >> 24) & 0b11111111;  // Load Left OPCODE into IR
  CU.MAR = (ALU.MBR >> 16) & 0b11111111; // load memory addr into MAR
  std::println("CYCLE |{}|", cycleCount);
  std::println("PC| {} || AC | {} |", binaryString(CU.PC),
               binaryString(ALU.AC));
  std::println("MAR| {} || MBR | {} |", binaryString(CU.MAR),
               binaryString(ALU.MBR));
  std::println("IBR| {} || IR | {} |", binaryString(CU.IBR),
               binaryString(CU.IR));
  std::println();
  ++cycleCount;
}

void instrSetup(CPU_ALU &ALU, CPU_CU &CU) {
  int8_t op2 = (CU.IBR >> 8) & 0b11111111;
  int8_t mem = CU.IBR & 0b11111111;
  CU.IR = op2;
  CU.MAR = mem;

  std::println("CYCLE |{}|", cycleCount);
  std::println("PC| {} || AC | {} |", binaryString(CU.PC),
               binaryString(ALU.AC));
  std::println("MAR| {} || MBR | {} |", binaryString(CU.MAR),
               binaryString(ALU.MBR));
  std::println("IBR| {} || IR | {} |", binaryString(CU.IBR),
               binaryString(CU.IR));
  std::println();
  ++cycleCount;
}

void fetchData(CPU_ALU &ALU, CPU_CU &CU) {
  ALU.MBR = MEMORY[CU.MAR]; // get and load memory into MBR
  std::println("CYCLE |{}|", cycleCount);
  std::println("PC| {} || AC | {} |", binaryString(CU.PC),
               binaryString(ALU.AC));
  std::println("MAR| {} || MBR | {} |", binaryString(CU.MAR),
               binaryString(ALU.MBR));
  std::println("IBR| {} || IR | {} |", binaryString(CU.IBR),
               binaryString(CU.IR));
  std::println();
  ++cycleCount;
}

void execution(CPU_ALU &ALU, CPU_CU &CU) {
  switch (CU.IR) {
  case LOAD_M: {
    ALU.AC = ALU.MBR;
    break;
  }
  case LOAD_NEGM: {
    ALU.AC = -ALU.MBR;
    break;
  }
  case LOAD_ABSM: {
    ALU.AC = abs(ALU.MBR);
    break;
  }
  case LOAD_NABSM: {
    ALU.AC = -abs(ALU.MBR);
    break;
  }
  case ADD_M: {
    ALU.AC = ALU.AC + ALU.MBR;
    break;
  }
  case SUB_M: {
    ALU.AC = ALU.AC - ALU.MBR;
    break;
  }
  case ADD_MABS: {
    ALU.AC = ALU.AC + abs(ALU.MBR);
    break;
  }
  case SUB_MABS: {
    ALU.AC = ALU.AC - abs(ALU.MBR);
    break;
  }
  case STOR_M: {
    ALU.MBR = ALU.AC;
    MEMORY[CU.MAR] = ALU.MBR;
  }
  case HALT: {
    shouldHalt = true;
    break;
  }
  }

  std::println("CYCLE |{}|", cycleCount);
  std::println("PC| {} || AC | {} |", binaryString(CU.PC),
               binaryString(ALU.AC));
  std::println("MAR| {} || MBR | {} |", binaryString(CU.MAR),
               binaryString(ALU.MBR));
  std::println("IBR| {} || IR | {} |", binaryString(CU.IBR),
               binaryString(CU.IR));
  std::println();
  if (right)
    ++CU.PC;
  right = !right;
  ++cycleCount;
}

int main() {

  // MEMORY[0] = (SUB_M << 24) | (0b01 << 16) | (ADD_M << 8) | 0b10;
  // baseConverter(MEMORY[0], 2);
  // int8_t op2 = (MEMORY[0] >> 8) & 0b11111111;
  // int8_t mem = MEMORY[0] & 0b11111111;
  // int32_t test = (op2 << 8) | mem;
  // baseConverter(test, 2);
  CPU_ALU ALU = {0};
  CPU_CU CU = {0};
  MEMORY[100] = 0b1010;
  MEMORY[101] = 0b10100;

  loadInstrIntoMEM(0b0, {LOAD_M, 0b1100100}, {ADD_M, 0b1100101});
  loadInstrIntoMEM(0b1, {STOR_M, 0b1100110});

  CU.PC = 0b0;
  while (!shouldHalt) {
    if (right) {
      instrSetup(ALU, CU);
    } else {
      fetchInstr(ALU, CU);
    }
    fetchData(ALU, CU);
    execution(ALU, CU);
  }
  std::println("MEM 102 {}", MEMORY[102]);

  return 0;
}

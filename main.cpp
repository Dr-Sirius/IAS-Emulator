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
  JUMP_ML = 0b00001101,    // 13 - Unconditional jump to M left instr
  JUMP_MR = 0b00001110,    // 14 - Unconditional jump to M right instr
  JUMP_PML = 0b00001111,   // 15 - If AC >= 0 jump to M left instr
  JUMP_PMR = 0b00010000,   // 16 - If AC >= 0 jump to M right instr
  STOR_ML = 0b00010010,    // 18 - Replace M at left instr with AC
  STOR_MR = 0b00010011,    // 19 - Replace M at right instr with AC

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
  string symb = "012";
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
  std::println("MAR <- PC");
  CU.MAR = CU.PC;
  std::println("MBR <- M[{}]", CU.MAR);
  ALU.MBR = MEMORY[CU.MAR];
  std::println("OP2 <- MBR");
  int8_t op2 = (ALU.MBR >> 8) & 0b11111111;
  std::println("MEM <- MBR");
  int8_t mem = ALU.MBR & 0b11111111;
  std::println("IBR <- 20:39");
  CU.IBR = (op2 << 8) | mem;
  std::println("MBR <- OP");
  CU.IR = (ALU.MBR >> 24) & 0b11111111; // Load Left OPCODE into IR
  std::println("MBR <- M");
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
  ++CU.PC;
}

void fetchData(CPU_ALU &ALU, CPU_CU &CU) {
  ALU.MBR = MEMORY[CU.MAR]; // get and load memory into MBR
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
    break;
  }

  case JUMP_ML: {
    CU.PC = CU.MAR;
    right = false;
    std::println("FETCH INSTR JMPL");
    break;
  }
  case JUMP_MR: {

    CU.PC = CU.MAR;
    right = true;
    std::println("FETCH INSTR JMP");
    fetchInstr(ALU, CU);
    break;
  }
  case JUMP_PML: {
    if (ALU.AC >= 0) {
      CU.PC = CU.MAR;
      right = false;
    }
    break;
  }
  case JUMP_PMR: {
    if (ALU.AC >= 0) {
      CU.PC = CU.MAR;
      right = true;
      std::println("FETCH INSTR +JMP");
      fetchInstr(ALU, CU);
    }
    break;
  }

  case STOR_ML: {
    int8_t op1 = (CU.IBR >> 24) & 0b11111111;
    int8_t op2 = (CU.IBR >> 8) & 0b11111111;
    int8_t mem2 = CU.IBR & 0b11111111;
    MEMORY[CU.MAR] = (op1 << 24) | (ALU.AC << 16) | (op2 << 8) | mem2;
    break;
  }
  case STOR_MR: {
    int8_t op1 = (CU.IBR >> 24) & 0b11111111;
    int8_t mem1 = CU.IBR & 0b11111111;
    int8_t op2 = (CU.IBR >> 8) & 0b11111111;
    MEMORY[CU.MAR] = (op1 << 24) | (mem1 << 16) | (op2 << 8) | ALU.AC;
    MEMORY[CU.MAR] = MEMORY[CU.MAR] | ALU.AC;
    break;
  }

  case HALT: {
    shouldHalt = true;
    break;
  }
  }

  std::println("CYCLE |{}|", cycleCount);
  // std::println("PC| {} || AC | {} |", binaryString(CU.PC),
  //              binaryString(ALU.AC));
  // std::println("MAR| {} || MBR | {} |", binaryString(CU.MAR),
  //              binaryString(ALU.MBR));
  // std::println("IBR| {} || IR | {} |", binaryString(CU.IBR),
  //              binaryString(CU.IR));
  // std::println();
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
  // MEMORY[100] = 0b1010;
  // MEMORY[101] = 0b10100;

  // loadInstrIntoMEM(0b0, {LOAD_M, 0b1100100}, {ADD_M, 0b1100101});
  // loadInstrIntoMEM(0b1, {STOR_M, 0b1100110});

  MEMORY[0b0] = 0b01;     // 1
  MEMORY[0b01] = 0b01010; // 10
  MEMORY[0b10] = 0b00100; // 4
  MEMORY[80] = 1;
  MEMORY[81] = 2;
  MEMORY[82] = 3;
  MEMORY[83] = 4;
  MEMORY[84] = 5;

  MEMORY[70] = -1;
  MEMORY[71] = -2;
  MEMORY[72] = -3;
  MEMORY[73] = -4;
  MEMORY[74] = -5;
  MEMORY[50] = 800;

  INSTRUCTION l3 = {LOAD_M, 84};
  INSTRUCTION r3 = {ADD_MABS, 74};
  INSTRUCTION l4 = {STOR_M, 64};
  INSTRUCTION r4 = {LOAD_M, 2};
  INSTRUCTION l5 = {SUB_M, 0};
  INSTRUCTION r5 = {JUMP_PMR, 6};
  INSTRUCTION l6 = {HALT};
  INSTRUCTION r6 = {LOAD_M, 2};
  INSTRUCTION l7 = {ADD_M, 50};
  INSTRUCTION r7 = {STOR_ML, 3};
  INSTRUCTION l8 = {SUB_M, 1};
  INSTRUCTION r8 = {STOR_MR, 3};
  INSTRUCTION l9 = {SUB_M, 1};
  INSTRUCTION r9 = {STOR_ML, 4};
  INSTRUCTION l10 = {JUMP_ML, 3};

  loadInstrIntoMEM(3, l3, r3);
  loadInstrIntoMEM(4, l4, r4);
  loadInstrIntoMEM(5, l5, r5);
  loadInstrIntoMEM(6, l6, r6);
  loadInstrIntoMEM(7, l7, r7);
  loadInstrIntoMEM(8, l8, r8);
  loadInstrIntoMEM(9, l9, r9);
  loadInstrIntoMEM(10, l10);

  CU.PC = 3;
  while (!shouldHalt) {
    if (right) {
      std::println("INSTR");
      instrSetup(ALU, CU);
      right = false;
    } else {
      std::println("FETCH INSTR");
      fetchInstr(ALU, CU);
      right = true;
    }

    std::println("FETCH DATA");
    fetchData(ALU, CU);
    std::println("EXE");
    execution(ALU, CU);
  }
  std::println("MEM 64 {}", MEMORY[64]);
  std::println("MEM 63 {}", MEMORY[63]);
  std::println("MEM 62 {}", MEMORY[62]);
  std::println("MEM 61 {}", MEMORY[61]);
  std::println("MEM 60 {}", MEMORY[60]);

  return 0;
}

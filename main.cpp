#include <cstdint>
#include <iostream>
#include <print>
#include <stack>

#define DEBUG_FETCH_CYLE false
#define DEBUG_MEMORY false
#define DEBUG_INSTRS false

using std::stack;
using std::string;

int32_t MEMORY[999];
uint cycleCount = 0;

bool shouldHalt = false;
bool right = false;

/**
 * Struct Representing IAS Control Unit
 *
 * #### Members:
 * `PC`  - Program Counter
 * `IBR` - Instruction Buffer Register
 * `IR`  - Instruction Register
 * `MAR` - Memory Address Register
 */
struct CPU_CU {
  int32_t PC;  // program counter
  int32_t IBR; // Instruction Buffer Register
  int32_t IR;  // Instruction Register
  int32_t MAR; // Memory Address Buffer
};

/**
 * Struct Representing IAS Arithmetic Logic Unit
 *
 * #### Members:
 * `AC`  - Accumulator Counter
 * `MQ`  - Multiplier/Quotient
 * `MBR` - Memory Buffer Register
 */
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

/**
 * @brief Converts number into binary string representation
 *
 *
 * @param n Number to be converted to binary string
 * @return binary string representation of n
 *
 * #### Usage:
 * ```cpp
 * std::string bin = binaryString(11); // returns "1011"
 * ```
 *
 **/
string binaryString(int32_t n) {
  // return std::to_string(n);
  n = abs(n);
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

/**
 * @brief Loads given Instructions into specified memory address
 *
 * @param address Memory address to load instructions
 * @param instr1 Instruction to be loaded into left hand side of memory block,
 * Defaults to HALT instruction
 * @param instr2 Instruction to be loaded into right hand side of memory block,
 * Defaults to HALT instruction
 *
 * #### Usage:
 * ```cpp
 * INSTRUCTION leftInstr =  {LOAD_M,0};
 * INSTRUCTION rightInstr = {ADD_M,10};
 * loadInstrIntoMEM(2,leftInstr,rightInstr);
 * ```
 * Or
 * ```cpp
 * INSTRUCTION leftInstr =  {0b00000001,0b0};
 * INSTRUCTION rightInstr = {0b00000101,0b1010};
 * loadInstrIntoMEM(0b10,leftInstr,rightInstr);
 * ```
 *
 **/
void loadInstrIntoMEM(int32_t address, INSTRUCTION instr1 = {HALT},
                      INSTRUCTION instr2 = {HALT}) {
  MEMORY[address] = (instr1.opcode << 24) | (instr1.operand << 16) |
                    (instr2.opcode << 8) | instr2.operand;
}

/**
 * @brief Fetches instructions at current Program Counter Location and loads
 * them into registers
 *
 * @param ALU Reference to Arithmetic Logic Unit
 * @param CU Reference to Control Unit
 *
 * @details
 * 1. Loads Program Counter into Memory Address Register (MAR <- PC)
 * 2. Fetches operand at address stored in MAR and loads it into Memory Buffer
 * Register (MBR <- M[MAR])
 * 3. Places right hand instruction into Instruction Buffer Register (IBR
 * <-MBR(20:39))
 * 4. Places left hand instruction opcode into Instruction Register (IR <-
 * MBR(0:7))
 * 5. Places left hand instruction memory address argument into MAR (MAR
 * <-MBR(8:19))
 **/
void fetchInstr(CPU_ALU &ALU, CPU_CU &CU) {
#if DEBUG_FETCH_CYLE
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
#else
  CU.MAR = CU.PC;
  ALU.MBR = MEMORY[CU.MAR];
  int8_t op2 = (ALU.MBR >> 8) & 0b11111111;
  int8_t mem = ALU.MBR & 0b11111111;
  CU.IBR = (op2 << 8) | mem;
  CU.IR = (ALU.MBR >> 24) & 0b11111111;  // Load Left OPCODE into IR
  CU.MAR = (ALU.MBR >> 16) & 0b11111111; // load memory addr into MAR
#endif
#if DEBUG_MEMORY
  std::println("CYCLE |{}|", cycleCount);
  std::println("PC| {} || AC | {} |", binaryString(CU.PC),
               binaryString(ALU.AC));
  std::println("MAR| {} || MBR | {} |", binaryString(CU.MAR),
               binaryString(ALU.MBR));
  std::println("IBR| {} || IR | {} |", binaryString(CU.IBR),
               binaryString(CU.IR));
  std::println();
#endif
  ++cycleCount;
}

/**
 * @brief Setups instructions held in IBR and loads into IR and MAR
 *
 * @param ALU Reference to Arithmetic Logic Unit
 * @param CU Reference to Control Unit
 *
 * @details
 * 1. Loads OPCODE stored in IBR(20:27) into IR (IR <- IBR(20:27))
 * 2. Loads operand stored in IBR(28:39) into Memory Address Register (MAR <-
 * M[MAR])
 **/
void instrSetup(CPU_ALU &ALU, CPU_CU &CU) {
  int8_t op2 = (CU.IBR >> 8) & 0b11111111;
  int8_t mem = CU.IBR & 0b11111111;
  CU.IR = op2;
  CU.MAR = mem;
#if DEBUG_MEMORY
  std::println("CYCLE |{}|", cycleCount);
  std::println("PC| {} || AC | {} |", binaryString(CU.PC),
               binaryString(ALU.AC));
  std::println("MAR| {} || MBR | {} |", binaryString(CU.MAR),
               binaryString(ALU.MBR));
  std::println("IBR| {} || IR | {} |", binaryString(CU.IBR),
               binaryString(CU.IR));
  std::println();
#endif
  ++cycleCount;
  ++CU.PC;
}

/**
 * @brief Fetches memory at address stored in MAR and loads it into MBR
 *
 * @param ALU Reference to Arithmetic Logic Unit
 * @param CU Reference to Control Unit
 *
 * @details
 * Loads memory at address stored in MAR and loads it into MBR (MBR <- M[MAR])
 **/
void fetchData(CPU_ALU &ALU, CPU_CU &CU) {
  ALU.MBR = MEMORY[CU.MAR]; // get and load memory into MBR
}

/**
 * @brief Executes Instruction currently in IR(OPCODE) and MBR(operand)
 *
 * @param ALU Reference to Arithmetic Logic Unit
 * @param CU Reference to Control Unit
 *
 **/
void execution(CPU_ALU &ALU, CPU_CU &CU) {
  switch (CU.IR) {
  case LOAD_M: {
#if DEBUG_INSTRS
    std::println("LOAD M({})", CU.MAR);
#endif
    ALU.AC = ALU.MBR;
    break;
  }
  case LOAD_NEGM: {
    ALU.AC = -ALU.MBR;
#if DEBUG_INSTRS
    std::println("LOAD -M({})", CU.MAR);
#endif
    break;
  }
  case LOAD_ABSM: {
    ALU.AC = abs(ALU.MBR);
#if DEBUG_INSTRS
    std::println("LOAD |M({})|", CU.MAR);
#endif
    break;
  }
  case LOAD_NABSM: {
    ALU.AC = -abs(ALU.MBR);
#if DEBUG_INSTRS
    std::println("LOAD |M({})|", CU.MAR);
#endif
    break;
  }
  case ADD_M: {
#if DEBUG_INSTRS
    std::println("ADD M({})", CU.MAR);
#endif
    ALU.AC = ALU.AC + ALU.MBR;
    break;
  }
  case SUB_M: {
#if DEBUG_INSTRS
    std::println("SUB M({})", CU.MAR);
#endif
    ALU.AC = ALU.AC - ALU.MBR;
    break;
  }
  case ADD_MABS: {
#if DEBUG_INSTRS
    std::println("ADD |M({})|", CU.MAR);
#endif
    ALU.AC = ALU.AC + abs(ALU.MBR);
    break;
  }
  case SUB_MABS: {
#if DEBUG_INSTRS
    std::println("SUB |M({})|", CU.MAR);
#endif
    ALU.AC = ALU.AC - abs(ALU.MBR);
    break;
  }
  case STOR_M: {
#if DEBUG_INSTRS
    std::println("STOR M({})", CU.MAR);
#endif
    ALU.MBR = ALU.AC;
    MEMORY[CU.MAR] = ALU.MBR;
    break;
  }

  case JUMP_ML: {
#if DEBUG_INSTRS
    std::println("JUMP M({},8:19)", CU.MAR);
#endif
    CU.PC = CU.MAR;
    right = false;

    break;
  }
  case JUMP_MR: {
#if DEBUG_INSTRS
    std::println("JUMP M({},20:39)", CU.MAR);
#endif
    CU.PC = CU.MAR;
    right = true;

    fetchInstr(ALU, CU);
    break;
  }
  case JUMP_PML: {
#if DEBUG_INSTRS
    std::println("JUMP +M({},8:19)", CU.MAR);
#endif
    if (ALU.AC >= 0) {
      CU.PC = CU.MAR;
      right = false;
    }
    break;
  }
  case JUMP_PMR: {
#if DEBUG_INSTRS
    std::println("JUMP +M({},20:39)", CU.MAR);
#endif
    if (ALU.AC >= 0) {
      CU.PC = CU.MAR;
      right = true;
      fetchInstr(ALU, CU);
    }
    break;
  }

  case STOR_ML: {
#if DEBUG_INSTRS
    std::println("STOR M({},8:19)", CU.MAR);
#endif
    int32_t addr = MEMORY[CU.MAR];
    int8_t op1 = (addr >> 24) & 0b11111111;
    int8_t op2 = (addr >> 8) & 0b11111111;
    int8_t mem2 = addr & 0b11111111;
    MEMORY[CU.MAR] = (op1 << 24) | (ALU.AC << 16) | (op2 << 8) | mem2;
    break;
  }
  case STOR_MR: {
#if DEBUG_INSTRS
    std::println("STOR M({},20:39)", CU.MAR);
#endif
    int32_t addr = MEMORY[CU.MAR];
    int8_t op1 = (addr >> 24) & 0b11111111;
    int8_t mem1 = (addr >> 16) & 0b11111111;
    int8_t op2 = (addr >> 8) & 0b11111111;
    MEMORY[CU.MAR] = (op1 << 24) | (mem1 << 16) | (op2 << 8) | ALU.AC;
    break;
  }

  case HALT: {
#if DEBUG_INSTRS
    std::println("HALT");
#endif
    shouldHalt = true;
    break;
  }
  }

#if DEBUG_MEMORY
  std::println("CYCLE |{}|", cycleCount);
  std::println("PC| {} || AC | {} |", binaryString(CU.PC),
               binaryString(ALU.AC));
  std::println("MAR| {} || MBR | {} |", binaryString(CU.MAR),
               binaryString(ALU.MBR));
  std::println("IBR| {} || IR | {} |", binaryString(CU.IBR),
               binaryString(CU.IR));
  std::println();
#endif
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

  MEMORY[50] = 80;

  INSTRUCTION l3 = {LOAD_M, 84};
  INSTRUCTION r3 = {ADD_MABS, 74};
  INSTRUCTION l4 = {STOR_M, 64};
  INSTRUCTION r4 = {LOAD_M, 2};
  INSTRUCTION l5 = {SUB_M, 0};
  INSTRUCTION r5 = {JUMP_PMR, 6};
  INSTRUCTION l6 = {HALT};
  INSTRUCTION r6 = {STOR_M, 2};
  INSTRUCTION l7 = {LOAD_M, 2};
  INSTRUCTION r7 = {ADD_M, 50};
  INSTRUCTION l8 = {STOR_ML, 3};
  INSTRUCTION r8 = {SUB_M, 1};
  INSTRUCTION l9 = {STOR_MR, 3};
  INSTRUCTION r9 = {SUB_M, 1};
  INSTRUCTION l10 = {STOR_ML, 4};
  INSTRUCTION r10 = {JUMP_ML, 3};

  loadInstrIntoMEM(3, l3, r3);
  loadInstrIntoMEM(4, l4, r4);
  loadInstrIntoMEM(5, l5, r5);
  loadInstrIntoMEM(6, l6, r6);
  loadInstrIntoMEM(7, l7, r7);
  loadInstrIntoMEM(8, l8, r8);
  loadInstrIntoMEM(9, l9, r9);
  loadInstrIntoMEM(10, l10, r10);

  CU.PC = 3;
  while (!shouldHalt) {
    if (right) {

      instrSetup(ALU, CU);
      right = false;
    } else {

      fetchInstr(ALU, CU);
      right = true;
    }

    fetchData(ALU, CU);

    execution(ALU, CU);
  }
  std::println("MEM 64 {}", MEMORY[64]);
  std::println("MEM 63 {}", MEMORY[63]);
  std::println("MEM 62 {}", MEMORY[62]);
  std::println("MEM 61 {}", MEMORY[61]);
  std::println("MEM 60 {}", MEMORY[60]);

  return 0;
}

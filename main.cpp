#include <bitset>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <print>
#include <vector>

#define DEBUG_FETCH_CYCLE false
#define DEBUG_MEMORY true
#define DEBUG_INSTRS true
#define DEBUG_MEM_DUMP true
#define DEBUG_MEM_DUMP_BIN false
#define MEMMAX 999

using std::string;

struct int40_t {
private:
  int64_t val;

  int64_t normalize(int64_t v) { return (v << 24) >> 24; }

public:
  int40_t(int64_t num) : val(normalize(num)) {}
  int40_t() : val(0) {}

  int40_t operator+(const int40_t &rhs) { return int40_t(val + rhs.val); }
  int40_t operator-(const int40_t &rhs) { return int40_t(val - rhs.val); }
  int40_t operator*(const int40_t &rhs) { return int40_t(val * rhs.val); }
  int40_t operator/(const int40_t &rhs) { return int40_t(val / rhs.val); }
  int40_t operator%(const int40_t &rhs) { return int40_t(val % rhs.val); }
  int40_t operator++() {
    *this = int40_t(val + 1);
    return *this;
  }
  int40_t operator--() {
    *this = int40_t(val - 1);
    return *this;
  }

  int40_t &operator+=(const int40_t &rhs) {
    val += rhs.val;
    val = normalize(val);
    return *this;
  }

  int40_t &operator-=(const int40_t &rhs) {
    val -= rhs.val;
    val = normalize(val);
    return *this;
  }

  int40_t operator-() { return int40_t(-val); }

  auto operator<=>(const int40_t &rhs) const { return val <=> rhs.val; }
  bool operator==(const int40_t &rhs) const { return val == rhs.val; }

  int40_t operator&(const int40_t &rhs) { return int40_t(val & rhs.val); }
  int40_t operator|(const int40_t &rhs) { return int40_t(val | rhs.val); }
  int40_t operator<<(const int40_t &rhs) { return int40_t(val << rhs.val); }
  int40_t operator>>(const int40_t &rhs) { return int40_t(val >> rhs.val); }

  int64_t getVal() { return val; }
};

int40_t MEMORY[MEMMAX];
uint cycleCount = 0;

bool shouldHalt = false;
bool right = false;
bool fileNotExist = false;

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
  int40_t PC;  // program counter
  int40_t IBR; // Instruction Buffer Register
  int40_t IR;  // Instruction Register
  int40_t MAR; // Memory Address Buffer
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
  int40_t AC; // Accumulator
  int40_t MQ;
  int40_t MBR; // Memory Buffer Register
};

enum OPCODES : int8_t {
  HALT = 0b0, // 0  - HALT
  // LOAD AC INSTRS
  LOAD_M = 0b00000001,     // 1  - load M into AC
  LOAD_NEGM = 0b00000010,  // 2  - load -M into AC
  LOAD_ABSM = 0b00000011,  // 3  - load |M| into AC
  LOAD_NABSM = 0b00000100, // 4  - load -|M| into AC

  // ADD/SUB INSTRS
  ADD_M = 0b00000101,    // 5  - Add M to AC, load result into AC
  SUB_M = 0b00000110,    // 6  - Sub M from AC, load result into AC
  ADD_MABS = 0b00000111, // 7  - Add |M| to AC, load result into AC
  SUB_MABS = 0b00001000, // 8  - Sub |M| from AC, load result into AC

  // LOAD MQ INSTRS
  LOAD_MQM = 0b00001001, // 9  - load M into MQ
  LOAD_MQ = 0b00001010,  // 10 - load MQ into AC

  // Mul/Div INSTRS
  MUL_M = 0b00001011, // 11 - Multiply MQ by M, Load upper 32 bits into AC,
                      // lower into MQ
  DIV_M = 0b00001100, // 12 - Divide MQ by M, load quotient into MQ, Remainder
                      // into AC

  // JUMP INSTRS
  JUMP_ML = 0b00001101,  // 13 - Unconditional jump to M left instr
  JUMP_MR = 0b00001110,  // 14 - Unconditional jump to M right instr
  JUMP_PML = 0b00001111, // 15 - If AC >= 0 jump to M left instr
  JUMP_PMR = 0b00010000, // 16 - If AC >= 0 jump to M right instr

  // Address Modify INSTRS
  STOR_ML = 0b00010010, // 18 - Replace M at left instr with AC
  STOR_MR = 0b00010011, // 19 - Replace M at right instr with AC

  LSH = 0b00010100, // 20 - Left Hand Shift, Multiply AC by 2
  RSH = 0b00010101, // 21 - Right Hand Shift, Divide AC by 2

  // Stor INSTR
  STOR_M = 0b00100001, // 33 - Store AC at M
};

struct INSTRUCTION {
  OPCODES opcode;
  int40_t operand = 0;
};

/**
 * @brief Converts number into binary string representation
 *
 *
 * @param n Number to be converted to binary string
 * @return 32-bit binary string representation of n
 *
 * #### Usage:
 * ```cpp
 * std::string bin = binString(11); // returns "0000 0000 0000 1011"
 * ```
 **/
string binString(int40_t n) {

  std::bitset<40> num(n.getVal());
  string bin = num.to_string();
  string nBin = "";
  unsigned short c = 0;

  for (auto it = bin.begin(); it != bin.end(); ++it) {
    if (c == 4) {
      nBin += " ";
      c = 0;
    }

    nBin += *it;

    ++c;
  }

  return nBin;
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
void loadInstrIntoMEM(int40_t address, INSTRUCTION instr1 = {HALT},
                      INSTRUCTION instr2 = {HALT}) {
  MEMORY[address.getVal()] =
      ((int64_t)instr1.opcode << 32) | (instr1.operand << 20).getVal() |
      ((int64_t)instr2.opcode << 12) | instr2.operand.getVal();
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
  std::println("HERE AT FETCH");
#if DEBUG_FETCH_CYCLE
  std::println("MAR <- PC");

  CU.MAR = CU.PC;
  std::println("MBR <- M[{}]", CU.MAR);
  ALU.MBR = MEMORY[CU.MAR];
  std::println("IBR <- INSTR2") CU.IBR = ALU.MBR & 0xfffff;
  std::println("MBR <- OP");
  CU.IR = (ALU.MBR >> 32) & 0xffffffff; // Load Left OPCODE into IR
  std::println("MBR <- M");
  CU.MAR = (ALU.MBR >> 20) & 0xfff; // load memory addr into MAR
#else
  CU.MAR = CU.PC;
  ALU.MBR = MEMORY[CU.MAR.getVal()];
  CU.IBR = ALU.MBR & 0xfffff;
  CU.IR = (ALU.MBR >> 32) & 0xffffffff; // Load Left OPCODE into IR
  CU.MAR = (ALU.MBR >> 20) & 0xfff;     // load memory addr into MAR
#endif
#if DEBUG_MEMORY
  std::println("{:-^100}", "FETCH CYCLE");
  std::println("CYCLE: {:>6}\nCU->\n PC: {:>44}\n MAR: {:>43}\n IBR: {:>43}\n "
               "IR: {:>44}",
               cycleCount, binString(CU.PC), binString(CU.MAR),
               binString(CU.IBR), binString(CU.IR));

  std::println("ALU->\n AC: {:>44}\n MQ: {:>44}\n MBR: {:>43}\n",
               binString(ALU.AC), binString(ALU.MQ), binString(ALU.MBR));

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
  int40_t op2 = (CU.IBR >> 12) & 0xfff;
  int40_t mem = CU.IBR & 0xfff;
  CU.IR = op2;
  CU.MAR = mem;
#if DEBUG_MEMORY
  std::println("{:-^100}", "FETCH CYCLE");
  std::println("CYCLE: {:>6}\nCU->\n PC: {:>44}\n MAR: {:>43}\n IBR: {:>43}\n "
               "IR: {:>44}",
               cycleCount, binString(CU.PC), binString(CU.MAR),
               binString(CU.IBR), binString(CU.IR));

  std::println("ALU->\n AC: {:>44}\n MQ: {:>44}\n MBR: {:>43}\n",
               binString(ALU.AC), binString(ALU.MQ), binString(ALU.MBR));

#endif
  ++cycleCount;
  std::println("PCINCR");
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
  ALU.MBR = MEMORY[CU.MAR.getVal()]; // get and load memory into MBR
}

/**
 * @brief Executes Instruction currently in IR(OPCODE) and MBR(operand)
 *
 * @param ALU Reference to Arithmetic Logic Unit
 * @param CU Reference to Control Unit
 *
 **/
void execution(CPU_ALU &ALU, CPU_CU &CU) {
  string cycleExec;
  switch (CU.IR.getVal()) {
  case LOAD_M: {
#if DEBUG_INSTRS
    cycleExec = std::format("LOAD M({})", CU.MAR.getVal());
#endif
    ALU.AC = ALU.MBR;
    break;
  }
  case LOAD_NEGM: {
    ALU.AC = -ALU.MBR;
#if DEBUG_INSTRS
    cycleExec = std::format("LOAD -M({})", CU.MAR.getVal());
#endif
    break;
  }
  case LOAD_ABSM: {
    ALU.AC = llabs(ALU.MBR.getVal());
#if DEBUG_INSTRS
    cycleExec = std::format("LOAD |M({})|", CU.MAR.getVal());
#endif
    break;
  }
  case LOAD_NABSM: {
    ALU.AC = -llabs(ALU.MBR.getVal());
#if DEBUG_INSTRS
    cycleExec = std::format("LOAD |M({})|", CU.MAR.getVal());
#endif
    break;
  }
  case ADD_M: {
#if DEBUG_INSTRS
    cycleExec = std::format("ADD M({})", CU.MAR.getVal());
#endif
    ALU.AC = ALU.AC + ALU.MBR;
    break;
  }
  case SUB_M: {
#if DEBUG_INSTRS
    cycleExec = std::format("SUB M({})", CU.MAR.getVal());
#endif
    ALU.AC = ALU.AC - ALU.MBR;
    break;
  }
  case ADD_MABS: {
#if DEBUG_INSTRS
    cycleExec = std::format("ADD |M({})|", CU.MAR.getVal());
#endif
    ALU.AC = ALU.AC + llabs(ALU.MBR.getVal());
    break;
  }
  case SUB_MABS: {
#if DEBUG_INSTRS
    cycleExec = std::format("SUB |M({})|", CU.MAR.getVal());
#endif
    ALU.AC = ALU.AC - llabs(ALU.MBR.getVal());
    break;
  }

  case LOAD_MQM: {
#if DEBUG_INSTRS
    cycleExec = std::format("LOAD MQ,M({})", CU.MAR.getVal());
#endif
    ALU.MQ = ALU.MBR;
    break;
  }

  case LOAD_MQ: {
#if DEBUG_INSTRS
    cycleExec = std::format("LOAD MQ", CU.MAR.getVal());
#endif
    ALU.AC = ALU.MQ;
    break;
  }

  case MUL_M: {
#if DEBUG_INSTRS
    cycleExec = std::format("MUL M({})", CU.MAR.getVal());
#endif
    __int128 product = static_cast<__int128>(ALU.MQ.getVal()) *
                       static_cast<__int128>(ALU.MBR.getVal());

    ALU.AC = int40_t(static_cast<int64_t>(product >> 64));

    ALU.MQ = int40_t(static_cast<int64_t>(product & 0xFFFFFFFFFFFFFFFF));

    break;
  }

  case DIV_M: {
#if DEBUG_INSTRS
    cycleExec = std::format("MUL M({})", CU.MAR.getVal());
#endif
    ALU.MQ = ALU.MQ / ALU.MBR;
    ALU.AC = ALU.MQ % ALU.MBR;
    break;
  }

  case JUMP_ML: {
#if DEBUG_INSTRS
    cycleExec = std::format("JUMP M({},8:19)", CU.MAR.getVal());
#endif
    CU.PC = CU.MAR;
    right = false;

    break;
  }
  case JUMP_MR: {
#if DEBUG_INSTRS
    cycleExec = std::format("JUMP M({},20:39)", CU.MAR.getVal());
#endif
    CU.PC = CU.MAR;
    right = true;

    fetchInstr(ALU, CU);
    break;
  }

  case JUMP_PML: {
#if DEBUG_INSTRS
    cycleExec = std::format("JUMP +M({},8:19)", CU.MAR.getVal());
#endif
    if (ALU.AC >= 0) {
      CU.PC = CU.MAR;
      right = false;
    }
    break;
  }
  case JUMP_PMR: {
#if DEBUG_INSTRS
    cycleExec = std::format("JUMP +M({},20:39)", CU.MAR.getVal());
#endif
    if (ALU.AC >= 0) {
      CU.PC = CU.MAR;
      right = true;
      fetchInstr(ALU, CU);
      std::println("HERE AT JUMP AF FETCH");
    }
    break;
  }

  case STOR_ML: {
#if DEBUG_INSTRS
    cycleExec = std::format("STOR M({},8:19)", CU.MAR.getVal());
#endif
    int40_t addr = MEMORY[CU.MAR.getVal()];
    int40_t op1 = addr >> 32;
    int40_t instr2 = addr & 0xfffff;
    MEMORY[CU.MAR.getVal()] = (op1 << 32) | (ALU.AC << 20) | instr2;
    break;
  }
  case STOR_MR: {
#if DEBUG_INSTRS
    cycleExec = std::format("STOR M({},20:39)", CU.MAR.getVal());
#endif
    int40_t addr = MEMORY[CU.MAR.getVal()];
    int40_t instr1 = addr >> 20;
    int40_t op2 = (addr >> 12) & 0xfff;
    MEMORY[CU.MAR.getVal()] = (instr1 << 20) | (op2 << 12) | ALU.AC;
    break;
  }

  case LSH: {
#if DEBUG_INSTRS
    cycleExec = std::format("LSH");
#endif
    ALU.AC = ALU.AC << 1;
    break;
  }

  case RSH: {
#if DEBUG_INSTRS
    cycleExec = std::format("RSH");
#endif
    ALU.AC = ALU.AC >> 1;
    break;
  }

  case STOR_M: {
#if DEBUG_INSTRS
    cycleExec = std::format("STOR M({})", CU.MAR.getVal());
#endif
    ALU.MBR = ALU.AC;
    MEMORY[CU.MAR.getVal()] = ALU.MBR;
    break;
  }

  case HALT: {
#if DEBUG_INSTRS
    cycleExec = std::format("HALT");
#endif
    shouldHalt = true;
    break;
  }
  }

#if DEBUG_MEMORY
  std::println("{:-^100}", "EXECUTE CYCLE");
  std::println("INSTR: {:>7}\nCYCLE: {:>6}\nCU->\n PC: {:>44}\n MAR: {:>43}\n "
               "IBR: {:>43}\n IR: {:>44}",
               cycleExec, cycleCount, binString(CU.PC), binString(CU.MAR),
               binString(CU.IBR), binString(CU.IR));

  std::println("ALU->\n AC: {:>44}\n MQ: {:>44}\n MBR: {:>43}\n",
               binString(ALU.AC), binString(ALU.MQ), binString(ALU.MBR));

#endif
  ++cycleCount;
}

std::vector<int64_t> readFile(const char *filename) {
  std::ifstream file(filename, std::ios::binary);

  const auto fileSize = std::filesystem::file_size(filename);

  std::vector<int64_t> fileData(fileSize / sizeof(uint64_t));
  file.read((char *)&fileData[0], fileSize);
  return fileData;
}

int40_t loadFileDataIntoMem(std::vector<int64_t> data, bool getPC = false) {

  int40_t PC;
  for (size_t i = 0; i < data.size(); ++i) {

    int40_t addr = (data[i] >> 40);
    int40_t mem = int40_t(data[i]);

    MEMORY[addr.getVal()] = mem;
    if (getPC && i == 0) {
      PC = addr;
    }
    if (data[i] == 0 && i != 0)
      break;
  }
  return PC;
}

// void program(CPU_ALU &ALU, CPU_CU &CU) {

// MEMORY[0b0] = 0b01;     // 1
// MEMORY[0b01] = 0b01010; // 10
// MEMORY[0b10] = 0b00100; // 4
// MEMORY[80] = 1;
// MEMORY[81] = 2;
// MEMORY[82] = 3;
// MEMORY[83] = 4;
// MEMORY[84] = 5;

// MEMORY[70] = -1;
// MEMORY[71] = -2;
// MEMORY[72] = -3;
// MEMORY[73] = -4;
// MEMORY[74] = -5;

// MEMORY[50] = 80;

// INSTRUCTION l3 = {LOAD_M, 84};
// INSTRUCTION r3 = {ADD_MABS, 74};
// INSTRUCTION l4 = {STOR_M, 64};
// INSTRUCTION r4 = {LOAD_M, 2};
// INSTRUCTION l5 = {SUB_M, 0};
// INSTRUCTION r5 = {JUMP_PMR, 6};
// INSTRUCTION l6 = {HALT};
// INSTRUCTION r6 = {STOR_M, 2};
// INSTRUCTION l7 = {LOAD_M, 2};
// INSTRUCTION r7 = {ADD_M, 50};
// INSTRUCTION l8 = {STOR_ML, 3};
// INSTRUCTION r8 = {SUB_M, 1};
// INSTRUCTION l9 = {STOR_MR, 3};
// INSTRUCTION r9 = {SUB_M, 1};
// INSTRUCTION l10 = {STOR_ML, 4};
// INSTRUCTION r10 = {JUMP_ML, 3};

// loadInstrIntoMEM(3, l3, r3);
// loadInstrIntoMEM(4, l4, r4);
// loadInstrIntoMEM(5, l5, r5);
// loadInstrIntoMEM(6, l6, r6);
// loadInstrIntoMEM(7, l7, r7);
// loadInstrIntoMEM(8, l8, r8);
// loadInstrIntoMEM(9, l9, r9);
// loadInstrIntoMEM(10, l10, r10);

// std::println("INSTR6: {}", binString(MEMORY[6]));

// CU.PC = 3;
// }

int main(int argc, char *argv[]) {

  if (argc == 1) {
    std::println("ERROR: no input files");
    std::println("Expected: ias <IAS memory map>.imm <IAS instr map>.iim");
    return 1;
  }

  if (argc == 2) {
    std::println("ERROR: missing input file");
    std::println("Expected: ias <IAS memory map>.imm <IAS instr map>.iim");
    return 1;
  }

  if (argc > 3) {
    std::println("ERROR: too many input files");
    std::println("Expected: ias <IAS memory map>.imm <IAS instr map>.iim");
    return 1;
  }

  CPU_ALU ALU = {0};
  CPU_CU CU = {0};

  if (!std::filesystem::exists(argv[1])) {
    std::println("file \"{}\" does not exist", argv[1]);
    fileNotExist = true;
  }
  if (!std::filesystem::exists(argv[2])) {
    std::println("file \"{}\" does not exist", argv[2]);
    fileNotExist = true;
  }

  if (fileNotExist) {
    std::println("Emulation stopped ERROR: file(s) do not exist");
    return 1;
  }

  loadFileDataIntoMem(readFile(argv[1]));
  CU.PC = loadFileDataIntoMem(readFile(argv[2]), true);

  // program(ALU, CU);

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

#if DEBUG_MEM_DUMP

  std::println("{:-^100}", "MEMORY DUMP ");
  for (size_t i = 0; i < MEMMAX; ++i) {
    if (MEMORY[i] == 0 && i != 0)
      continue;
#if DEBUG_MEM_DUMP_BIN
    std::println("ADDR {} - {}", binString(i), binString(MEMORY[i]));
#else
    std::println("ADDR {}:\n INT VAL -> {}\n BIN VAL -> {}\n", i,
                 MEMORY[i].getVal(), binString(MEMORY[i]));

#endif
  }
#endif

  return 0;
}

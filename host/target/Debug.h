/**
 *  Helper interface to debug functions of target
 *
 *  All rights reserved.
 *  Tiny Labs Inc
 *  2022
 */
#include "Target.h"

#ifndef DEBUG_H
#define DEBUG_H

// Return error codes
#define SUCCESS       0
#define ERR_UNKNOWN   1
#define ERR_TIMEOUT   2
#define ERR_PARAMS    3
#define ERR_NOHALT    4
#define ERR_NOMEM     5

typedef enum {
  REG_R0   = 0,
  REG_R1   = 1,
  REG_R2   = 2,
  REG_R3   = 3,
  REG_R4   = 4,
  REG_R5   = 5,
  REG_R6   = 6,
  REG_R7   = 7,
  REG_R8   = 8,
  REG_R9   = 9,
  REG_R10  = 10,
  REG_R11  = 11,
  REG_R12  = 12,
  REG_SP   = 13,
  REG_LR   = 14,
  REG_PC   = 15,
  REG_xPSR = 16,
  REG_MSP  = 17,
  REG_PSP  = 18,
  REG_CTRL = 20, /* bit 24 */
  REG_FMSK = 20, /* bit 16 */
  REG_BPRI = 20, /* bit 8 */
  REG_PMSK = 20, /* bit 0 */
  REG_FPSR = 33, /* Floating point unit */
  REG_S0   = 0x40,
  REG_S1   = 0x41,
  REG_S2   = 0x42,
  REG_S3   = 0x43,
  REG_S4   = 0x44,
  REG_S5   = 0x45,
  REG_S6   = 0x46,
  REG_S7   = 0x47,
  REG_S8   = 0x48,
  REG_S9   = 0x49,
  REG_S10  = 0x4A,
  REG_S11  = 0x4B,
  REG_S12  = 0x4C,
  REG_S13  = 0x4D,
  REG_S14  = 0x4E,
  REG_S15  = 0x4F,
  REG_S16  = 0x50,
  REG_S17  = 0x51,
  REG_S18  = 0x52,
  REG_S19  = 0x53,
  REG_S20  = 0x54,
  REG_S21  = 0x55,
  REG_S22  = 0x56,
  REG_S23  = 0x57,
  REG_S24  = 0x58,
  REG_S25  = 0x59,
  REG_S26  = 0x5A,
  REG_S27  = 0x5B,
  REG_S28  = 0x5C,
  REG_S29  = 0x5D,
  REG_S30  = 0x5E,
  REG_S31  = 0x5F
} reg_t;

class Debug {
 private:
  Target *target;
  int timeout = 20;
  
 public:
  Debug (Target *t);
  virtual ~Debug ();

  // Set timeout retries
  void SetTimeout (int timeout) {this->timeout = timeout;}
  
  // Run control
  int Halt (bool do_reset);
  int Run (void);
  int Step (void);
  
  // Access core registers
  int RegRead (reg_t reg, uint32_t *val);
  int RegWrite (reg_t reg, uint32_t val);

  // Load binary into RAM
  int LoadBin (uint32_t addr, const char *filename);
};

#endif /* DEBUG_H */

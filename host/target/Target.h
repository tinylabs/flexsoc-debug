/**
 *  Target wrapper to access specific CSRs from com link. This is a very thin
 *  layer to wrap the autogenerated csr classes + native comm layer.
 *
 *  All rights reserved.
 *  Tiny Labs Inc
 *  2020
 */
#ifndef TARGET_H
#define TARGET_H

#include <stdint.h>
#include <stdlib.h>

#include "flexdbg_csr.h"


// ADIv5 status
typedef enum {
              ADIv5_FAULT     = 1,
              ADIv5_TIMEOUT   = 2,
              ADIv5_OK        = 4,
              ADIv5_NOCONNECT = 7
              
} adiv5_stat_t;

// Modes of operation
typedef enum {
              MODE_SWD        = 0,
              MODE_JTAG       = 1
} debug_mode_t;

class Target {

  static Target *inst;

private:
  flexdbg_csr *csr;
  Target (char *id);
  
 public:

  // Get singleton instance
  static Target *Ptr (void);
  static Target *Ptr (char *id);

  // Destructor
  virtual ~Target ();

  // Validate CRC32 of interface
  bool Validate (void);
  
  // General APIs
  void ReadW (uint32_t addr, uint32_t *data, uint32_t cnt);
  void ReadH (uint32_t addr, uint16_t *data, uint32_t cnt);
  void ReadB (uint32_t addr, uint8_t *data, uint32_t cnt);
  void WriteW (uint32_t addr, const uint32_t *data, uint32_t cnt);
  void WriteH (uint32_t addr, const uint16_t *data, uint32_t cnt);
  void WriteB (uint32_t addr, const uint8_t *data, uint32_t cnt);
  uint32_t ReadReg (uint32_t addr);
  void WriteReg (uint32_t addr, uint32_t val);

  // Switch modes
  void Mode (debug_mode_t mode);

  // Set PHY clkdiv
  void ClkDiv (uint8_t div);
  
  // Read/Write ADIv5
  adiv5_stat_t WriteDP (uint8_t addr, uint32_t data);
  adiv5_stat_t ReadDP (uint8_t addr, uint32_t *data);
  adiv5_stat_t WriteAP (uint8_t ap, uint8_t addr, uint32_t data);
  adiv5_stat_t ReadAP (uint8_t ap, uint8_t addr, uint32_t *data);
  void Reset (bool pswitch);
  const char *ADIv5_Stat (adiv5_stat_t code);
  
  // Access CSRs
  uint32_t FlexsocID (void);
};

#endif /* TARGET_H */



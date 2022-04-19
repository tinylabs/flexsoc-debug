/**
 *  Target wrapper to access specific CSRs from com link
 *
 *  All rights reserved.
 *  Tiny Labs Inc
 *  2020
 */
#include <unistd.h>
#include <cassert>

#include "Target.h"
#include "flexsoc.h"
#include "hwreg.h"
#include "log.h"

// Singleton pointer
Target *Target::inst = NULL;

// Single callback instance
static irq_handler cb = NULL;
static void flexsoc_irq_convert (uint8_t *buf, int len)
{
  // Assert we have a valid callback
  assert (cb != NULL);
  
  // Make sure length is 2
  assert (len == 2);

  // Pass IRQ to callback
  cb (buf[1]);
}

Target::Target (char *id)
{
  int rv;
  rv = flexsoc_open (id);
  if (rv)
    log (LOG_FATAL, "Failed to open: %s", id);

  // Create autogen CSR classes
  csr = new flexdbg_csr (CSR_BASE, &flexsoc_reg_read, &flexsoc_reg_write);
  if (!csr)
    log (LOG_FATAL, "Failed to inst flexsoc_csr");
}

Target::~Target ()
{
  // Delete CSR classes
  delete csr;
    
  // Close comm link
  flexsoc_close ();
}

Target *Target::Ptr (char *id)
{
  if (!inst) 
    inst =  new Target (id);
  return inst;
}

Target *Target::Ptr (void)
{
  return inst;
}

// Validate CRC to ensure CSRs in sync
bool Target::Validate (void)
{
  return CRC32 != csr->crc32 ();
}

// General APIs
void Target::ReadW (uint32_t addr, uint32_t *data, uint32_t cnt)
{
  if (flexsoc_readw (addr, data, cnt))
    log (LOG_FATAL, "flexsoc_readw failed!");
}

void Target::ReadH (uint32_t addr, uint16_t *data, uint32_t cnt)
{
  if (flexsoc_readh (addr, data, cnt))
    log (LOG_FATAL, "flexsoc_readh failed!");
}

void Target::ReadB (uint32_t addr, uint8_t *data, uint32_t cnt)
{
  if (flexsoc_readb (addr, data, cnt))
    log (LOG_FATAL, "flexsoc_readb failed!");
}

void Target::WriteW (uint32_t addr, const uint32_t *data, uint32_t cnt)
{
  if (flexsoc_writew (addr, data, cnt))
    log (LOG_FATAL, "flexsoc_writew failed!");
}

void Target::WriteH (uint32_t addr, const uint16_t *data, uint32_t cnt)
{
  if (flexsoc_writeh (addr, data, cnt))
    log (LOG_FATAL, "flexsoc_writeh failed!");
}

void Target::WriteB (uint32_t addr, const uint8_t *data, uint32_t cnt)
{
  if (flexsoc_writeb (addr, data, cnt))
    log (LOG_FATAL, "flexsoc_writeb failed!");
}

uint32_t Target::ReadReg (uint32_t addr)
{
  return flexsoc_reg_read (addr);
}

void Target::WriteReg (uint32_t addr, uint32_t val)
{
  flexsoc_reg_write (addr, val);
}


// Access CSRs
uint32_t Target::FlexsocID (void)
{
  return csr->flexsoc_id ();
}

uint32_t Target::Reset (bool pswitch)
{
  uint32_t val;
  
  // DP[0xc] is used as a pseudo register to allow
  // protocol specific RESET/PROTOCOL SWITCHING
  WriteDP (0xc, 0);
  if (pswitch)
    WriteDP (0xc, 1);

  // Must read IDR on reset
  this->ReadDP (0, &val);
  return val;
}

adiv5_stat_t Target::WriteDP (uint8_t addr, uint32_t data)
{
  uint32_t stat;
  
  // Write data
  csr->adiv5_data (data);

  // Write command
  csr->adiv5_cmd (addr & 0xc);

  // No response for RESET
  if (addr != 0xc) {

    // Wait until command completes
    do {
      stat = csr->adiv5_status ();
    } while ((stat & 2) == 0);

    return (adiv5_stat_t)(stat >> 2);
  }

  // For RESET return success
  return ADIv5_OK;

}

adiv5_stat_t Target::ReadDP (uint8_t addr, uint32_t *data)
{
  uint32_t stat;

  // Read command
  csr->adiv5_cmd ((addr & 0xc) | 1);

  // Wait until command completes
  do {
    stat = csr->adiv5_status ();
  } while ((stat & 2) == 0);

  // Check results
  if ((adiv5_stat_t)(stat >> 2) != ADIv5_OK)
    return (adiv5_stat_t)(stat >> 2);
  
  // Read data
  *data = csr->adiv5_data ();

  // Return success
  return ADIv5_OK;
}

adiv5_stat_t Target::WriteAP (uint8_t ap, uint8_t addr, uint32_t data)
{
  uint32_t stat;
  adiv5_stat_t rv;
  
  // Write DP[select] - apbank
  rv = WriteDP (8, (ap << 24) | (addr & 0xF0));
  if (rv != ADIv5_OK)
    return rv;

  // Write data
  csr->adiv5_data (data);
  
  // Wrte AP
  csr->adiv5_cmd ((addr & 0xc) | 2);

  // Wait until command completes
  do {
    stat = csr->adiv5_status ();
  } while ((stat & 2) == 0);

  // Return results
  return (adiv5_stat_t)(stat >> 2);
}

adiv5_stat_t Target::ReadAP (uint8_t ap, uint8_t addr, uint32_t *data)
{
  uint32_t stat;
  adiv5_stat_t rv;
  
  // Write DP[select] - apbank
  rv = WriteDP (8, (ap << 24) | (addr & 0xF0));
  if (rv != ADIv5_OK)
    return rv;

  // Wrte AP
  csr->adiv5_cmd ((addr & 0xc) | 3);

  // Wait until command completes
  do {
    stat = csr->adiv5_status ();
  } while ((stat & 2) == 0);

  // Check results
  if ((adiv5_stat_t)(stat >> 2) != ADIv5_OK)
    return (adiv5_stat_t)(stat >> 2);
  
  // Read data
  *data = csr->adiv5_data ();

  // Return success
  return ADIv5_OK;
}

const char *Target::ADIv5_Stat (adiv5_stat_t code)
{
  switch (code) {
    case ADIv5_OK:        return "OK";
    case ADIv5_TIMEOUT:   return "ADIv5_TIMEOUT";
    case ADIv5_FAULT:     return "ADIv5_FAULT";
    case ADIv5_NOCONNECT: return "ADIv5_NOCONNECT";
    default:              return "UNKNOWN ERROR";
  }
}

void Target::SetPhy (phy_t phy)
{
  switch (phy) {
    case PHY_SWD:
      csr->jtag_n_swd (0);
      break;
    case PHY_JTAG:
      csr->jtag_n_swd (1);
      break;
  }
}

void Target::BridgeAPSel (uint8_t ap)
{
  csr->apsel (ap);
}

void Target::BridgeEn (bool enabled)
{
  csr->bridge_en (enabled);
}

void Target::BridgeMode (brg_mode_t mode)
{
  switch (mode) {
    case MODE_NORMAL:
      csr->seq (0);
      break;
    case MODE_SEQUENTIAL:
      csr->seq (1);
      break;
  }
}

void Target::IRQScanEn (bool enabled)
{
  csr->irq_scan (enabled);
}

void Target::RegisterIRQHandler (irq_handler handler)
{
  cb = handler;
  flexsoc_register (&flexsoc_irq_convert);
}

void Target::UnregisterIRQHandler (void)
{
  flexsoc_unregister ();
  cb = NULL;
}

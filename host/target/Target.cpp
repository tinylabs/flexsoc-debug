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
static irq_handler_t cb = NULL;
static void flexsoc_irq_convert (uint8_t *buf, int len)
{
    // Assert we have a valid callback
    assert (cb != NULL);
  
    // Make sure length is 2
    assert (len == 2);

    // Pass IRQ to callback
    cb (buf[0], buf[1]);
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

    // Validate CSR matches
    if (Validate () != 0)
        log (LOG_FATAL, "CSR Mismatch - regen gateware/csr");
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
    uint32_t crc = csr->crc32 ();
    if (crc == CRC32)
        log (LOG_TRACE, "CRC matched: %08X", crc);
    return CRC32 != crc;
}

int Target::EnableAP (bool do_enable)
{
    int i;
    uint32_t val;

    log (LOG_TRACE, "EnableAP: %d", do_enable);
  
    // Write to enable AP
    if (do_enable) {
        if (WriteDP (4, 0x50000000) == ADIv5_OK)
            return -1;
        for (i = 0; i < timeout; i++) {
            if (ReadDP (4, &val) == ADIv5_OK)
                return -1;
            if ((val & 0xF0000000) == 0xF0000000)
                break;
        }
    }
    // Disable AP
    else {
        if (WriteDP (4, 0x00000000) == ADIv5_OK)
            return -1;
        for (i = 0; i < timeout; i++) {
            if (ReadDP (4, &val) == ADIv5_OK)
                return -1;
            if ((val & 0xF0000000) == 0x00000000)
                break;
        }
    }
    // Check if we timed out
    if (i == timeout) {
        log (LOG_TRACE, "Timed Out");
        return -1;
    }

    // Setup default CSW - Priv data word access
    if (do_enable)
        WriteAP (0, 0xA3000042);

    // Save status
    ap_enabled = do_enable;
    return 0;
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
    uint32_t id = csr->flexsoc_id ();
    log (LOG_TRACE, "ID: %07X v%X", (id >> 4) & 0xFFFFFFF, id & 0xF);
    return id;
}

uint32_t Target::Reset (bool pswitch)
{
    uint32_t val, idr;

    log (LOG_TRACE, "RESET%s", pswitch ? "+PSWITCH" : "");
    // DP[0xc] is used as a pseudo register to allow
    // protocol specific RESET/PROTOCOL SWITCHING
    WriteDP (0xc, 0);
    if (pswitch)
        WriteDP (0xc, 1);

    // Must read IDR on reset
    idr = this->ReadDP (0, &val);
    log (LOG_TRACE, "IDR: %08X", idr);
    return val;
}

adiv5_stat_t Target::WriteDP (uint8_t addr, uint32_t data)
{
    uint32_t stat;

    log (LOG_TRACE, "WriteDP(%02X): %08X", addr, data);
  
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

    log (LOG_TRACE, "ReadDP(%02X)", addr);

    // Read command
    csr->adiv5_cmd ((addr & 0xc) | 1);

    // Wait until command completes
    do {
        stat = csr->adiv5_status ();
    } while ((stat & 2) == 0);

    // Check results
    if ((adiv5_stat_t)(stat >> 2) != ADIv5_OK) {
        log (LOG_TRACE, "=> %s", ADIv5_Stat ((adiv5_stat_t)(stat >> 2)));
        return (adiv5_stat_t)(stat >> 2);
    }
  
    // Read data
    *data = csr->adiv5_data ();
    log (LOG_TRACE, "=> %08X", *data);
  
    // Return success
    return ADIv5_OK;
}

adiv5_stat_t Target::WriteAP (uint8_t addr, uint32_t data)
{
    uint32_t stat;
    adiv5_stat_t rv;
  
    log (LOG_TRACE, "WriteAP(%02X): %08X", addr, data);
  
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

    if ((adiv5_stat_t)(stat >> 2) != ADIv5_OK)
        log (LOG_TRACE, "=> %s", ADIv5_Stat ((adiv5_stat_t)(stat >> 2)));

    // Return results
    return (adiv5_stat_t)(stat >> 2);
}

adiv5_stat_t Target::ReadAP (uint8_t addr, uint32_t *data)
{
    uint32_t stat;
    adiv5_stat_t rv;
  
    log (LOG_TRACE, "ReadAP(%02X): ", addr);

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
    log (LOG_TRACE, "=> %08X", *data);

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
    log (LOG_TRACE, "SETPHY: %s", phy == PHY_SWD ? "SWD" : "JTAG");
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

void Target::BridgeIRQScanEn (bool enabled)
{
    csr->irq_scan (enabled);
}

void Target::BridgeIRQBuf (uint32_t addr)
{
    csr->irq_base (addr);
}

void Target::RegisterIRQHandler (irq_handler_t handler)
{
    cb = handler;
    flexsoc_register (&flexsoc_irq_convert);
}

void Target::UnregisterIRQHandler (void)
{
    flexsoc_unregister ();
    cb = NULL;
}

void Target::IRQAck (uint8_t cmd)
{
    flexsoc_send (&cmd, 1);
}

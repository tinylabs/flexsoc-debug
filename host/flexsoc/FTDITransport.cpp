/**
 *  FTDI transport implementationn
 *
 *  All rights reserved
 *  Tiny Labs Inc
 *  2019
 */
#include "FTDITransport.h"
#include "log.h"

// VID/PID of FT2232H chipset
#define USB_VID  0x0403
#define USB_PID  0x6010

FTDITransport::FTDITransport (void)
  : Transport ()
{
  int rv;
  
  // Create FTDI structure
  ftdi = ftdi_new ();
  if (!ftdi)
    log (LOG_FATAL, "Failed to create FTDI struct!");

  // Set interface to B
  rv = ftdi_set_interface (ftdi, INTERFACE_B);
  if (rv)
    log (LOG_FATAL, "Failed to set interface!");
}

FTDITransport::~FTDITransport ()
{
  if (ftdi)
    ftdi_free (ftdi);
}

void FTDITransport::ReadSize (uint32_t sz)
{
  if (ftdi_read_data_set_chunksize (ftdi, sz))
    log (LOG_FATAL, "Unable to set read size");
}

void FTDITransport::WriteSize (uint32_t sz)
{
  if (ftdi_write_data_set_chunksize (ftdi, sz))
    log (LOG_FATAL, "Unable to set write size");
}

int FTDITransport::Open (char *id)
{
  int rv, type;
  
  // Look for device by serial # if available
  rv = ftdi_usb_open_desc (ftdi, USB_VID, USB_PID, NULL, *id == '0' ? NULL : id);
  if (rv < 0)
    // No devices found
    return -1;

  // Select the interface
  rv = ftdi_set_interface (ftdi, INTERFACE_B);
  if (rv)
    log (LOG_FATAL, "Failed to set interface");
  
  // Purge buffers
  rv = ftdi_usb_purge_buffers (ftdi);
  if (rv)
    log (LOG_FATAL, "Failed to purge FTDI buffers!");

  // Reset controller
  rv = ftdi_set_bitmode (ftdi, 0, BITMODE_RESET);
  if (rv)
    log (LOG_FATAL, "Failed to reset bitmode");

  // Decrease latency timer to minimum
  rv = ftdi_set_latency_timer (ftdi, 1);
  if (rv)
    log (LOG_FATAL, "Failed to set latency timer");

  // Get interface type
  if (ftdi_get_eeprom_value (ftdi, CHANNEL_B_TYPE, &type))
    log (LOG_FATAL, "Unable to get ch B driver");

  // If UART then set baud and send autobaud character
  if (type == CHANNEL_IS_UART) {
    
    // Set baudrate to 12Mbaud (only applies to serial)
    rv = ftdi_set_baudrate (ftdi, 12000000);
    if (rv)
      log (LOG_FATAL, "Failed to set baudrate");

    // Set 8N2
    rv = ftdi_set_line_property (ftdi, BITS_8, STOP_BIT_1, NONE);
    if (rv)
      log (LOG_FATAL, "Failed to config UART mode: 8N2");
  }
 
  return 0;
}

void FTDITransport::Close (void)
{
  struct ftdi_context *id = ftdi;
  Flush ();
  ftdi = NULL;
  ftdi_usb_close (id);
}

void FTDITransport::Flush (void)
{
  if (ftdi_usb_purge_buffers (ftdi))
    log (LOG_FATAL, "Failed to purge buffers");
}

int FTDITransport::Read (uint8_t *buf, int len)
{
  int rv;

  // Do read
  rv = ftdi_read_data (ftdi, buf, len);

  // Device has been closed - LIBUSB_ERROR_NO_DEVICE
  if (rv == -4)
    return DEVICE_NOTAVAIL;
  else if (rv < 0)
    log (LOG_FATAL, "FTDI read failed: rv=%d", rv);
  return rv;
}

int FTDITransport::Write (const uint8_t *buf, int len)
{
  int rv;
  
  // Do write
  rv = ftdi_write_data (ftdi, buf, len);

  // Device has been closed - LIBUSB_ERROR_NO_DEVICE
  if (rv == -4)
    return DEVICE_NOTAVAIL;
  else if (rv < 0)
    log (LOG_FATAL, "FTDI write failed: rv=%d", rv);

  return rv;
}


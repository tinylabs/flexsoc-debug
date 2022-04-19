#include <stdint.h>
#include <stdio.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "irq.h"

static int gpiosock = -1;

void irq_init (uint16_t port)
{
  int rv;
  struct sockaddr_in a4;

  // Create socket
  gpiosock = socket (AF_INET, SOCK_STREAM, 0);
  if (gpiosock < 0)
    printf ("Unable to create gpiosock!");

  // Setup address
  inet_pton (AF_INET, "127.0.0.1", &a4.sin_addr);
  a4.sin_family = AF_INET;
  a4.sin_port = htons (port);

  // Connect to server
  rv = connect (gpiosock, (struct sockaddr *)&a4, sizeof (a4));
  printf ("rv = %d\n", rv);
  if (rv != 0)
    printf ("Failed to connect to localhost:%d\n", port);
  printf ("Connected to remote GPIO :%d\n", port);
}

void irq_exit (void)
{
  if (gpiosock != -1)
    close (gpiosock);
}

void send_irq (uint8_t irq, uint8_t active)
{
  uint8_t data[2];

  if (gpiosock == -1)
    return;
  
  // Generate data output
  if (active)
    data[0] = (irq & 0x7F) | 0x80;
  else
    data[0] = (irq & 0x7F);

  // Add flush command
  data[1] = 0xFF;

  // Write out to socket
  if (write (gpiosock, data, 2) != 2)
    printf ("Failed to send IRQ\n");
}

void pulse_irq (uint8_t irq)
{
  send_irq (irq, 1);
  send_irq (irq, 0);
}

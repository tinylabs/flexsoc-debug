/**
 *   All rights reserved.
 *   Tiny Labs Inc
 *   2018-2022
 *
 */
    .syntax     unified
    .arch       armv7-m

    /* Stack for preempted IRQs */
    .equ IRQ_NESTED,    6          /* Allow for 8 nested/preempted IRQ */
    .equ IRQ_STACK_SZ,  8*4*IRQ_NESTED
    
    /*   Core register map is static as very little is actually happening
     *   on target. The following are stored in registers:
     *   R0 - Current IRQ taken from IPSR  |- Backed up on stack 
     *   R1 -                              |
     *   R2 -                              |
     *   R3 -                              |
     *   R4 - Base pointer of IRQ stack (ascending) 
     *   R5 - Base pointer of IRQ servicing (ascending)
     *   R6 - Data reg, assemble before sending
     *   R7 - 
     *   R8 - 
     *   R9 -  IRQ counter (32b with rollover)
     *   R10 - Interrupt clear register
     *   R11 - DCRDR - 3 IRQ fifo + 8bit LSB counter
     *   R12 -                             |- Backed up on stack
    */

    /* Name registers for clarity */
    ZERO .req  r7   /* Always zero */
    MSK  .req  r8   /* MSK of storage offset */
    CTR  .req  r9   /* Total IRQ counter */
    IRQ  .req  r10  /* ICER */
    ACK  .req  r11  /* DCRDR */
    
    /* Core ISR vector table */
    .section    .vector
__isr_vector:
    .align      2                /* IRQ  Priority  Name    (cc=configurable)  */
     b          Reset_Handler    /* Now we can direcly call blob MSP setup below */
    .word       __irq_buf        /* 1    -3        Reset handler              */
    .word       NMI_ISR          /* 2    -2        NMI handler                */
    .word       HardFault_ISR    /* 3    -1        Hard Fault handler         */
    .word       Fault_ISR        /* 4    cc        Mem manage                 */
    .word       Fault_ISR        /* 5    cc        Bus fault                  */
    .word       Fault_ISR        /* 6    cc        Usage fault                */
    .word       Fault_ISR        /* 7    --        Unused                     */
    .word       Fault_ISR        /* 8    --        Unused                     */
    .word       Fault_ISR        /* 9    --        Unused                     */
    .word       Fault_ISR        /* 10   --        Unused                     */
    .word       Fault_ISR        /* 11   cc        SVCall                     */
    .word       Fault_ISR        /* 12   cc        Debug monitor              */
    .word       Fault_ISR        /* 13   --        Not used                   */
    .word       Fault_ISR        /* 14   cc        PendSD                     */  
    .word       Fault_ISR        /* 15   cc        Systick                    */

    /* Generate common vector entry for each external IRQ */
    .rept 252
    .word       Forward_ISR
    .endr
    
    /* IRQ Handlers */
    .text
    .align  1

/* Register usage 
 *
 * r0 - IRQ number
 * r1 - Bit shift value
 * r2 - Disable bitmask
 * r3 - Offset from base IECR
 */
/* Use shared IRQ with slightly longer execution time */
Forward_ISR:
    /* Fetch IRQ from IPSR */
    mrs r0, IPSR
    mrs r1, MSP

    /* Calculate depth of IRQ
     * bits 6:5 indicate IRQ depth
     * 80 1000 0000
     * 60 0110 0000
     * 40 0100 0000
     * 20 0010 0000
     */
    and r1, MSK, r1, lsr #5
    
    /* Write to shared buffer */
    strb r0, [IRQ, r1]
    
    /* Wait for acknowledgment */
wait_ack:
    ldrb r0, [ACK, r1]
    bne wait_ack
    
    /* Clear IRQ */
    strb ZERO, [IRQ, r1]
    
    /* Increment IRQ counter*/
    add CTR, #1
    
    /* Exit ISR */
    bx lr
    
    /* Reset handler
     *   Must do the following:
     *   - Configure core registers with constant addresses needed
     *   - 
     *   - Sleep with wfi until IRQ is active
     */
Reset_Handler:
    ldr r0, =__stack_top
    msr msp, r0
    mov CTR, #0
    ldr IRQ, =__irq_buf
    ldr ACK, =__ack_buf
    mov MSK, 3
    mov ZERO, #0
    
    // Sleep until IRQ comes in
wait:
    wfi
    b wait

NMI_ISR:
    bx lr
    
HardFault_ISR:
    bx lr
    
Fault_ISR:
    bx lr

    /* Put literal pool here */
    .ltorg

    /* Allocated shared buffer for IRQ forwarding */
    .align 4 // 16byte alignment for Bn access
    __irq_buf = .
    .word  0
    __ack_buf = .
    .word 0
    
    /* Allocate space for stack */
    .word 0xdeadbeef
    .align 4
    .space IRQ_STACK_SZ, 0x11
    __stack_top = .
.end

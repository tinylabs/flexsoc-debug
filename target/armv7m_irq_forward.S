/**
 *   All rights reserved.
 *   Tiny Labs Inc
 *   2018-2022
 *
 */
    .syntax     unified
    .arch       armv7-m

    /* Constants */
    .equ DCRDR,          0xE000EDF8 /* Scratch register to talk to debugger */
    .equ ICER,           0xE000E180 /* Interrupt clear base */
    /* FIFO for handling IRQs */
    .equ IRQ_FIFO_SZ,   0x40       /* Space for 16 nested interrupts */
    .equ IRQ_FIFO_BASE, 0x20000400 + IRQ_FIFO_SZ
    /* Stack for preempted IRQs */
    .equ IRQ_NESTED,    8          /* Allow for 8 nested IRQ */
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
    IIRQ .req  r4   /* Incoming IRQ */
    OIRQ .req  r5   /* Outgoing IRQ */
    BUF  .req  r6   /* Buffer to send */
    CTR  .req  r9   /* Total IRQ counter */
    DREG .req  r11  /* DCRDR */
    ICR  .req  r10  /* ICER */
    
    /* Core ISR vector table */
    .section    .vector
__isr_vector:
    .align      2                /* IRQ  Priority  Name    (cc=configurable)  */
     b          Reset_Handler    /* Now we can direcly call blob MSP setup below */
    .word       Reset_Handler    /* 1    -3        Reset handler              */
    .long       NMI_ISR          /* 2    -2        NMI handler                */
    .long       HardFault_ISR    /* 3    -1        Hard Fault handler         */
    .long       Fault_ISR        /* 4    cc        Mem manage                 */
    .long       Fault_ISR        /* 5    cc        Bus fault                  */
    .long       Fault_ISR        /* 6    cc        Usage fault                */
    .long       Fault_ISR        /* 7    --        Unused                     */
    .long       Fault_ISR        /* 8    --        Unused                     */
    .long       Fault_ISR        /* 9    --        Unused                     */
    .long       Fault_ISR        /* 10   --        Unused                     */
    .long       Fault_ISR        /* 11   cc        SVCall                     */
    .long       Fault_ISR        /* 12   cc        Debug monitor              */
    .long       Fault_ISR        /* 13   --        Not used                   */
    .long       Fault_ISR        /* 14   cc        PendSD                     */  
    .long       Fault_ISR        /* 15   cc        Systick                    */

.if SPACE_OPTIMIZED
    /* Generate common vector entry for each external IRQ */
    .rept 252
    .long       Forward_ISR
    .endr
.else
    .irp IRQ, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, \
    29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, \
    45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, \
    61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, \
    77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, \
    93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, \
    107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, \
    120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, \
    133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, \
    146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, \
    159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, \
    172, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, \
    185, 186, 187, 188, 189, 190, 191, 192, 193, 194, 195, 196, 197, \
    198, 199, 200, 201, 202, 203, 204, 205, 206, 207, 208, 209, 210, \
    211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223, \
    224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, \
    237, 238, 239, 240, 241, 242, 243, 244, 245, 246, 247, 248, 249, \
    250, 251, 252, 253, 254, 255
    .long       ISR_\IRQ
    .endr
.endif
    
    /* IRQ Handlers */
    .text
    .align  1

/* Use shared IRQ with slightly longer execution time */
.if SPACE_OPTIMIZED
Forward_ISR:
    add CTR, #1          // Increment global counter
    /* Disable IRQ until serviced on the host */
    /* Get the shift amount */
    and r1, r0, #0x1f
    /* Shift by 5 lsbs of IRQ */
    mov r2, #1
    lsl r2, r1
    /* Calculate address for write
    *  top 27 bits */
    lsr r3, r0, #3
    /* Clear bottom two bits ie: x4 
     * to get offset from base */
    and r3, #0xFFFFFFFC
    /* Disable IRQ until host is finished running
     * ISR */
    str r2, [ICR, r3]
    /* Write IRQ into FIFO */
    mrs r0, IPSR         // Fetch IRQ from IPSR
    stmia IIRQ!, {r0}    // Push IRQ and global counter onto IRQ stack
    /* Exit ISR */
    bx lr

.else
    /* Use time optimized handlers at the cost of space */
    /* Generate unique ISR for each */
    .irp IRQ, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, \
    29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, \
    45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, \
    61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, \
    77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, \
    93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, \
    107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, \
    120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, \
    133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, \
    146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, \
    159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, \
    172, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, \
    185, 186, 187, 188, 189, 190, 191, 192, 193, 194, 195, 196, 197, \
    198, 199, 200, 201, 202, 203, 204, 205, 206, 207, 208, 209, 210, \
    211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223, \
    224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, \
    237, 238, 239, 240, 241, 242, 243, 244, 245, 246, 247, 248, 249, \
    250, 251, 252, 253, 254, 255
ISR_\IRQ:
    add CTR, #1
    mov r0, #\IRQ
    /* Disable IRQ */
    mov r2, #(1 << (\IRQ & 0x1f))
    str r2, [ICR, #((\IRQ >> 5) * 4)]
    /* Save IRQ */
    stmia IIRQ!, {r0}    
    bx lr
    .endr
.endif
    
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
    ldr ICR, =ICER
    ldr DREG, =DCRDR
wait:
    // Increase priority to -1 to prevent preemption
    // Set both stacks to base if they're equal
    cpsid f
    teq IIRQ, OIRQ
    itt eq
    ldreq IIRQ, =IRQ_FIFO_BASE
    ldreq OIRQ, =IRQ_FIFO_BASE
    cpsie f
    isb

    // Busy wait, we cannot sleep because
    // IRQ could come in right before sleep and we
    // couldn't process
loop:
    teq IIRQ, OIRQ
    beq loop

    /* Clear transfer reg */
    mov BUF, #0
next:
    /* Pop IRQ off of stack */
    ldmia OIRQ!, {r1}
    /* Push IRQ onto transfer reg */
    /* DCRDR = (DCRDR << 8) | IRQ */
    orr BUF, r1, BUF, lsl #8
    /* Are we caught up */
    teq IIRQ, OIRQ
    beq send
    /* Is buffer at max? */
    ands BUF, #0x00FF0000
    beq send
    b next
send:
    /* Write 8bit LSB from counter */
    bfi BUF, CTR, #0, #8
    /* Write to DCRDR */
    str BUF, [DREG]
    /* Are we caught up */
    teq IIRQ, OIRQ
    beq wait
    /* Assemble next buffer */
    b next

    /* Register usage 
     *
     * r0 - IRQ number
     * r1 - Bit shift value
     * r2 - Disable bitmask
     * r3 - Offset from base IECR
    */
    
NMI_ISR:
    bx lr
    
HardFault_ISR:
    bx lr
    
Fault_ISR:
    bx lr

    /* Put literal pool here */
    .ltorg

    /* Allocate space for FIFO */
    /* 4 byte alignment */
    .align 2
    .space IRQ_FIFO_SZ
    /* Allocate space for stack */
    .word 0xdeadbeef
    .space IRQ_STACK_SZ
    __stack_top = .
.end

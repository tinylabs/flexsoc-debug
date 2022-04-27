/* Everything directly loaded and executed out of RAM */
MEMORY
{
    RAM ( rxw )       : ORIGIN = 0x20000000, LENGTH = 8k
}

SECTIONS
{
    .ramcode :
    {
        *(.vector)
        *(.text*)
        *(.data*)
        *(.rodata*)
    } > RAM
}

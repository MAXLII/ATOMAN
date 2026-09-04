--retain '(AUTO_REG_SECTION)'

MEMORY
{
PAGE 0 :
   BEGIN       : origin = 0x000000, length = 0x000002
   RAMM0       : origin = 0x0000F6, length = 0x00030A
   RAMLS0      : origin = 0x008000, length = 0x000800
   RAMLS1      : origin = 0x008800, length = 0x000800
   RAMLS2      : origin = 0x009000, length = 0x000800
   RAMLS3      : origin = 0x009800, length = 0x000800
   RAMLS4      : origin = 0x00A000, length = 0x000800
   RAMGS0      : origin = 0x00C000, length = 0x002000
   RAMGS1      : origin = 0x00E000, length = 0x002000
   RESET       : origin = 0x3FFFC0, length = 0x000002

PAGE 1 :
   BOOT_RSVD   : origin = 0x000002, length = 0x0000F1
   RAMM1       : origin = 0x000400, length = 0x0003F8
   RAMLS5      : origin = 0x00A800, length = 0x000800
   RAMLS6      : origin = 0x00B000, length = 0x000800
   RAMLS7      : origin = 0x00B800, length = 0x000800
   RAMGS2      : origin = 0x010000, length = 0x002000
   RAMGS3      : origin = 0x012000, length = 0x001FF8
}

SECTIONS
{
   codestart           : > BEGIN, PAGE = 0
   .TI.ramfunc         : > RAMLS4, PAGE = 0
   .text               : >> RAMLS0 | RAMLS1 | RAMLS2 | RAMLS3 | RAMGS0 | RAMGS1, PAGE = 0
   .cinit              : > RAMGS1, PAGE = 0
   .switch             : > RAMM0, PAGE = 0
   .reset              : > RESET, PAGE = 0, TYPE = DSECT

   .stack              : > RAMM1, PAGE = 1

#if defined(__TI_EABI__)
   .bss                : >> RAMGS2 | RAMGS3, PAGE = 1
   .bss:output         : > RAMGS2, PAGE = 1
   .bss:cio            : > RAMLS6, PAGE = 1
   .init_array         : > RAMM0, PAGE = 0
   .const              : >> RAMLS5 | RAMLS6, PAGE = 1
   .data               : >> RAMGS2 | RAMGS3, PAGE = 1
   .sysmem             : > RAMGS3, PAGE = 1
   AUTO_REG_SECTION :
   {
      __section_start = .;
      *(AUTO_REG_SECTION)
      __section_end = .;
   } > RAMLS7, PAGE = 1
#else
   .pinit              : > RAMM0, PAGE = 0
   .ebss               : >> RAMGS2 | RAMGS3, PAGE = 1
   .econst             : >> RAMLS5 | RAMLS6, PAGE = 1
   .esysmem            : > RAMGS3, PAGE = 1
#endif

   ramgs2              : > RAMGS2, PAGE = 1
   ramgs3              : > RAMGS3, PAGE = 1
}

/*         ______   ___    ___ 
 *        /\  _  \ /\_ \  /\_ \ 
 *        \ \ \L\ \\//\ \ \//\ \      __     __   _ __   ___ 
 *         \ \  __ \ \ \ \  \ \ \   /'__`\ /'_ `\/\`'__\/ __`\
 *          \ \ \/\ \ \_\ \_ \_\ \_/\  __//\ \L\ \ \ \//\ \L\ \
 *           \ \_\ \_\/\____\/\____\ \____\ \____ \ \_\\ \____/
 *            \/_/\/_/\/____/\/____/\/____/\/___L\ \/_/ \/___/
 *                                           /\____/
 *                                           \_/__/
 *      By Shawn Hargreaves
 *      shawn@talula.demon.co.uk
 *      http://www.talula.demon.co.uk/allegro/
 *
 *      SVGA bank switching code. These routines will be called with
 *      a line number in %eax and a pointer to the bitmap in %edx. The
 *      bank switcher should select the appropriate bank for the line,
 *      and replace %eax with a pointer to the start of the line.
 *
 *      See readme.txt for copyright information.
 */


#ifndef DJGPP
#error This file should only be used by the djgpp version of Allegro
#endif

#include "asmdefs.inc"

.text



#define SETUP(name)
#define CLEANUP(name)


/* Template for bank switchers. Produces a framework that checks which 
 * bank is currently selected and only executes the provided code if it
 * needs to change bank. Restores %eax: if the switch routine touches any 
 * other registers it must save and restore them itself.
 */
#define BANK_SWITCHER(name, cache, code...)                                  \
.globl name                                                                ; \
   .align 4                                                                ; \
name:                                                                      ; \
   SETUP(name)                                                             ; \
									   ; \
   pushl %eax                                                              ; \
									   ; \
   addl BMP_YOFFSET(%edx), %eax              /* add line offset */         ; \
									   ; \
   shll $2, %eax                                                           ; \
   addl __gfx_bank, %eax                     /* lookup which bank */       ; \
   movl (%eax), %eax                                                       ; \
   cmpl cache, %eax                          /* need to change? */         ; \
   je name##_done                                                          ; \
									   ; \
   movl %eax, cache                          /* store the new bank */      ; \
   code                                      /* and change it */           ; \
									   ; \
   .align 4, 0x90                                                          ; \
name##_done:                                                               ; \
   popl %eax                                                               ; \
   movl BMP_LINE(%edx, %eax, 4), %eax        /* load line address */       ; \
									   ; \
   CLEANUP(name)                                                           ; \
   ret                                                                     ; \
									   ; \
.globl name##_end                                                          ; \
   .align 4                                                                ; \
name##_end:                                                                ; \
   ret 




/* Uses VESA function 05h (real mode interrupt) to select the bank in %eax.
 * Restores all registers except %eax, which is handled by BANK_SWITCHER.
 */
#define SET_VESA_BANK_RM(window)                                             \
   pushal                                    /* store registers */         ; \
   pushw %es                                                               ; \
									   ; \
   movw ___djgpp_ds_alias, %bx                                             ; \
   movw %bx, %es                                                           ; \
									   ; \
   movl $0x10, %ebx                          /* call int 0x10 */           ; \
   movl $0, %ecx                             /* no stack required */       ; \
   movl $__dpmi_reg, %edi                    /* register structure */      ; \
									   ; \
   movw $0x4F05, DPMI_AX(%edi)               /* VESA function 05h */       ; \
   movw $window, DPMI_BX(%edi)               /* which window? */           ; \
   movw %ax, DPMI_DX(%edi)                   /* which bank? */             ; \
   movw $0, DPMI_SP(%edi)                    /* zero stack */              ; \
   movw $0, DPMI_SS(%edi)                                                  ; \
   movw $0, DPMI_FLAGS(%edi)                 /* and zero flags */          ; \
									   ; \
   movl $0x0300, %eax                        /* simulate RM interrupt */   ; \
   int $0x31                                                               ; \
									   ; \
   popw %es                                  /* restore registers */       ; \
   popal




/* Uses the VESA 2.0 protected mode interface to select the bank in %eax.
 * Restores all registers except %eax, which is handled by BANK_SWITCHER.
 */
#define SET_VESA_BANK_PM(window)                                             \
   pushal                                    /* store registers */         ; \
									   ; \
   movw $window, %bx                         /* which window? */           ; \
   movw %ax, %dx                             /* which bank? */             ; \
   movl __pm_vesa_switcher, %eax                                           ; \
   call *%eax                                /* do it! */                  ; \
									   ; \
   popal                                     /* restore registers */




/* Uses the VESA 2.0 protected mode interface to select the bank in %eax,
 * passing a selector for memory mapped io in %es. Restores all registers 
 * except %eax, which is handled by BANK_SWITCHER.
 */
#define SET_VESA_BANK_PM_ES(window)                                          \
   pushal                                    /* store registers */         ; \
   pushw %es                                                               ; \
									   ; \
   movw $window, %bx                         /* which window? */           ; \
   movw %ax, %dx                             /* which bank? */             ; \
   movw __mmio_segment, %ax                  /* load selector into %es */  ; \
   movw %ax, %es                                                           ; \
   movl __pm_vesa_switcher, %eax                                           ; \
   call *%eax                                /* do it! */                  ; \
									   ; \
   popw %es                                  /* restore registers */       ; \
   popal




/* VESA window 1 switching routines */
BANK_SWITCHER( __vesa_window_1, __last_bank_1, SET_VESA_BANK_RM(0) )
BANK_SWITCHER( __vesa_pm_window_1, __last_bank_1, SET_VESA_BANK_PM(0) )
BANK_SWITCHER( __vesa_pm_es_window_1, __last_bank_1, SET_VESA_BANK_PM_ES(0) )



/* VESA window 2 switching routines */
#undef CLEANUP
#define CLEANUP(name) addl __window_2_offset, %eax
BANK_SWITCHER( __vesa_window_2, __last_bank_2, SET_VESA_BANK_RM(1) )
BANK_SWITCHER( __vesa_pm_window_2, __last_bank_2, SET_VESA_BANK_PM(1) )
BANK_SWITCHER( __vesa_pm_es_window_2, __last_bank_2, SET_VESA_BANK_PM_ES(1) )
#undef CLEANUP
#define CLEANUP(name)




.globl __af_wrapper
__af_wrapper:



/* callback for VBE/AF to access real mode interrupts (DPMI 0x300) */
.globl __af_int86
   .align 4
__af_int86:
   pushal
   pushw %es
   pushw %ds
   popw %es

   xorb %bh, %bh
   xorl %ecx, %ecx

   movl $0x300, %eax
   int $0x31 

   popw %es
   popal
   ret



/* callback for VBE/AF to access real mode functions (DPMI 0x301) */
.globl __af_call_rm
   .align 4
__af_call_rm:
   pushal
   pushw %es
   pushw %ds
   popw %es

   xorb %bh, %bh
   xorl %ecx, %ecx

   movl $0x301, %eax
   int $0x31 

   popw %es
   popal
   ret



.globl __af_wrapper_end
__af_wrapper_end:




/* helper for accelerated driver bank switches, which arbitrates between
 * hardware and cpu control over the video memory.
 */
#undef SETUP
#define SETUP(name)                                                          \
   cmpl $0, __accel_active    /* is the accelerator active? */             ; \
   jz name##_inactive                                                      ; \
									   ; \
   pushl %eax                 /* save registers */                         ; \
   pushl %ecx                                                              ; \
   pushl %edx                                                              ; \
									   ; \
   movl __accel_driver, %ecx                                               ; \
   movl __accel_idle, %edx                                                 ; \
   pushl %ecx                                                              ; \
   call *%edx                 /* turn off the graphics hardware */         ; \
   addl $4, %esp                                                           ; \
									   ; \
   popl %edx                  /* restore registers */                      ; \
   popl %ecx                                                               ; \
   popl %eax                                                               ; \
									   ; \
   movl $0, __accel_active    /* flag accelerator is inactive */           ; \
									   ; \
   .align 4, 0x90                                                          ; \
name##_inactive:



/* stub for linear accelerated graphics modes */
.globl __accel_bank_stub
   .align 4
__accel_bank_stub:
   SETUP(__accel_bank_stub)
   movl BMP_LINE(%edx, %eax, 4), %eax
   ret

.globl __accel_bank_stub_end
   .align 4
__accel_bank_stub_end:
   ret



/* bank switcher for accelerated graphics modes */
BANK_SWITCHER( __accel_bank_switch, __last_bank_1,
   pushl %ecx;
   pushl %edx;
   movl __accel_driver, %ecx;
   movl __accel_set_bank, %edx;
   pushl %eax;
   pushl %ecx;
   call *%edx;
   addl $8, %esp;
   popl %edx;
   popl %ecx;
)

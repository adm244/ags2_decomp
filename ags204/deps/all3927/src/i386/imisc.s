/*         ______   ___    ___ 
 *        /\  _  \ /\_ \  /\_ \ 
 *        \ \ \L\ \\//\ \ \//\ \      __     __   _ __   ___ 
 *         \ \  __ \ \ \ \  \ \ \   /'__`\ /'_ `\/\`'__\/ __`\
 *          \ \ \/\ \ \_\ \_ \_\ \_/\  __//\ \L\ \ \ \//\ \L\ \
 *           \ \_\ \_\/\____\/\____\ \____\ \____ \ \_\\ \____/
 *            \/_/\/_/\/____/\/____/\/____/\/___L\ \/_/ \/___/
 *                                           /\____/
 *                                           \_/__/
 *
 *      Math routines, compiled sprite wrapper, etc.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#include "asmdefs.inc"

.text



/* empty bank switch routine for the standard VGA mode and memory bitmaps */
FUNC(_stub_bank_switch)
   movl BMP_LINE(%edx, %eax, 4), %eax
   ret

FUNC(_stub_unbank_switch)
   ret

FUNC(_stub_bank_switch_end)
   ret




/* void apply_matrix_f(MATRIX_f *m, float x, float y, float z, 
 *                                  float *xout, float *yout, float *zout);
 *  Floating point vector by matrix multiplication routine.
 */
FUNC(apply_matrix_f)

   #define MTX    ARG1
   #define X      ARG2
   #define Y      ARG3
   #define Z      ARG4
   #define XOUT   ARG5
   #define YOUT   ARG6
   #define ZOUT   ARG7

   pushl %ebp
   movl %esp, %ebp
   pushl %ebx

   movl MTX, %edx 
   movl XOUT, %eax 
   movl YOUT, %ebx 
   movl ZOUT, %ecx 

   flds  M_V00(%edx) 
   fmuls X 
   flds  M_V01(%edx) 
   fmuls Y 
   flds  M_V02(%edx) 
   fmuls Z 
   fxch  %st(2) 

   faddp %st(0), %st(1) 
   flds  M_V10(%edx) 
   fxch  %st(2) 

   faddp %st(0), %st(1) 
   fxch  %st(1) 

   fmuls X 
   fxch  %st(1) 

   fadds M_T0(%edx) 
   flds  M_V11(%edx) 

   fmuls Y 
   flds  M_V12(%edx) 

   fmuls Z 
   fxch  %st(1) 

   faddp %st(0), %st(3) 
   flds  M_V20(%edx) 
   fxch  %st(3) 

   faddp %st(0), %st(1) 
   fxch  %st(2) 

   fmuls X 
   fxch  %st(2) 

   fadds M_T1(%edx) 
   flds  M_V21(%edx) 

   fmuls Y 
   flds  M_V22(%edx) 

   fmuls Z 
   fxch  %st(4) 

   faddp %st(0), %st(1) 
   fxch  %st(1) 
   fstps (%ebx) 

   faddp %st(0), %st(2) 
   fstps (%eax) 

   fadds M_T2(%edx) 
   fstps (%ecx)

   popl %ebx
   movl %ebp, %esp
   popl %ebp
   ret                              /* end of apply_matrix_f() */




#undef X
#undef Y




/* void draw_compiled_sprite(BITMAP *bmp, COMPILED_SPRITE *sprite, int x, y)
 *  Draws a compiled sprite onto the specified bitmap at the specified
 *  position, _ignoring_ clipping. The bitmap must be in the same format
 *  that the sprite was compiled for.
 */
FUNC(draw_compiled_sprite)

   #define BMP       ARG1
   #define SPRITE    ARG2
   #define X         ARG3
   #define Y         ARG4

   pushl %ebp
   movl %esp, %ebp
   subl $4, %esp                 /* 1 local variable: */

   #define PLANE     -4(%ebp)

   pushl %ebx
   pushl %esi
   pushl %edi

   movl BMP, %edx                /* bitmap pointer in edx */
 #ifdef USE_FS
   movw BMP_SEG(%edx), %fs       /* load segment selector into fs */
 #endif

   movl SPRITE, %ebx
   cmpw $0, CMP_PLANAR(%ebx)     /* is the sprite planar or linear? */
   je linear_compiled_sprite

   movl X, %ecx                  /* get write plane mask in bx */
   andb $3, %cl
   movl $0x1102, %ebx
   shlb %cl, %bh

   movl BMP_LINE+4(%edx), %ecx   /* get line width in ecx */
   subl BMP_LINE(%edx), %ecx

   movl X, %esi                  /* get destination address in edi */
   shrl $2, %esi
   movl Y, %edi
   movl BMP_LINE(%edx, %edi, 4), %edi
   addl %esi, %edi

   movl $0x3C4, %edx             /* port address in dx */

   movl $0, PLANE                /* zero the plane counter */

   _align_
planar_compiled_sprite_loop:
   movl %ebx, %eax               /* set the write plane */
   outw %ax, %dx 

   movl %edi, %eax               /* get address in eax */

   movl PLANE, %esi              /* get the drawer function in esi */
   shll $3, %esi
   addl SPRITE, %esi
   movl CMP_DRAW(%esi), %esi

   call *%esi                    /* and draw the plane! */

   incl PLANE                    /* next plane */
   cmpl $4, PLANE
   jge draw_compiled_sprite_done

   rolb $1, %bh                  /* advance the plane position */
   adcl $0, %edi
   jmp planar_compiled_sprite_loop

   _align_
linear_compiled_sprite:
   movl X, %eax
   movzwl CMP_COLOR_DEPTH(%ebx), %ecx
   cmpl $24, %ecx
   jne normal_linear_compiled_sprite
   leal (%eax, %eax, 2), %eax
   jmp end24bpp_linear_compiled_sprite

   _align_
normal_linear_compiled_sprite:
   addl $7, %ecx
   shrl $4, %ecx
   shll %cl, %eax

end24bpp_linear_compiled_sprite:
   movl %eax, %ecx               /* x coordinate in ecx */
   movl Y, %edi                  /* y coordinate in edi */
   movl BMP_WBANK(%edx), %esi    /* bank switch function in esi */
   movl CMP_DRAW(%ebx), %ebx     /* drawer function in ebx */

   call *%ebx                    /* and draw it! */

draw_compiled_sprite_done:
   movl BMP, %edx
   UNWRITE_BANK()

   popl %edi
   popl %esi
   popl %ebx
   movl %ebp, %esp
   popl %ebp
   ret                           /* end of draw_compiled_sprite() */




/* void _do_stretch(BITMAP *source, BITMAP *dest, void *drawer, 
 *                  int sx, fixed sy, fixed syd, int dx, int dy, int dh, 
 *                  int color_depth);
 *
 *  Helper function for stretch_blit(), calls the compiled line drawer.
 */
FUNC(_do_stretch)

   #define SOURCE       ARG1
   #define DEST         ARG2
   #define DRAWER       ARG3
   #define SX           ARG4
   #define SY           ARG5
   #define SYD          ARG6
   #define DX           ARG7
   #define DY           ARG8
   #define DH           ARG9
   #define COL_DEPTH    ARG10

   pushl %ebp
   movl %esp, %ebp
   pushw %es
   pushl %edi
   pushl %esi
   pushl %ebx

   movl DEST, %edx
   movw BMP_SEG(%edx), %es       /* load destination segment */
   movl DRAWER, %ebx             /* the actual line drawer */

   movl BMP_ID(%edx), %eax
   testl $BMP_ID_PLANAR, %eax
   jnz stretch_modex_loop
   movl COL_DEPTH, %eax
   cmpl $8, %eax
   je stretch_normal_loop
   cmpl $15, %eax
   je stretch_bpp_16
   cmpl $16, %eax
   je stretch_bpp_16
   cmpl $24, %eax
   je stretch_bpp_24
   cmpl $32, %eax
   je stretch_bpp_32
   jmp stretch_done


   /* special loop for 24 bit */
   _align_
stretch_bpp_24:
   movl SX, %eax
   leal (%eax, %eax, 2), %eax
   movl %eax, SX
   movl DX, %eax
   leal (%eax, %eax, 2), %eax
   movl %eax, DX

   _align_
stretch_loop24:
   movl SOURCE, %edx             /* get source line (in esi) and bank */
   movl SY, %eax
   shrl $16, %eax
   READ_BANK()
   movl %eax, %esi
   addl SX, %esi

   movl DEST, %edx               /* get dest line (in edi) and bank */
   movl DY, %eax
   WRITE_BANK()
   movl %eax, %edi
   addl DX, %edi
   pushl %edx
   pushl %ebx

   call *%ebx                    /* draw (clobbers eax, ebx, ecx, edx) */

   popl %ebx
   popl %edx
   movl SYD, %eax                /* next line in source bitmap */
   addl %eax, SY
   incl DY                       /* next line in dest bitmap */
   decl DH
   jg stretch_loop24
   jmp stretch_done


   /* special loop for mode-X */
   _align_
stretch_modex_loop:
   movl SOURCE, %edx             /* get source line (in esi) and bank */
   movl SY, %eax
   shrl $16, %eax
   movl BMP_LINE(%edx, %eax, 4), %esi
   addl SX, %esi

   movl DEST, %edx               /* get dest line (in edi) and bank */
   movl DY, %eax
   movl BMP_LINE(%edx, %eax, 4), %edi
   addl DX, %edi

   call *%ebx                    /* draw the line (clobbers eax and ecx) */

   movl SYD, %eax                /* next line in source bitmap */
   addl %eax, SY
   incl DY                       /* next line in dest bitmap */
   decl DH
   jg stretch_modex_loop
   jmp stretch_done


   _align_
stretch_bpp_16:
   shll $1, SX
   shll $1, DX
   jmp stretch_normal_loop

   _align_
stretch_bpp_32:
   shll $2, SX
   shll $2, DX


   /* normal stretching loop */
   _align_
stretch_normal_loop:
   movl SOURCE, %edx             /* get source line (in esi) and bank */
   movl SY, %eax
   shrl $16, %eax
   READ_BANK()
   movl %eax, %esi
   addl SX, %esi

   movl DEST, %edx               /* get dest line (in edi) and bank */
   movl DY, %eax
   WRITE_BANK()
   movl %eax, %edi
   addl DX, %edi

   call *%ebx                    /* draw the line (clobbers eax and ecx) */

   movl SYD, %eax                /* next line in source bitmap */
   addl %eax, SY
   incl DY                       /* next line in dest bitmap */
   decl DH
   jg stretch_normal_loop


stretch_done:
   movl SOURCE, %edx
   UNWRITE_BANK()

   movl DEST, %edx
   UNWRITE_BANK()

   popl %ebx
   popl %esi
   popl %edi
   popw %es
   movl %ebp, %esp
   popl %ebp
   ret                           /* end of _do_stretch() */




/* unsigned long _blender_trans24(unsigned long x, y, n);
 *  24 bit trans blender function. See colblend.c for the others.
 */
FUNC(_blender_trans24)
   pushl %ebp
   movl %esp, %ebp
   pushl %esi
   pushl %ecx
   pushl %ebx

   movl ARG1, %esi
   movl ARG2, %ebx
   movl ARG3, %ecx

   movl %esi, %eax
   movl %ebx, %edx
   andl $0xFF00FF, %eax
   andl $0xFF00FF, %edx

   orl %ecx, %ecx
   jz noinc

   incl %ecx

noinc:
   subl %edx, %eax
   imull %ecx, %eax
   shrl $8, %eax
   addl %ebx, %eax

   andl $0xFF00, %ebx
   andl $0xFF00, %esi

   subl %ebx, %esi
   imull %ecx, %esi
   shrl $8, %esi
   addl %ebx, %esi
   andl $0xFF00FF, %eax
   andl $0xFF00, %esi

   orl %esi, %eax

   popl %ebx
   popl %ecx
   popl %esi
   movl %ebp, %esp
   popl %ebp
   ret                           /* end of _blender_trans24() */


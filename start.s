
;@-------------------------------------------------------------------------
;@-------------------------------------------------------------------------

.globl _start
_start:
    // read cpu id, stop slave cores
    mrc p15, #0, r1, c0, c0, #5
    and r1, r1, #3
    cmp r1, #0
    bne hang

    // set vector address.
    ldr r0, =_vectors
    mcr P15, 0, r0, c12, c0, 0

    .equ MODE_IRQ, 0x12
    .equ MODE_SVC, 0x13
    .equ IRQ_BIT,  0x80
    .equ FIQ_BIT,  0x40

    // Inicializa MMU
    bl mmu_init

    // save CPSR.
    mrs r0, cpsr

    // Entrar em modo IRQ
    msr cpsr_c, #MODE_IRQ
    bl init_irq_stack

    // Entrar em modo SVC
    msr cpsr, r0
    bl init_svc_stack

@     // Clear bss.
@     ldr r4, =__bss_start__
@     ldr r9, =__bss_end__
@     mov r5, #0
@     mov r6, #0
@     mov r7, #0
@     mov r8, #0
@     b       2f

@ 1:
@     // store multiple at r4.
@     stmia r4!, {r5-r8}

@     // If we are still below bss_end, loop.
@ 2:
@     cmp r4, r9
@     blo 1b

    // Entrar em modo SVC
    @ msr cpsr_c, #MODE_SVC

    // Call kernel_main
    ldr r3, =main
    blx r3

hang:
    wfi
    b hang
    // cpu id > 0, stop
1:  wfe
    b       1b
2:
    // Definir endereço de vector table
    ldr r1, _vectors
    mrc p15, 0, r1, c12, c0, 0

    // Iniciar MMU
    @ bl mmu_init

    // iniciar stack dos modos
    // Entrar em modo IRQ


    // Retornar para modo usuário
    bl main
@ hang: b hang

.globl PUT32
PUT32:
    str r1,[r0]
    bx lr

.globl GET32
GET32:
    ldr r0,[r0]
    bx lr

.globl dummy
dummy:
    bx lr

.globl io_halt
io_halt:
    wfi
    bx lr

.globl GETPC
GETPC:
    mov r0,lr
    bx lr

irq:
    push {r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,r12,lr}
    bl exc_handler
    pop  {r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,r12,lr}
    subs pc,lr,#4

.globl showcpu0
showcpu0:
    mrc p15, 0, r0, c0, c0, 5
    bx lr

.global _get_stack_pointer
_get_stack_pointer:
  mov r0, r13
  bx lr

.global _context_switch
_context_switch:
  // same as stmfd/stmdb !r13, {...}
  push {r0-r12,r14}
  str sp, [r0]
  ldr sp, [r1]
  // mov sp, r1
  // same as ldmfd/ldmia !r13, {...}
  pop {r0-r12}
  pop {pc} // pc points to the previous lr

.globl enable_irq
enable_irq:
    cpsie i
    bx lr

.globl disable_irq
disable_irq:
    cpsid i
    bx lr

.globl BRANCHTO
BRANCHTO:
    bx r0

.global _vectors
_vectors:
    b halt
    b halt
    b exc_handler    // SVC Handler
    b halt
    b halt
    NOP                    // Reserved vector
    b irq    // IRQ Handler
    b halt    // FIQ Handler

;@-------------------------------------------------------------------------
;@-------------------------------------------------------------------------


;@-------------------------------------------------------------------------
;@
;@ Copyright (c) 2012 David Welch dwelch@dwelch.com
;@
;@ Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
;@
;@ The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
;@
;@ THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
;@
;@-------------------------------------------------------------------------

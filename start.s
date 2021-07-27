
;@-------------------------------------------------------------------------
;@-------------------------------------------------------------------------

.globl _start
_start:
    bl boot

hang:
    wfi
    b hang

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
    push {r0-r12}
    push {lr}
    bl exc_handler
    pop {lr}
    pop {r0-r12}
    subs pc,lr,#4
    // push {lr,r12,r11,r10,r9,r8,r7,r6,r5,r4,r3,r2,r1,r0}
    // stmfd sp!, {r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,r12,lr}
    // stmia sp!, {lr,r12,r11,r10,r9,r8,r7,r6,r5,r4,r3,r2,r1,r0}
    // ldmia sp!, {lr,r12,r11,r10,r9,r8,r7,r6,r5,r4,r3,r2,r1,r0}^
    // ldmfd sp!, {r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,r12,lr}^

.globl showcpu0
showcpu0:
    mrc p15, 0, r0, c0, c0, 5
    bx lr

.global _get_stack_pointer
_get_stack_pointer:
  mov r0, r13
  bx lr


    @ pop {r0-r12}
    @ msr cpsr_c, #0x1F           // back to sys mode
    @ push {r0-r12}               // push to sp_usr
    @ msr cpsr_c, #0x12           // back to irq mode
    @ sub r0, r0, #4              // ajust return address of lr
    @ mrs r2, spsr                // get cpsr_usr (spsr_irq)
    @ msr cpsr_c, #0x1F           // back to sys mode
    @ mrc p15, 0, r1, c2, 0, 0    // get ttbr0
    @ push {r0-r2,lr}             // push to sp_usr
    @ push {r0-r12, lr}           // push to sp_usr

@ .global _context_switch
@ _context_switch:
@     @ push {r0-r12, lr}
@     str sp, [r0]
@     ldr sp, [r1]
@     pop {r0-r12}
@     pop {pc}
    // push {r0-r12,r14}
    // str sp, [r0]
    // ldr sp, [r1]
    // // mov sp, r1
    // // same as ldmfd/ldmia !r13, {...}
    // pop {r0-r12,r14}
    // bx lr
    // pop {pc} // pc points to the previous lr

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

halt1:
    b halt1

halt2:
    b halt2

halt3:
    b halt3

halt4:
    subs pc,lr,#4

halt5:
    subs pc,lr,#4

halt6:
    b halt6

.global _vectors
_vectors:
    b halt1         // Reset
    b halt2         // Undefined instruction
    b halt3         // SVC Handler
    b halt4         // Prefetch abort
    b halt5         // Data abort
    NOP             // Reserved vector
    b irq           // IRQ Handler
    b halt6         // FIQ Handler


boot:
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
    .equ MODE_USR, 0x10
    .equ IRQ_BIT,  0x80
    .equ FIQ_BIT,  0x40

    // Inicializa MMU
    bl mmu_init

    // save CPSR.
    mrs r0, cpsr

    // Entrar em modo IRQ
    @ bic r1, r0, #0x1F
    @ orr r2, r1, #0x10
    @ orr r1, r1, #0x12
    msr cpsr_c, #MODE_IRQ | IRQ_BIT | FIQ_BIT
    @ bl init_irq_stack
    mov sp, #0x4000
    isb

    // Entrar em modo SVC
    msr cpsr_c, #MODE_SVC | IRQ_BIT | FIQ_BIT
    @ bl init_svc_stack
    mov sp, #0x8000
    isb

    bl uart_init
    bl init_timer

    // Entrar em modo User
    msr cpsr_c, #MODE_USR
    isb
    bl init_thread

    ldr r3, =main
    blx r3
    bl hang

.global _before_context_switch
_before_context_switch:
    @ pop {r0-r12, lr}            // pop from sp_irq
    @ sub lr, lr, #4              // ajust return address
    pop {r0}                    // pop lr onto r0
    mrs r1, spsr                // get cpsr_usr (spsr_irq)
    msr cpsr_c, #0x1F           // back to sys mode
    mrc p15, 0, r2, c2, c0, 0    // get ttbr0
    push {r0}
    push {r1}
    push {r2}
    msr cpsr_c, #0x12           // back to irq mode
    pop {r0-r12}
    msr cpsr_c, #0x1F           // back to sys mode
    push {r0-r12,lr}
    b schedule

.global _after_context_switch
_after_context_switch:
    str sp, [r0]
    ldr sp, [r1]
    ldr r2, [sp, #56]
    ldr r1, [sp, #60]
    @ ldr r0, [sp, #64]
    mcr p15, 0, r2, c2, c0, 0 // load ttbr0
    msr cpsr, r1             // back to usr mode
    pop {r0-r12,lr}
    add sp, sp, #8
    pop {pc}

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

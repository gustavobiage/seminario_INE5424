.globl _start
_start:
    bl begin

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

.globl showcpu0
showcpu0:
    mrc p15, 0, r0, c0, c0, 5
    bx lr

.global _get_stack_pointer
_get_stack_pointer:
  mov r0, r13
  bx lr

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

reset_handler:
    b reset_handler

undefined_handler:
    b undefined_handler

svc_handler:
    b svc_handler

prefetch_abort_handler:
    subs pc,lr,#4

data_abort_handler:
    subs pc,lr,#4

fiq_handler:
    b fiq_handler

irq_handler:
    push {r0-r12}                    // save r0-r12
    push {lr}                        // save lr
    bl exc_handler                   // branch to exc_handler
    pop {lr}                         // restore lr
    pop {r0-r12}                     // restore r0-r12
    subs pc, lr, #4                  // ajust pc and return

.global _vectors
_vectors:
    b reset_handler                  // Reset
    b undefined_handler              // Undefined instruction
    b svc_handler                    // SVC Handler
    b prefetch_abort_handler         // Prefetch abort
    b data_abort_handler             // Data abort
    nop                              // Reserved vector
    b irq_handler                    // IRQ Handler
    b fiq_handler                    // FIQ Handler

begin:
    .equ MODE_IRQ, 0x12
    .equ MODE_SVC, 0x13
    .equ MODE_USR, 0x10
    .equ IRQ_BIT,  0x80
    .equ FIQ_BIT,  0x40

    // read cpu id and stop slave cores
    mrc p15, #0, r1, c0, c0, #5
    and r1, r1, #3
    cmp r1, #0
    bne hang

    // set vector table handlers
    ldr r0, =_vectors
    mcr P15, 0, r0, c12, c0, 0

    // initialize MMU
    bl mmu_init

    // initialize IRQ stack
    msr cpsr_c, #MODE_IRQ | IRQ_BIT | FIQ_BIT
    bl init_irq_stack

    // initialize SVC stack
    msr cpsr_c, #MODE_SVC | IRQ_BIT | FIQ_BIT
    bl init_svc_stack

    bl uart_init
    bl init_timer

    // initialize User stack
    msr cpsr_c, #MODE_USR
    bl init_task

    ldr r3, =main
    blx r3
    bl hang

.global _before_context_switch
_before_context_switch:
    pop {r0}                     // pop lr onto r0
    mrs r1, spsr                 // save cpsr_usr (spsr_irq) to r1
    msr cpsr_c, #0x1F            // move to System Mode
    mrc p15, 0, r2, c2, c0, 0    // save ttbr0 to r2
    push {r0}                    // save lr
    push {r1}                    // save spsr_irq
    push {r2}                    // save ttbr0
    msr cpsr_c, #0x12            // move to IRQ Mode
    pop {r0-r12}                 // pop r0-r12
    msr cpsr_c, #0x1F            // move to System Mode
    push {r0-r12,lr}             // save r0-r12
    b schedule                   // go to "schedule"

.global _after_context_switch
_after_context_switch:
    str sp, [r0]                 // store sp into PCB of old process
    ldr sp, [r1]                 // load  sp from PCB of new process
    ldr r2, [sp, #56]            // load ttbr0 that was saved previously
    ldr r1, [sp, #60]            // load spsr_irq that was saved previously
    mcr p15, 0, r2, c2, c0, 0    // write ttbr0
    msr cpsr, r1                 // move to User Mode
    pop {r0-r12,lr}              // pop r0-r12 and lr
    add sp, sp, #8               // adjust stack pointer
    pop {pc}                     // pop pc


//-------------------------------------------------------------------------
//-------------------------------------------------------------------------

// 2  outer corner
// 4
// 6
// 8  TX out
// 10 RX in

extern void PUT32 ( unsigned int, unsigned int );
extern void PUT16 ( unsigned int, unsigned int );
extern void PUT8 ( unsigned int, unsigned int );
extern unsigned int GET32 ( unsigned int );
extern unsigned int GETPC ( void );
extern void dummy ( unsigned int );
extern unsigned int BRANCHTO ( unsigned int );

extern void uart_init ( void );
extern unsigned int uart_lcr ( void );
extern void uart_flush ( void );
extern void uart_send ( unsigned int );
extern unsigned int uart_recv ( void );
extern unsigned int uart_check ( void );
extern void hexstring ( unsigned int );
extern void hexstrings ( unsigned int );
extern void timer_init ( void );
extern unsigned int timer_tick ( void );

extern void timer_init ( void );
extern unsigned int timer_tick ( void );

extern unsigned int showcpu0 ( void );
// extern void mmu_init ( void );

extern void _context_switch(volatile unsigned int **from_sp, volatile unsigned int **to_sp);
extern unsigned int* _get_stack_pointer(void);

#define MAX_STACK 256
#define MAX_THREADS 3

typedef struct stack {
    volatile unsigned int *stack;
    unsigned int stack_base[MAX_STACK];
} thread_t;

typedef struct {
    unsigned int length;
    unsigned int current_id;
    thread_t thread[MAX_THREADS];
} scheduler_t;

scheduler_t scheduler;
struct stack irq_stack;
struct stack svc_stack;
struct stack fiq_stack;

void halt() { while(1); };

void set_stack(volatile unsigned int * stack) {
    __asm__("mov sp, %0" : : "r"(stack): );
}

void init_irq_stack() {
    int mode = 0;
    __asm__("mrs %0, CPSR  \n"
            "and %0, #0xff \n"
            "cmp %0, #0xd2 \n" // O modo atual do processador é IRQ?
            "bne halt      \n" : : "r"(mode): );
    irq_stack.stack = irq_stack.stack_base + (MAX_STACK - 1);
    set_stack(irq_stack.stack);
}

void init_fiq_stack() {
    int mode = 0;
    __asm__("mrs %0, CPSR  \n"
            "and %0, #0xff \n"
            "cmp %0, #0xd1 \n" // O modo atual do processador é FIQ?
            "bne halt      \n" : : "r"(mode): );
    fiq_stack.stack = fiq_stack.stack_base + (MAX_STACK - 1);
    set_stack(fiq_stack.stack);
}

void init_svc_stack() {
    int mode = 0;
    __asm__("mrs %0, CPSR  \n"
            "and %0, #0xff \n"
            "cmp %0, #0xd3 \n" // O modo atual do processador é SVC?
            "bne halt      \n" : : "r"(mode): );
    svc_stack.stack = irq_stack.stack_base + (MAX_STACK - 1);
    set_stack(svc_stack.stack);
}

// void init

void init_thread() {
    const int main_id = 0;
    scheduler.length = 1;
    scheduler.current_id = main_id;
    scheduler.thread[main_id].stack = scheduler.thread[main_id].stack_base + (MAX_STACK - 1);
    set_stack(scheduler.thread[main_id].stack);
}

void create_thread(void *thread_entry) {
    unsigned int id = scheduler.length;
    scheduler.thread[id].stack = scheduler.thread[id].stack_base + (MAX_STACK - 1);

    // *scheduler.thread[id].stack-- = 0x00000093; // cpsr =IRQ enabled/FIQ & Thumb
                                                // disabled/Processor mode = SVC
    // *scheduler.thread[id].stack-- = (unsigned int) thread_entry;  // r15: pc
    *scheduler.thread[id].stack-- = (unsigned int) thread_entry;       // r14: lr
    *scheduler.thread[id].stack-- = 0;                  // r12
    *scheduler.thread[id].stack-- = 0;                  // r11
    *scheduler.thread[id].stack-- = 0;                  // r10
    *scheduler.thread[id].stack-- = 0;                  // r9
    *scheduler.thread[id].stack-- = 0;                  // r8
    *scheduler.thread[id].stack-- = 0;                  // r7
    *scheduler.thread[id].stack-- = 0;                  // r6
    *scheduler.thread[id].stack-- = 0;                  // r5
    *scheduler.thread[id].stack-- = 0;                  // r4
    *scheduler.thread[id].stack-- = 0;                  // r3
    *scheduler.thread[id].stack-- = 0;                  // r2
    *scheduler.thread[id].stack-- = 0;                  // r1
    *scheduler.thread[id].stack = 0;                    // r0
    // don't do stack--!

    scheduler.length++;
}

void context_switch() {
    int current_id = scheduler.current_id;
    int next_id = current_id + 1;

    if (next_id >= scheduler.length) {
        next_id = 0;
    }

    // get current sp
    scheduler.thread[current_id].stack = _get_stack_pointer();

    // do context switch
    scheduler.current_id = next_id;

    // hexstring((unsigned int) current_id);
    // hexstring((unsigned int) next_id);

    _context_switch(&scheduler.thread[current_id].stack,
                    &scheduler.thread[next_id].stack);
}

int task1( void ) {
    int n = 10;
    while (n--) {
        hexstring(1);
        context_switch();
    }
    return(0);
}

int task2( void ) {
    int n = 10;
    while(n--) {
        hexstring(2);
        context_switch();
    }
    return(0);
}

//------------------------------------------------------------------------
int main ( void )
{
    // mmu_init();
    uart_init();

    init_thread();
    create_thread(task1);
    create_thread(task2);

    int n = 10;
    while(n--) {
        hexstring(0);
        context_switch();
    }

    return(0);
}
//-------------------------------------------------------------------------
//-------------------------------------------------------------------------


//-------------------------------------------------------------------------
//
// Copyright (c) 2015 David Welch dwelch@dwelch.com
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
//-------------------------------------------------------------------------

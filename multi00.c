#define MAX_STACK 1024
#define MAX_THREADS 3

extern void io_halt();
extern void hexstring(unsigned int);
extern unsigned int build_page_table();
extern unsigned int* _get_stack_pointer();
extern void _after_context_switch(volatile unsigned int **from_sp, volatile unsigned int **to_sp);

typedef struct stack {
    volatile unsigned int *stack;
    unsigned int stack_base[MAX_STACK];
} thread_t;

typedef struct {
    unsigned int length;
    volatile unsigned int current_id;
    thread_t thread[MAX_THREADS];
} scheduler_t;

scheduler_t scheduler;
struct stack irq_stack;
struct stack svc_stack;

void halt() { while(1); };

inline void set_stack(volatile unsigned int * stack) {
    __asm__("mov sp, %0" : : "r"(stack): );
}

void init_irq_stack() {
    irq_stack.stack = irq_stack.stack_base + (MAX_STACK - 1);
    __asm__("mov sp, %0" : : "r"(irq_stack.stack): );
}

void init_svc_stack() {
    svc_stack.stack = svc_stack.stack_base + (MAX_STACK - 1);
    __asm__("mov sp, %0" : : "r"(svc_stack.stack): );
}

void init_thread() {
    const int main_id = 0;
    scheduler.length = 1;
    scheduler.current_id = main_id;
    scheduler.thread[main_id].stack = scheduler.thread[main_id].stack_base + (MAX_STACK - 1);
    __asm__("mov sp, %0" : : "r"(scheduler.thread[main_id].stack): );
}

void create_thread(void *thread_entry) {
    unsigned int id = scheduler.length;
    scheduler.thread[id].stack = scheduler.thread[id].stack_base + (MAX_STACK - 1);
    *scheduler.thread[id].stack-- = (unsigned int) thread_entry;       // r15: pc
    *scheduler.thread[id].stack-- = 0x60000110;                        // cpsr
    *scheduler.thread[id].stack-- = build_page_table();                // ttbr0
    *scheduler.thread[id].stack-- = 0;                                 // r14: lr
    *scheduler.thread[id].stack-- = 0;                                 // r12
    *scheduler.thread[id].stack-- = 0;                                 // r11
    *scheduler.thread[id].stack-- = 0;                                 // r10
    *scheduler.thread[id].stack-- = 0;                                 // r9
    *scheduler.thread[id].stack-- = 0;                                 // r8
    *scheduler.thread[id].stack-- = 0;                                 // r7
    *scheduler.thread[id].stack-- = 0;                                 // r6
    *scheduler.thread[id].stack-- = 0;                                 // r5
    *scheduler.thread[id].stack-- = 0;                                 // r4
    *scheduler.thread[id].stack-- = 0;                                 // r3
    *scheduler.thread[id].stack-- = 0;                                 // r2
    *scheduler.thread[id].stack-- = 0;                                 // r1
    *scheduler.thread[id].stack = 0;                                   // r0
    scheduler.length++;
}

void schedule() {
    int current_id = scheduler.current_id;
    int next_id = current_id + 1;

    if (next_id >= scheduler.length) {
        next_id = 0;
    }

    // get current sp
    scheduler.thread[current_id].stack = _get_stack_pointer();

    // do context switch
    scheduler.current_id = next_id;

    int ttbr0 = *(scheduler.thread[next_id].stack + 14);
    hexstring(ttbr0);
    _after_context_switch(&scheduler.thread[current_id].stack,
                          &scheduler.thread[next_id].stack);
}

int task1( void ) {
    while (1) {
        hexstring(1);
        io_halt();
    }
}

int task2( void ) {
    while (1) {
        hexstring(2);
        io_halt();
    }
}

int main ( void )
{
    create_thread(task1);
    create_thread(task2);
    while (1) {
        hexstring(0);
        io_halt();
    }
    return(0);
}
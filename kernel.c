#define MAX_TASKS 3
#define MAX_STACK 1024

extern void io_halt();
extern void hexstring(unsigned int);
extern unsigned int build_page_table();
extern unsigned int* _get_stack_pointer();
extern void _after_context_switch(volatile unsigned int **from_sp, volatile unsigned int **to_sp);

typedef struct stack {
    volatile unsigned int *stack;
    unsigned int stack_base[MAX_STACK];
} task_t;

typedef struct {
    unsigned int length;
    volatile unsigned int current_id;
    task_t task[MAX_TASKS];
} scheduler_t;

scheduler_t scheduler;
struct stack irq_stack;
struct stack svc_stack;

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

void init_task() {
    const int main_id = 0;
    scheduler.length = 1;
    scheduler.current_id = main_id;
    scheduler.task[main_id].stack = scheduler.task[main_id].stack_base + (MAX_STACK - 1);
    __asm__("mov sp, %0" : : "r"(scheduler.task[main_id].stack): );
}

void create_task(void *task_entry) {
    unsigned int id = scheduler.length;
    scheduler.task[id].stack = scheduler.task[id].stack_base + (MAX_STACK - 1);
    *scheduler.task[id].stack-- = (unsigned int) task_entry;       // r15: pc
    *scheduler.task[id].stack-- = 0x60000110;                        // cpsr
    *scheduler.task[id].stack-- = build_page_table();                // ttbr0
    *scheduler.task[id].stack-- = 0;                                 // r14: lr
    *scheduler.task[id].stack-- = 0;                                 // r12
    *scheduler.task[id].stack-- = 0;                                 // r11
    *scheduler.task[id].stack-- = 0;                                 // r10
    *scheduler.task[id].stack-- = 0;                                 // r9
    *scheduler.task[id].stack-- = 0;                                 // r8
    *scheduler.task[id].stack-- = 0;                                 // r7
    *scheduler.task[id].stack-- = 0;                                 // r6
    *scheduler.task[id].stack-- = 0;                                 // r5
    *scheduler.task[id].stack-- = 0;                                 // r4
    *scheduler.task[id].stack-- = 0;                                 // r3
    *scheduler.task[id].stack-- = 0;                                 // r2
    *scheduler.task[id].stack-- = 0;                                 // r1
    *scheduler.task[id].stack = 0;                                   // r0
    scheduler.length++;
}

void schedule() {
    int current_id = scheduler.current_id;
    int next_id = current_id + 1;

    if (next_id >= scheduler.length) {
        next_id = 0;
    }

    // get current sp
    scheduler.task[current_id].stack = _get_stack_pointer();

    // print ttbr0 of next task
    int ttbr0 = *(scheduler.task[next_id].stack + 14);
    hexstring(ttbr0);

    // do context switch
    scheduler.current_id = next_id;
    _after_context_switch(&scheduler.task[current_id].stack,
                          &scheduler.task[next_id].stack);
}

int task1() {
    while (1) {
        hexstring(1);
        io_halt();
    }
    return 0;
}

int task2() {
    while (1) {
        hexstring(2);
        io_halt();
    }
    return 0;
}

int main() {
    create_task(task1);
    create_task(task2);
    while (1) {
        hexstring(0);
        io_halt();
    }
    return 0;
}
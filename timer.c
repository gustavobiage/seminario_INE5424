#define CORE0_TIMER_IRQCNTL     0x40000040
#define CORE0_IRQ_SOURCE        0x40000060

extern void enable_irq(void);
extern void disable_irq(void);
extern void io_halt(void);
extern void schedule();

void routing_core0cntv_to_core0irq(void)
{
    (*(volatile unsigned int*) CORE0_TIMER_IRQCNTL) = 0x08;
}

unsigned int read_core0timer_pending(void)
{
    unsigned int tmp;
    tmp = (*(volatile unsigned int*) CORE0_IRQ_SOURCE);
    return tmp;
}

static unsigned int cntfrq = 0;

void enable_cntv(void)
{
    unsigned int cntv_ctl;
    cntv_ctl = 1;
	asm volatile ("mcr p15, 0, %0, c14, c3, 1" :: "r"(cntv_ctl) ); // write CNTV_CTL
}

void disable_cntv(void)
{
    unsigned int cntv_ctl;
    cntv_ctl = 0;
	asm volatile ("mcr p15, 0, %0, c14, c3, 1" :: "r"(cntv_ctl) ); // write CNTV_CTL
}

unsigned int read_cntv_tval(void)
{
    unsigned int val;
	asm volatile ("mrc p15, 0, %0, c14, c3, 0" : "=r"(val) );
    return val;
}

inline void write_cntv_tval(unsigned int val)
{
	asm volatile ("mcr p15, 0, %0, c14, c3, 0" :: "r"(val) );
    return;
}

unsigned int read_cntfrq(void)
{
    unsigned int val;
	asm volatile ("mrc p15, 0, %0, c14, c0, 0" : "=r"(val) );
    return val;
}

void exc_handler(void)
{
    if (read_core0timer_pending() & 0x08) {
        write_cntv_tval(cntfrq);               // clear CNTV interrupt and set next 1sec timer
        __asm__("b _before_context_switch");   // go to context_switch
    }
}

void init_timer(void)
{
    cntfrq = read_cntfrq();              // read CNTFRQ
    write_cntv_tval(cntfrq);             // clear CNTV interrupt and set next 1 sec timer.
    routing_core0cntv_to_core0irq();
    enable_cntv();
    enable_irq();
}

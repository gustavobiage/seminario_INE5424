#include <stddef.h>
#include <stdint.h>


extern void hexstring(unsigned int);

#define uint32_t unsigned int

extern void enable_irq(void);
extern void disable_irq(void);
extern void io_halt(void);
extern void context_switch();

// Memory-Mapped I/O output
static inline void mmio_write(uint32_t reg, uint32_t data)
{
    *(volatile uint32_t*)reg = data;
}

// Memory-Mapped I/O input
static inline uint32_t mmio_read(uint32_t reg)
{
    return *(volatile uint32_t*)reg;
}

#define CORE0_TIMER_IRQCNTL 0x40000040
#define CORE0_IRQ_SOURCE 0x40000060

void routing_core0cntv_to_core0irq(void)
{
    mmio_write(CORE0_TIMER_IRQCNTL, 0x08);
}

uint32_t read_core0timer_pending(void)
{
    uint32_t tmp;
    tmp = mmio_read(CORE0_IRQ_SOURCE);
    return tmp;
}

static uint32_t cntfrq = 0;

void enable_cntv(void)
{
    uint32_t cntv_ctl;
    cntv_ctl = 1;
	asm volatile ("mcr p15, 0, %0, c14, c3, 1" :: "r"(cntv_ctl) ); // write CNTV_CTL
}

void disable_cntv(void)
{
    uint32_t cntv_ctl;
    cntv_ctl = 0;
	asm volatile ("mcr p15, 0, %0, c14, c3, 1" :: "r"(cntv_ctl) ); // write CNTV_CTL
}

uint64_t read_cntvct(void)
{
	uint64_t val;
	asm volatile("mrrc p15, 1, %Q0, %R0, c14" : "=r" (val));
	return (val);
}

uint64_t read_cntvoff(void)
{
	uint64_t val;
    asm volatile("mrrc p15, 4, %Q0, %R0, c14" : "=r" (val));
	return (val);
}

uint32_t read_cntv_tval(void)
{
    uint32_t val;
	asm volatile ("mrc p15, 0, %0, c14, c3, 0" : "=r"(val) );
    return val;
}

void write_cntv_tval(uint32_t val)
{
	asm volatile ("mcr p15, 0, %0, c14, c3, 0" :: "r"(val) );
    return;
}

uint32_t read_cntfrq(void)
{
    uint32_t val;
	asm volatile ("mrc p15, 0, %0, c14, c0, 0" : "=r"(val) );
    return val;
}


void exc_handler(void)
{
    hexstring(555);
    if (read_core0timer_pending() & 0x08 ) {
        write_cntv_tval(cntfrq);    // clear cntv interrupt and set next 1sec timer.

        context_switch();
        hexstring(555);
        // uart_puts("core0timer_pendig : ");
        // uart_hex_puts(read_core0timer_pending());
        // uart_puts("handler CNTV_TVAL : ");
        // uart_hex_puts(read_cntv_tval());
        // uart_puts("handler CNTVCT    : ");
        // uart_hex_puts( (uint32_t) read_cntvct() & 0xFFFFFFFF);
    }
    return;
}

void kernel_main(void)
{
    // uint32_t val;

    // uart_puts("timer01 : arm generic timer every 1 sec.\n");
    // uart_puts("exit : Ctrl-A x ,monitor : Ctrl-A c\n\n");

    // uart_puts("CNTFRQ  : ");
    cntfrq = read_cntfrq();
    // uart_hex_puts(cntfrq);

    write_cntv_tval(cntfrq);    // clear cntv interrupt and set next 1 sec timer.
    // uart_puts("CNTV_TVAL: ");
    // val = read_cntv_tval();
    // uart_hex_puts(val);

    routing_core0cntv_to_core0irq();
    enable_cntv();
    enable_irq();

    while (1) {
        hexstring(3);
        io_halt();
    }
}

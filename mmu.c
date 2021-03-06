
enum {
    PAGE_TABLES                = 0x3eef0000,
    TTBCR_DOMAIN               = 0xffffffff,
    TTB_MEMORY_DESCRIPTOR      = 0x90c0e,
    TTB_DEVICE_DESCRIPTOR      = 0x90c0a,
    TTB_PERIPHERAL_DESCRIPTOR  = 0x90c16,
    VECTOR_TABLE               = 0x00008000,
    FLAG_SET_REG               = 0x40000000
};

unsigned int* page_tables = (unsigned int*) 0x3eef0000;

inline void branch_prediction_enable() {
    __asm__("                                                                       \t\n\
        mrc     p15, 0, r0, c1, c0, 0                  // Read SCTLR                \t\n\
        orr     r0, r0, #(1 << 11)                     // Set the Z bit (bit 11)    \t\n\
        mcr     p15, 0,r0, c1, c0, 0                   // Write SCTLR               \t\n\
        ");
}

inline void enable_dside_prefetch() {
    __asm__("mrc p15, 0, r0, c1, c0, 1 \t\n\
         orr r0, r0, #(0x1 << 2)   \t\n\
         mcr p15, 0, r0, c1, c0, 1 \t\n\
        ");
}

inline void invalidate_tlb() {
    __asm__("mov r0, #0x0 \t\n mcr p15, 0, r0, c8, c7, 0"); // TLBIALL - Invalidate entire Unifed TLB
}

inline void clear_branch_prediction_array() {
    __asm__("mov r0, #0x0 \t\n mcr p15, 0, r0, c7, c5, 6"); // BPIALL - Invalidate entire branch predictor array
}

inline void set_domain_access() {
    __asm__("mcr p15, 0, %0, c3, c0, 0" : : "p"(TTBCR_DOMAIN) :);
}

inline void page_tables_setup(unsigned int* page_tables) {
    unsigned int aux = 0x0;
    for (int curr_page = 1006; curr_page >= 0; curr_page--) {
        aux = TTB_MEMORY_DESCRIPTOR | (curr_page << 20);
        ((unsigned int*) page_tables)[curr_page] = aux;
    }
    aux = TTB_DEVICE_DESCRIPTOR | (1007 << 20);
    ((unsigned int*) page_tables)[1007] = aux;
    for (int curr_page = 4095; curr_page > 1007; curr_page--) {
        aux = TTB_PERIPHERAL_DESCRIPTOR | (curr_page << 20);
        ((unsigned int*) page_tables)[curr_page] = aux;
    }
}

unsigned int build_page_table() {
    unsigned int base = (unsigned int) page_tables;
    page_tables_setup(page_tables);
    page_tables -= 8 << 12;
    return base;
}

inline void enable_mmu() {
    // TTB0 size is 16 kb, there is no TTB1 and no TTBCR
    // ARMv7 Architecture Reference Manual, pages 1330
    __asm__ ("mov r0, #0x0 \t\n mcr p15, 0, r0, c2, c0, 2"); // Write Translation Table Base Control Register.
    __asm__ ("mcr p15, 0, %0, c2, c0, 0" : : "p"(PAGE_TABLES) :); // Write Translation Table Base Register 0.

    // Enable MMU
    //-------------
    //0     - M, set to enable MMU
    // Leaving the caches disabled until after scatter loading.
    __asm__ ("                                                                  \t\n\
        mrc     p15, 0, r0, c1, c0, 1       // Read ACTLR                       \t\n\
        orr     r0, r0, #(0x01 << 6)        // Set SMP bit                      \t\n\
        mcr     p15, 0, r0, c1, c0, 1       // Write ACTLR                      \t\n\
        mrc     p15, 0, r0, c1, c0, 0       // Read current control reg         \t\n\
        orr     r0, r0, #(0x1 << 2)         // The C bit (data cache).          \t\n\
        bic     r0, r0, #(0x1 << 29)        // Set AFE to 0 disable Access Flag.\t\n\
        orr     r0, r0, #(0x1 << 12)        // The I bit (instruction cache).   \t\n\
        orr     r0, r0, #0x01               // Set M bit                        \t\n\
        mcr     p15, 0, r0, c1, c0, 0       // Write reg back                   \t\n\
        ");
}

inline void clear_bss() {
    unsigned int bss_start, bss_end;
    __asm__("ldr %0, =__bss_start__" : "=r"(bss_start) :);
    __asm__("ldr %0, =__bss_end__" : "=r"(bss_end) :);
    unsigned int limit = (bss_end - bss_start)/4;
    for(unsigned int i = 0; i < limit; i++) {
        ((unsigned int*) bss_start)[i] = 0x0;
    }
}

// DSB causes completion of all cache maintenance operations appearing in program
// order before the DSB instruction.
inline void dsb()
{
    __asm__("dsb");
}

// An ISB instruction causes the effect of all branch predictor maintenance
// operations before the ISB instruction to be visible to all instructions
// after the ISB instruction.
inline void isb()
{
    __asm__("isb");
}

inline void invalidate_caches()
{
    __asm__("                                                                           \t\n\
    // Disable L1 Caches.                                                               \t\n\
    mrc     p15, 0, r1, c1, c0, 0 // Read SCTLR.                                        \t\n\
    bic     r1, r1, #(0x1 << 2) // Disable D Cache.                                     \t\n\
    mcr     p15, 0, r1, c1, c0, 0 // Write SCTLR.                                       \t\n\
                                                                                        \t\n\
    // Invalidate Data cache to create general-purpose code. Calculate there            \t\n\
    // cache size first and loop through each set + way.                                \t\n\
    mov     r0, #0x0 // r0 = 0x0 for L1 dcache 0x2 for L2 dcache.                       \t\n\
    mcr     p15, 2, r0, c0, c0, 0 // CSSELR Cache Size Selection Register.              \t\n\
    mrc     p15, 1, r4, c0, c0, 0 // CCSIDR read Cache Size.                            \t\n\
    and     r1, r4, #0x7                                                                \t\n\
    add     r1, r1, #0x4 // r1 = Cache Line Size.                                       \t\n\
    ldr     r3, =0x7fff                                                                 \t\n\
    and     r2, r3, r4, lsr #13 // r2 = Cache Set Number ??? 1.                           \t\n\
    ldr     r3, =0x3ff                                                                  \t\n\
    and     r3, r3, r4, lsr #3 // r3 = Cache Associativity Number ??? 1.                  \t\n\
    clz     r4, r3 // r4 = way position in CISW instruction.                            \t\n\
    mov     r5, #0 // r5 = way loop counter.                                            \t\n\
way_loop:                                                                               \t\n\
    mov     r6, #0 // r6 = set loop counter.                                            \t\n\
set_loop:                                                                               \t\n\
    orr     r7, r0, r5, lsl r4 // Set way.                                              \t\n\
    orr     r7, r7, r6, lsl r1 // Set set.                                              \t\n\
    mcr     p15, 0, r7, c7, c6, 2 // DCCISW r7.                                         \t\n\
    add     r6, r6, #1 // Increment set counter.                                        \t\n\
    cmp     r6, r2 // Last set reached yet?                                             \t\n\
    ble     set_loop // If not, iterate set_loop,                                       \t\n\
    add     r5, r5, #1 // else, next way.                                               \t\n\
    cmp     r5, r3 // Last way reached yet?                                             \t\n\
    ble     way_loop // if not, iterate way_loop.                                       \t\n\
    // mov r2, #0                                                                       \t\n\
    // mcr p15, 0, r2, c7, c7, 0                                                        \t\n\
    ");
}

void mmu_init() {
    invalidate_caches();
    clear_branch_prediction_array();
    invalidate_tlb();
    enable_dside_prefetch();
    set_domain_access();
    dsb();
    isb();

    page_tables = (unsigned int*) PAGE_TABLES;
    page_tables_setup(page_tables);
    page_tables -= 8 << 12;

    enable_mmu();
    dsb();
    isb();
    branch_prediction_enable();
    dsb();
    clear_bss();
}

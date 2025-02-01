/* See COPYRIGHT for copyright information. */

#include <inc/assert.h>
#include <inc/cons_color.h>
#include <inc/stdio.h>
#include <inc/string.h>
#include <kern/console.h>
#include <kern/kclock.h>
#include <kern/monitor.h>
#include <kern/pmap.h>

void i386_init(void)
{
    extern char edata[], end[];

    // Before doing anything else, complete the ELF loading process.
    // Clear the uninitialized global data (BSS) section of our program.
    // This ensures that all static/global variables start out zero.
    memset(edata, 0, end - edata);

    // Initialize the console.
    // Can't call cprintf until after we do this!
    cons_init();

    cprintf(ANSI_FG_MAGENTA ANSI_BG_WHITE "\n------------------------ FORMATTED TEST "
                                          "-------------------\n" ANSI_NONE);
    cprintf("\n6828 decimal is %o octal!\n", 6828);

    // Do Lab1 Exercise 8.
    int x = 1, y = 3, z = 4;
    cprintf("Numbers x=%d y=%d z=%d\n", x, y, z);

    unsigned int i = 0x00646c72;
    cprintf("Print i: H%x Wo%s\n", 57616, &i);

    cprintf("Test: x=%d, y=%d \n", 3);

    cprintf(ANSI_FG_MAGENTA ANSI_BG_WHITE "\n------------------------ Page Memory Init "
                                          "-------------------\n" ANSI_NONE);
    // Lab 2 memory management initialization functions
    mem_init();

    // Drop into the kernel monitor.
    while (1)
        monitor(NULL);
}

/*
 * Variable panicstr contains argument to first call to panic; used as flag
 * to indicate that the kernel has already called panic.
 */
const char *panicstr;

/*
 * Panic is called on unresolvable fatal errors.
 * It prints "panic: mesg", and then enters the kernel monitor.
 */
void        _panic(const char *file, int line, const char *fmt, ...)
{
    va_list ap;

    if (panicstr)
        goto dead;
    panicstr = fmt;

    // Be extra sure that the machine is in as reasonable state
    asm volatile("cli; cld");

    va_start(ap, fmt);
    cprintf("kernel panic at %s:%d: ", file, line);
    vcprintf(fmt, ap);
    cprintf("\n");
    va_end(ap);

dead:
    /* break into the kernel monitor */
    while (1)
        monitor(NULL);
}

/* like panic, but don't */
void _warn(const char *file, int line, const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    cprintf("kernel warning at %s:%d: ", file, line);
    vcprintf(fmt, ap);
    cprintf("\n");
    va_end(ap);
}

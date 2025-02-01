// Simple command-line kernel monitor useful for
// controlling the kernel and exploring the system interactively.

#include <inc/assert.h>
#include <inc/memlayout.h>
#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/x86.h>
#include <kern/console.h>
#include <kern/kdebug.h>
#include <kern/monitor.h>

#define CMDBUF_SIZE 80  // enough for one VGA text line

struct Command
{
    const char *name;
    const char *desc;
    // return -1 to force monitor to exit
    int (*func)(int argc, char **argv, struct Trapframe *tf);
};

static struct Command commands[] = {
    {"help", "Display this list of commands", mon_help},
    {"kerninfo", "Display information about the kernel", mon_kerninfo},
    {"backtrace", "Display current all stack backtrace and register values", mon_backtrace}
};

/***** Implementations of basic kernel monitor commands *****/

int mon_help(int argc, char **argv, struct Trapframe *tf)
{
    int i;

    for (i = 0; i < ARRAY_SIZE(commands); i++)
        cprintf("%s - %s\n", commands[i].name, commands[i].desc);
    return 0;
}

int mon_kerninfo(int argc, char **argv, struct Trapframe *tf)
{
    extern char _start[], entry[], etext[], edata[], end[];

    cprintf("Special kernel symbols:\n");
    cprintf("  _start                  %08x (phys)\n", _start);
    cprintf("  entry  %08x (virt)  %08x (phys)\n", entry, entry - KERNBASE);
    cprintf("  etext  %08x (virt)  %08x (phys)\n", etext, etext - KERNBASE);
    cprintf("  edata  %08x (virt)  %08x (phys)\n", edata, edata - KERNBASE);
    cprintf("  end    %08x (virt)  %08x (phys)\n", end, end - KERNBASE);
    cprintf("Kernel executable memory footprint: %dKB\n", ROUNDUP(end - entry, 1024) / 1024);
    return 0;
}

int mon_backtrace(int argc, char **argv, struct Trapframe *tf)
{
    // Your code here.
    volatile uint32_t cur_ebp = read_ebp();

    while (cur_ebp <= 0xf010afc8)  // the begining top EBP value.
    {
        cprintf(
            "ebp %08x eip %08x args %08x %08x %08x %08x %08x\n", cur_ebp, *(uint32_t *)(cur_ebp + 0x04),
            *(uint32_t *)(cur_ebp + 0x08), *(uint32_t *)(cur_ebp + 0x0c), *(uint32_t *)(cur_ebp + 0x10),
            *(uint32_t *)(cur_ebp + 0x14), *(uint32_t *)(cur_ebp + 0x18)
        );

        cur_ebp += 0x20;  // Every time call new function, the EBP always changed 32 bytes, also 8 32-bits words.

        /**
         * @brief call/ret address, sample:
        eSp:0xf010af70 eBp:0xf010af88
                call <function>

        eSp:0xf010af6c eBp:0xf010af88	call pushed the function ret address (a 32-bit word) into stack.
                push %eBp

        eSp:0xf010af68 eBp:0xf010af88
                movl %eSp, %eBp

        eSp:0xf010af68 eBp:0xf010af68

                  +------------------+
`ebp + 0x10` --> |  3rd argument     |
`ebp + 0xC`  --> |  2nd argument     | <-- Last function stack frame.
`ebp + 0x8`  --> |  1st argument     |
`call func`      +------------------+
`ebp + 0x4`  --> |  Return Address   | <-- `ret` will use this to return.
`ebp`        --> |  Old EBP          |
                  +------------------+
         *
         */
    }

    cur_ebp = 0xf010aff8;  // i386_init() ebp value.
    cprintf(
        "ebp %08x eip %08x args %08x %08x %08x %08x %08x\n", cur_ebp, *(uint32_t *)(cur_ebp + 0x04),
        *(uint32_t *)(cur_ebp - 0x04), *(uint32_t *)(cur_ebp - (0x08)), *(uint32_t *)(cur_ebp - (0x0c)),
        (*(uint32_t *)(cur_ebp - (0x10))), *(uint32_t *)(cur_ebp - (0x14))
    );
    return 0;
}

/***** Kernel monitor command interpreter *****/

#define WHITESPACE "\t\r\n "
#define MAXARGS    16

static int runcmd(char *buf, struct Trapframe *tf)
{
    int   argc;
    char *argv[MAXARGS];
    int   i;

    // Parse the command buffer into whitespace-separated arguments
    argc       = 0;
    argv[argc] = 0;
    while (1)
    {
        // gobble whitespace
        while (*buf && strchr(WHITESPACE, *buf))
            *buf++ = 0;
        if (*buf == 0)
            break;

        // save and scan past next arg
        if (argc == MAXARGS - 1)
        {
            cprintf("Too many arguments (max %d)\n", MAXARGS);
            return 0;
        }
        argv[argc++] = buf;
        while (*buf && !strchr(WHITESPACE, *buf))
            buf++;
    }
    argv[argc] = 0;

    // Lookup and invoke the command
    if (argc == 0)
        return 0;
    for (i = 0; i < ARRAY_SIZE(commands); i++)
    {
        if (strcmp(argv[0], commands[i].name) == 0)
            return commands[i].func(argc, argv, tf);
    }
    cprintf("Unknown command '%s'\n", argv[0]);
    return 0;
}

void monitor(struct Trapframe *tf)
{
    char *buf;

    cprintf("Welcome to the JOS kernel monitor!\n");
    cprintf("Type 'help' for a list of commands.\n");

    while (1)
    {
        buf = readline("K> ");
        if (buf != NULL)
            if (runcmd(buf, tf) < 0)
                break;
    }
}

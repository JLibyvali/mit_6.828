# Lab1 Notes

## Pre-required  
1GBit: 0x40000000(Hex)  
1MBit: 0x00100000(Hex)  
1KBit: 0x00000400(Hex)  

> 16-bit Intel 8088 hard-wired memory layout.   

```c  
+------------------+  <- 0xFFFFFFFF (4GB)
|      32-bit      |
|  memory mapped   |
|     devices      |
|                  |
/\/\/\/\/\/\/\/\/\/\

/\/\/\/\/\/\/\/\/\/\
|                  |
|      Unused      |
|                  |
+------------------+  <- depends on amount of RAM
|                  |
|                  |
| Extended Memory  |
|                  |
|                  |
+------------------+  <- 0x00100000 (1MB)
|     BIOS ROM     |
+------------------+  <- 0x000F0000 (960KB)
|  16-bit devices, |
|  expansion ROMs  |
+------------------+  <- 0x000C0000 (768KB)
|   VGA Display    |
+------------------+  <- 0x000A0000 (640KB)
|                  |
|    Low Memory    |
|                  |
+------------------+  <- 0x00000000  

```  
Real Mode: BIOS can get control of all machine address after power-up, which is Crucial because non any-software code in RAM can be executed by processor.  


## PARTS 


### PART I. the ROM BIOS  


At GDB First Debug stop, we can know something about real mode.  
QEMU places it own BIOS(called SeaBIOS) at the segment address (CS=0xf000:IP=0xfff0) = 0xffff0 physical address.  

```c  

[f000:fff0] 0xffff0:	ljmp   $0xf000,$0xe05b

// The IBM PC starts executing at physical address 0x000ffff0, which is at the very top of the 64KB area reserved for the ROM BIOS.
// The PC starts executing with CS = 0xf000 and IP = 0xfff0.
// The first instruction to be executed is a jmp instruction, which jumps to the `segmented address` CS = 0xf000 and IP = 0xe05b.  

``` 
So instruction begin at (CS:IP) segment address. And will execute an instruction that `jmp` to (CS=0xf000:IP=0xe05b)

Intel broke the barrier of 1MB RAM in processor 80286 and 80386. So PC architectures preserved the Origin memory layout to keep backward compatibility. Therefore, modern PC have a `hole` in physical memory from `0x000A0000 -> 0x00100000`, which dividing the memory into `low memory` and `extended memory` .    

---  

* Ex.2 Figure out the general idea of what BIOS done.   


> CPU is in Real-Mode, BIOS sets up an Interrupt Descriptor Table and initialized some devices like VGA. this is the BIOS message `Starting SeaBIOS` where com from.  
> Initialized PCI bus and all important device BIOS know about.  
> After that, searched all possible bootable devices like a floppy,hard drive, CD-ROM. Then read the `boot loader` into RAM.  

### PART II. Boot Loader    

Hard disk for PCs is divided into 512 bytes region called sector, which is also the disk's minimum transfer granularity: `read or write operations must be one or more sectors in size and aligned on a sector boundary.`   
When BIOS find a bootable disk, it will read disk first sector into physical address `0x7c00 - 0x7dff`, and use `jmp` instruction to set (CS = 0000 : IP = 7c00), passing Machine control to boot loader.  

---  

**Read code boot.S, main.c boot.asm**  

**Pre-required: read the Makefile and .gdbinit. realize the ELF format, using `readelf` and `objdump` tool help understand code.**   

--- 


> GNUmakefile:  
>  Top-level makefile, describe the info `make target rule`, boot/Makefrag nad  kern/Makefrag responsible for compile code.  


> .gdbinit:  
>  When you `make gdb` its the configuration file of gdb, connect to QEMU target and run a loop  which `translate the segment address to physical address` both in processor 16bit and 32bit mode.  


> boot/Makefrag:   
> Compiling the boot.o main.o boot.out.  Link the boot.out(also the bootloader) with `	$(V)$(LD) $(LDFLAGS) -N -e start -Ttext 0x7C00 -o $@.out $^`    


> kern/Makefrag:   
> using LD script to linking.   

> boot.S: 
> 1. Switch the processor from real-mode to 32bit protected mode, because Only in this mode software can access all memory above 1MB  in processor's physical address space. And jmp to `bootmain` function. If failed, jumped into a infinite loop.  
> 2. Set the stack pointer for `bootmain` function and loading a Bootstrap GDT to make virtual address translate.  


> main.c:  
>  Read the kernel from IDE disk device via x86's specific instructions(like `outb`, `inb`). In details, it reads via a ELF format struct, and call the kernel entry by ELF struct member, i.e. a function pointer. the main.c not returned.  

> boot.asm:   
> the disassembled code from `boot.out` ELF executable file. Reads it with GDB `x /Ni ADDR` instruction can help you identify where you are.  


---- 

* Ex.3 Using GDB debug the bootloader, examine the progress to kernel.  

`0x7c00` enter the `boot.S` program code, and if every thing OK, it will call `bootmain()` function at[0x7d19].  
the `bootmain()` function will read segment-address data to `ELF-HEADER` address(0x100000).  

```c  
	#define ELFHDR		((struct Elf *) 0x10000) // 1MB address load the kernel.img. and type is `struct Elf`, so can access members by offset. 
	// and it's a macro, also in disassembled file code.  

	struct Proghdr *ph, *eph;

	// read 1st page off disk, and `readsect()` is really read function using x86 instructions.  
	readseg((uint32_t) ELFHDR, SECTSIZE*8, 0);

	// is this a valid ELF?
	if (ELFHDR->e_magic != ELF_MAGIC)
		goto bad;

	// load each program segment (ignores ph flags)
	ph = (struct Proghdr *) ((uint8_t *) ELFHDR + ELFHDR->e_phoff);
	eph = ph + ELFHDR->e_phnum;
	
	for (; ph < eph; ph++)
		// p_pa is the load address of this segment (as well
		// as the physical address)
		readseg(ph->p_pa, ph->p_memsz, ph->p_offset);
	/** ASM for-loop 
	7d56:	39 f3                	cmp    %esi,%ebx
    7d58:	73 17                	jae    7d71 <bootmain+0x58>
    7d5a:	50                   	push   %eax
    7d5b:	83 c3 20             	add    $0x20,%ebx
    7d5e:	ff 73 e4             	push   -0x1c(%ebx)
    7d61:	ff 73 f4             	push   -0xc(%ebx)
    7d64:	ff 73 ec             	push   -0x14(%ebx)
    7d67:	e8 6e ff ff ff       	call   7cda <readseg>
    7d6c:	83 c4 10             	add    $0x10,%esp
    7d6f:	eb e5                	jmp    7d56 <bootmain+0x3d>
	 */

	// call the entry point from the ELF header
	//ASM     7d71:	ff 15 18 00 01 00    	call   *0x10018
	((void (*)(void)) (ELFHDR->e_entry))();	

```  

> `[Q]?` At what point does the processor start executing 32-bit code? What exactly causes the switch from 16-bit to 32-bit mode?  

In `boot.S` code `seta20.2` label, the instructions `.code32` made this conversion happened.   

> `[Q]?` What is the last instruction of the boot loader executed, and what is the first instruction of the kernel it just loaded?  

The last instruction of boot loader is in `boot/main.c`, instruction: ` 7d71:	ff 15 18 00 01 00    	call   *0x10018 `.  
The first instruction of kernel is in `kern/entry.S`, the instruction:  

```c
	entry:
	movw	$0x1234,0x472			# warm boot  
```

> `[Q]?`  Where is the first instruction of the kernel?   

In kern/entry.S  

> `[Q]?`  How does the boot loader decide how many sectors it must read in order to fetch the entire kernel from disk? Where does it find this information?  

Using ELF header struct members. First read a page size memory to address[0x100000] with the struct Elf type. then access struct Elf to get the ELF file end page offset. Then using `readsegt()` function to read correct num sectors.  

#### Loading Kernel  
---  


In this sections, using the `objdump` to read `obj/kern/kern and obj/boot/boot.out` various header tale information. See wiki to know ELF format and its tables [https://en.wikipedia.org/wiki/Executable_and_Linkable_Format](https://en.wikipedia.org/wiki/Executable_and_Linkable_Format).  
`objdump -h ` read the ELF file section table, it contains all section in this ELF file. 

Like: 

```c
Idx Name          Size      VMA       LMA       File off  Algn
  1 .text         0000018c  00007c00  00007c00  000000d4  2**2
                  CONTENTS, ALLOC, LOAD, CODE

``` 
VMA(link address) is the address from which section expected to executed, LMA(load address) is the memory address which section wanna be loaded into memory.   

The linker encodes the link address in the binary in various ways, such as when the code needs the `address of a global variable`,  with the result that a binary usually won't work if it is executing from an address that it's not expected linked for. (It's OK to generate positions-independent code)  

`objdump -p ` read the ELF file program header table, it contains the areas of ELF file need be loaded into memory, marked as `LOAD`. and another information:  
```c 

Program Header:
    LOAD off    0x000000d4 vaddr 0x00007c00 paddr 0x00007c00 align 2**2
         filesz 0x00000228 memsz 0x00000228 flags rwx

```  



---  

* Ex.5:  Then change the link address in boot/Makefrag to something wrong   

I know what will happen.    

---  

* Ex.6: Examine the 8 words of memory at 0x00100000 at the point the BIOS enters the boot loader, and then again at the point the boot loader enters the kernel. Why are they different?  

Just thinking, because loading the kernel.img ELF file into `0x00100000` physical address, so after bootloader cal kernel.ing entry point, the 8 words after `0x00100000` is full of kern.img `.text` section data.  

### PART III. Kernel  

#### Using virtual memory to work around position dependence  

AS we can see above, the `boot.out` link address is equaled with load address, case its the raw physical address, but `kern` link address`0xf0100000` is different with load address`0x00100000`.    
Case the `kernel` always wanna linking at very high virtual address so that low address space used for user program.    
Up until `kern/entry.S` sets the CR0_PG flag (via writing a register), memory reference are treated as physical address. Once CR0_PG flag set, memory references are virtual address that translated by hardware to physical address.  
SO We need do a map about conversion of virtual address and physical address. In this lab we will map 4MB spaces.`entrypgdir.c` finished two map.  1. From kernel memory space `0xf0000000 - 0xf0400000` to physical space `0x00000000 - 0x00400000`.  2. From virtual memory space `0x00000000 - 0x00400000` to physical memory space.  

---  

* Ex.7: What is the first instruction after the new mapping is established that would fail to work properly if the mapping weren't in place?    

After the instruction `f0100025:	0f 22 c0      	mov    %eax,%cr0`, the kernel address `0xf0100000` is mapped into physical address `0x00100000`. So the `x /8x ADDR` data result is the same.  
So the first instruction would failed if not mapped properly is  

```c
f0100028:	b8 2f 00 10 f0       	mov    $0xf010002f,%eax
f010002d:	ff e0                	jmp    *%eax
```
Because it using the kernel virtual address to call function.  If we comment that statements:  
We will get error,  

```c 
0xf0100026 <relocated+2>:    Error while running hook_stop:  
Cannot access memory at address 0xf0100026      
```  


#### Formatted Printing to the Console  

`printf.c` implemented basic print function formatted print function, variadic arguments printf function.  
`printfmt.c` implemented various formatted print functions.  
`console.c` implemented the terminal console and initialized some IO devices via writing the register.  

---  

* Ex.8: We have omitted a small fragment of code - the code necessary to print octal numbers using patterns of the form "%o". Find and fill in this code fragment.   

```c

// (unsigned) octal
case 'o':
  num = getuint(&ap, lflag);
  base = 8;
  goto number;  


```

> `[Q]?` Explain the interface between printf.c and console.c. Specifically, what function does console.c export? How is this function used by printf.c?    


* printf.c    

`putch()`  output a char to console, invoke the interface of `console.c`.  
`vcprintf()` output a formatted string and calculating the output char counts. Invoked the `vprintfmt()` of `printfmt.c`.  
`cprint()` the C-style like print function.    

* console.c  

Mainly implement IO device initialized function such as Serial, Parallel Port, CGA/VGA Display, Keyboard. And general device independent function owned by CONSOLE. Export these interface: `cons_getc(void)`, `cons_init(void)`, `cputchar(int)`, `cgetchar(void)`, `iscons(void)`.   


> `[Q]?` Explain the following from console.c:  

```c
1      if (crt_pos >= CRT_SIZE) {
2              int i;
3              memmove(crt_buf, crt_buf + CRT_COLS, (CRT_SIZE - CRT_COLS) * sizeof(uint16_t));
4              for (i = CRT_SIZE - CRT_COLS; i < CRT_SIZE; i++)
5                      crt_buf[i] = 0x0700 | ' ';
6              crt_pos -= CRT_COLS;
7      }
``` 

CONSOLE using `crt_buf` and `crt_pos` to output char and recording output positions.  SO if output char had filled on page screen,  
Clear current output buffer, and reset `crt_pos` to next new screen.  

> `[Q]?` Trace the execution of the following code step-by-step? These notes cover GCC's calling convention on the x86.  

```c 
int x = 1, y = 3, z = 4;
cprintf("x %d, y %x, z %d\n", x, y, z);  

```  

* In the call to cprintf(), to what does `fmt` point? To what does `ap` point?     

`cprintf()` declaration: `cprintf(const char *fmt, ...)`, so `fmt` point to `"x %d, y %x, z %d\n"` string, `ap` point to arguments list address in order.     


* List (in order of execution) each call to `cons_putc`, `va_arg`, and `vcprintf`. For `cons_putc`, list its argument as well. For `va_arg`, list what `ap` points to before and after the call. For `vcprintf` list the values of its two arguments.     

`cons_putc()` is in `console.c`, it defined write char to devices. `cons_putc()` is wrapped by `printf.c/putch()`, `putch()` as callback for some function in `printfmt.c`.    
`va_arg` used for parsing `va_list ap` pointer, mainly used in printfmt.c `getint()/getuint()/vprintfmt()` functions.    
`vcprintf()` defined in `printf.c`, to print specific arguments parsed in `cprintf()` function.  
```c

/* Call Stack: 
cprintf(const char* fmt, ...) 
\/
vcprintf(const char* fmt, va_list ap)
\/
void vprintfmt(void (*putch)(int, void *), void *putdat, const char *fmt, va_list ap)
\/
printnum()/putch(int ch, int *cnt)
\/
cons_putc(int ch)
*/

// Output: Numbers x= //
(gdb) f
#2  0xf0100911 in vcprintf (fmt=0xf01019b3 "Numbers x=%d y=%d z=%d\n", 
    ap=0xf010afc4 "\001") at kern/printf.c:21
21              vprintfmt((void*)putch, &cnt, fmt, ap);
(gdb) p fmt 
$9 = 0xf01019b3 "Numbers x=%d y=%d z=%d\n"
(gdb) p ap
$10 = (va_list) 0xf010afc4 "\001"

// Output: Numbers x=1 //
(gdb) f
#4  0xf01010ff in vprintfmt (putch=<optimized out>, putdat=0xf010af8c, 
    fmt=0xf01019bf " y=%d z=%d\n", ap=0xf010afc8 "\003") at lib/printfmt.c:215
215           printnum(putch, putdat, num, base, width, padc);
(gdb) p fmt 
$23 = 0xf01019bf " y=%d z=%d\n"
(gdb) p ap
$24 = (va_list) 0xf010afc8 "\003"

(gdb) f 
#0  cons_putc (c=c@entry=49) at kern/console.c:436
436             lpt_putc(c);
(gdb) p c
$25 = 49

// Output: Numbers x=1 y=3 //
(gdb) f
#4  0xf01010ff in vprintfmt (putch=<optimized out>, putdat=0xf010af8c, 
    fmt=0xf01019c4 " z=%d\n", ap=0xf010afcc "\004") at lib/printfmt.c:215
215           printnum(putch, putdat, num, base, width, padc);
(gdb) p fmt 
$32 = 0xf01019c4 " z=%d\n"
(gdb) p ap 
$33 = (va_list) 0xf010afcc "\004"

(gdb) f
#0  cons_putc (c=c@entry=51) at kern/console.c:436
436             lpt_putc(c);
(gdb) p c
$34 = 51


```  
As above show, x86 calling convention passing argument Rigth to Left pushed them into stack, so `va_list ap` is `0xf010afc4 -> 0xf010afc8 -> 0xf010afcc`, to access the arguments address. The `va_arg()` finishd the stack address changed.  


> `[Q]?` What is the output? Explain how this output is arrived at in the step-by-step manner of the previous exercise:  

```c
unsigned int i = 0x00646c72;
cprintf("H%x Wo%s", 57616, &i);

```  

**Output:**   
```c 
Print i: He110 World
``` 

Because 57616(hex) = e110(decimal), `va_arg()` parsed `&i` as `rld`.  

> `[Q]?` what is going to be printed after 'y='? (note: the answer is not a specific value.) Why does this happen?  

```c
cprintf("x=%d, y=%d\n",3);
```  

output `x-3, y=-2671xxx`, because using the statement `unsigned long long num  = static long long getint()`, then `(long long)num` trigger the type overflow.  

> `[Q]?` Let's say that GCC changed its calling convention so that it pushed arguments on the stack in declaration order, so that the last argument is pushed last. How would you have to change cprintf or its interface so that it would still be possible to pass it a variable number of arguments?   

[TODO] HARD!!!  

---  
##### Challenge
**Enhance the console to allow text to be printed in different colors. If you're feeling really adventurous, you could try switching the VGA hardware into a graphics mode and making the console draw text onto the graphical frame buffer.**   
Console printed in different colors, easy.  
switching VGA to graphics mode? quit.    
---  


#### Stack  
--- 

* Ex.9: Determine where the kernel initializes its stack, and exactly where in memory its stack is located. How does the kernel reserve space for its stack? And at which "end" of this reserved area is the stack pointer initialized to point to?  

Before call any C function, the machine should initialized stack work done. So the stack initialized in `entry.S:relocated`.  After virtual address paging.  
`entry.S:reloacted` clear the `EBP` register, and Using GDB can see that `ESP` start from `0xf010b00`.  
```c 
Section Headers:
  [Nr] Name              Type            Addr     Off    Size   
  [ 0]                   NULL            00000000 000000 000000 
  [ 1] .text             PROGBITS        f0100000 001000 00194d 
  [ 2] .rodata           PROGBITS        f0101960 002960 00080c 
  [ 3] .note.gnu.pr[...] NOTE            f010216c 00316c 000028 
  [ 4] .stab             PROGBITS        f0102194 003194 000001 
  [ 5] .stabstr          STRTAB          f0102195 003195 000001 
  [ 6] .data             PROGBITS        f0103000 004000 00a300
  [ 7] .bss              PROGBITS        f010d300 00e300 000661 

```  
And in `entry.S`:  
```c
relocated:

	movl	$0x0,%ebp			# nuke frame pointer

	# Set the stack pointer
	movl	$(bootstacktop),%esp

	# now to C code
	call	i386_init

// ...............
	.p2align	PGSHIFT		# force page alignment
	.globl		bootstack
bootstack:
	.space		KSTKSIZE
	.globl		bootstacktop   
bootstacktop:

```
So the stack address space layout:  
> bootstack(0xf010300), also the `.data` section link address.  
> bootstacktop(0xf010b00), `ESP` start to decrease.  
> `.bss` section at (0xf010d300)  

The kernel link the `ESP` at high address to preserve stack address space, the space size = 0xf010b00 - 0xf010300 = 2Kib

---  

* Ex.10:  Examine what happens each time it gets called after the kernel starts. How many 32-bit words does each recursive nesting level of test_backtrace push on the stack, and what are those words?    
After first call at `init.c:i386_init()`, every time nested call will push 5 32-bit words before.  
First call set `%EAX` store arguments 'five'. Then every nested call will pretend/know that `%EAX` holds this call fram function argument.  
Then will push `%EBP %EBX %EBX(After set as %EAX value) $0xf0101960 %EAX(After calculated minos 1)`.  


---  

* Ex.11:  mon_backtrace() function.  

OK!!


* Ex.12: Modify your stack backtrace function to display, for each eip, the function name, source file name, and line number corresponding to that eip.   

[TODO]  


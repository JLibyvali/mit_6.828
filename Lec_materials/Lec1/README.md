# Lab1 Notes

## Pre-required  
1GBit: 0x40000000(Hex)  
1MBit: 0x100000(Hex)  
1KBit: 0x400(Hex)  

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

> Question At what point does the processor start executing 32-bit code? What exactly causes the switch from 16-bit to 32-bit mode?  

In `boot.S` code `seta20.2` label, the instructions `.code32` made this conversion happened.   

> Question What is the last instruction of the boot loader executed, and what is the first instruction of the kernel it just loaded?  

The last instruction of boot loader is in `boot/main.c`, instruction: ` 7d71:	ff 15 18 00 01 00    	call   *0x10018 `.  
The first instruction of kernel is in `kern/entry.S`, the instruction:  

```c
	entry:
	movw	$0x1234,0x472			# warm boot  
```

> Question Where is the first instruction of the kernel?   

In kern/entry.S  

> Question How does the boot loader decide how many sectors it must read in order to fetch the entire kernel from disk? Where does it find this information?  

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


---  

* Ex.7: What is the first instruction after the new mapping is established that would fail to work properly if the mapping weren't in place?    



        * From objdump: the bootloader load address and link address are same, but it's different in kernel: load at [0xf0100000] and linked at [0x00100000].    
        * In 'entry()[at 0x10000c] ': Before enter kernel code, first into entry.S. Base information above, kernel code linked and run from [0xf0100000], So need to map the virtual memory to physical memory within 4MB memory in Lab1.  
        The mapping finished by page-table handed write in 'entrypgdir.c',  mapping 2 VM(virtual memory) ranges to PM. After entry.S set FLAG(CR0_PG), the VM finished mapping. And jump to KERNELBASE and call  i386_init();
        * If we comment the important code[movl %eax ,%cr0]: System exit, can't find address [0xf0100000], sucked in 'entry.S'. And 'x /x' output some address not under [0xf0400000]. 

---  

* Ex.8: We have omitted a small fragment of code - the code necessary to print octal numbers using patterns of the form "%o". Find and fill in this code fragment.   



> Question Explain the interface between printf.c and console.c. Specifically, what function does console.c export? How is this function used by printf.c?  

> Question Explain the following from console.c:  

```c
1      if (crt_pos >= CRT_SIZE) {
2              int i;
3              memmove(crt_buf, crt_buf + CRT_COLS, (CRT_SIZE - CRT_COLS) * sizeof(uint16_t));
4              for (i = CRT_SIZE - CRT_COLS; i < CRT_SIZE; i++)
5                      crt_buf[i] = 0x0700 | ' ';
6              crt_pos -= CRT_COLS;
7      }
```

> Question Trace the execution of the following code step-by-step? These notes cover GCC's calling convention on the x86.  

```c 
int x = 1, y = 3, z = 4;
cprintf("x %d, y %x, z %d\n", x, y, z);

// In the call to cprintf(), to what does `fmt` point? To what does `ap` point?
// List (in order of execution) each call to cons_putc, va_arg, and vcprintf. For cons_putc, list its argument as well. For va_arg, list what ap points to before and after the call. For vcprintf list the values of its two arguments.  

```

> Question What is the output? Explain how this output is arrived at in the step-by-step manner of the previous exercise:  

```c
unsigned int i = 0x00646c72;
cprintf("H%x Wo%s", 57616, &i);

```

> Question what is going to be printed after 'y='? (note: the answer is not a specific value.) Why does this happen?  

```c
cprintf("x=%d, y=%d\n",3);
```
> Question How would you have to change cprintf or its interface so that it would still be possible to pass it a variable number of arguments?  

Using macro,  

---  

#### Challenge
**Enhance the console to allow text to be printed in different colors.**  


---  

> Exercies code: finished '%o' format output  
> origin code:  
```C
        // (unsigned) octal
		case 'o':
			// Replace this with your code.
			putch('X', putdat);
			putch('X', putdat);
			putch('X', putdat);
			break;

```
> Changed code: 
```C
		// (unsigned) octal
		case 'o':
			num = getint(&ap, lflag);
			putch('0',putdat);
			base = 8;

			goto number;
			break;
```

> Answer the questions:  

*  ***Explain the interface between printf.c and console.c. Specifically, what function does console.c export? How is this function used by printf.c?***
     'console.c' export the 'cons_putc()' for printf.c, which can support the const character output.
* ***Explain the following from console.c:***  

```c

1      if (crt_pos >= CRT_SIZE) {
2              int i;
3              memmove(crt_buf, crt_buf + CRT_COLS, (CRT_SIZE - CRT_COLS) * sizeof(uint16_t));
4              for (i = CRT_SIZE - CRT_COLS; i < CRT_SIZE; i++)
5                      crt_buf[i] = 0x0700 | ' ';
6              crt_pos -= CRT_COLS;
7      }

```
    'crt_pos' was position pointer. Whole console size:25x80, if the pointer moved to edge, move the pointer to the '0 col', and other data to next line.
    
* ***Let's say that GCC changed its calling convention so that it pushed arguments on the stack in declaration order, so that the last argument is pushed last. How would you have to change cprintf or its interface so that it would still be possible to pass it a variable number of arguments?***

> Challenge:
Challenge Enhance the console to allow text to be printed in different colors. The traditional way to do this is to make it interpret ANSI escape sequences embedded in the text strings printed to the console, but you may use any mechanism you like. There is plenty of information on the 6.828 reference page and elsewhere on the web on programming the VGA display hardware. If you're feeling really adventurous, you could try switching the VGA hardware into a graphics mode and making the console draw text onto the graphical frame buffer.  

#### Stack
* 9. : Kernel initializes the stack in 'entry.S': movl $(bootstacktop),%esp[0xf0100034].  Kernel push the 'entry_pgdir' memory address into %esp and push [0x00] into $ebp  to reserve the kernel space. And the %esp point to [0xf010b021], kernel base is [0xf0000000]. It also the stack top position.

* 10. StackBacktrace:  Every time getting into 'test_backtrace',  first the stack-frame [$eip] register also called Program counter register will storaged the return address , which is the stack next instruction. Then call another 'test_backtrace' will entry new stack frame, at the begning of new frame, first 'push $ebp' and 'mov $esp, $ebp' storaged stack-frame information to $ebp. Every time getting into 'test_backtrace' will push 2 32-bit words into stack, the $ebp and $ebx storaging stack infomation and arguments.


* 11. Implement the function in monitor.c: mon_backtrace().  
code from others:
```c
	// Your code here.

	uint32_t *ebp;

	ebp = (uint32_t *) read_ebp();
	cprintf("Stack Backtrace:\n");

	while (ebp!=0) {
		cprintf("  ebp %08x",ebp);
		cprintf("  eip %08x",*(ebp+1));
		
		cprintf(" args");
		cprintf(" %08x",*(ebp+2));
		cprintf(" %08x",*(ebp+3));
		cprintf(" %08x",*(ebp+4));
		cprintf(" %08x",*(ebp+5));
		cprintf(" %08x\n",*(ebp+6));

		ebp = (uint32_t*) *ebp;
	}
	
```

* 12. Modify your stack backtrace function to display, for each eip, the function name, source file name, and line number corresponding to that eip.
        * look for the lab's information to understand what the __STAB_* from.  
```C
	// Search within [lline, rline] for the line number stab.
	// If found, set info->eip_line to the right line number.
	// If not found, return -1.
	//
	// Hint:
	//	There's a particular stabs type used for line numbers.
	//	Look at the STABS documentation and <inc/stab.h> to find
	//	which one.
	// Your code here.
	stab_binsearch(stabs, &lline, &rline, N_SLINE, addr);

	if(lline <= rline){
		info->eip_line = stabs[rline].n_desc;
	}else{
		return -1;
	}
```

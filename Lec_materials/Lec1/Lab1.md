# Lab1 configuration    fall 2018
## Basic
real mode:
    Real mode is a simple mode, support for first PCs, such as intel-8808. In this mode, memory address were directly mapped, virtual address is equal to physical address.


## ToolChain preparation

* qemu emulator, at right version.
* gdb tools

## Exerices
### PC Bootstrap
* 1. : use jos system. jos was a 16-bit system with x86-i386 architecture. 16-bit PCs only capable for addressing 1MB of physical memory, using one offset to calculate the physical address using viural address. 
* 2. : GDB usages is the main, 
### Bootloader
* 3. : use gdb to debug the 'Boot Loader', breakpoint at [0X7c00], and answer the question.
    * At what point does the processor start executing 32-bit code? What exactly causes the switch from 16-bit to 32-bit mode?
        After the "enable A20 stage", "use Switch from real to protected mode. Using a bootstrap GDT and segment translation that makes virtual addresses identical to their physical addresses, so that the effective memory map does not change during the switch." is the reason changed. But the point that starting executing 32-bit code is at address:[0x7c2d], we find here another "ljmp" command.
        In 16-bit mode run 720Bytes.
    
    * What is the last instruction of the boot loader executed, and what is the first instruction of the kernel it just loaded?
        The boot loader executed " ((void (*)(void)) (ELFHDR->e_entry))();" at [0x7d71], ASM:call [0x10018]. And between boot loader executable file to kernel file, is still exist other register commands.
        After those, the kernel first instruction:in entry.S:74 relocated: Clear the frame pointer register (EBP) so that once we get into debugging C code, stack backtraces will be terminated properly.
         
    * Where is the first instruction of the kernel?  
       It's at [0xf010002f]. 
    
    * How does the boot loader decide how many sectors it must read in order to fetch the entire kernel from disk? Where does it find this information?
        To  read the information in ELF header and with the offset increse. ELF header.  


> So apparently, boot loader execution order:machine boot -> entry 16-bit real mode -> enable A20 stage then can use the virtual address -> entry 32-bit protected mode -> call bootmain() ->  read bytes from disk to ram, the segment nums from ELF header -> call kernel file: entry -> finished boot loader.  

* 4. : C pointer

* 5. : in 'jos/boot/Makefrag' , the boot loader loading address was set to '0x7c00': case it could disable the interrupt so that the system will not keep restart. 

* 6. : When at bootloader, "x /8x 0x100000" always keep 0, after call "enrty()" int main.c. It begins have value. In 'enrty()' will build map from virtual address to physical address.

### Kernel
* 7. : Kernel: 
        * From objdump: the bootloader load address and link address are same, but it's different in kernel: load at [0xf0100000] and linked at [0x00100000].    
        * In 'entry()[at 0x10000c] ': Before enter kernel code, first into entry.S. Base information above, kernel code linked and run from [0xf0100000], So need to map the virtual memory to physical memory within 4MB memory in Lab1.  
        The mapping finished by page-table handed write in 'entrypgdir.c',  mapping 2 VM(virtual memory) ranges to PM. After entry.S set FLAG(CR0_PG), the VM finished mapping. And jump to KERNELBASE and call  i386_init();
        * If we comment the important code[movl %eax ,%cr0]: System exit, can't find address [0xf0100000], sucked in 'entry.S'. And 'x /x' output some address not under [0xf0400000]. 

* 8. : printf():
    * 'print.c' main implement the basic print function, 'printfmt.c' main implement the output format base on the string content, 'console.c' main implement the keyboard input function, and initialize some serial devices.

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

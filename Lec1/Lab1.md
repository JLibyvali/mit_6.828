# Lab1 configuration    fall 2018
## Basic
real mode:
    Real mode is a simple mode, support for first PCs, such as intel-8808. In this mode, memory address were directly mapped, virtual address is equal to physical address.


## ToolChain preparation


## Exerices
* 1: use jos system. jos was a 16-bit system with x86-i386 architecture. 16-bit PCs only capable for addressing 1MB of physical memory, using one offset to calculate the physical address using viural address. 
* 2: GDB usages is the main, 
* 3: use gdb to debug the 'Boot Loader', breakpoint at '0X7c00', and answer the question.
    * At what point does the processor start executing 32-bit code? What exactly causes the switch from 16-bit to 32-bit mode?
        After the "enable A20 stage", "use Switch from real to protected mode. Using a bootstrap GDT and segment translation that makes virtual addresses identical to their physical addresses, so that the effective memory map does not change during the switch." is the reason changed. But the point that starting executing 32-bit code is at address:"0x7c2d", we find here another "ljmp" command.
        In 16-bit mode run 720Bytes.
    
    * What is the last instruction of the boot loader executed, and what is the first instruction of the kernel it just loaded?
        The boot loader executed " ((void (*)(void)) (ELFHDR->e_entry))();" at 0x7d71, ASM:call 0x10018. And between boot loader executable file to kernel file, is still exist other register commands.
        After those, the kernel first instruction:in entry.S:74 relocated: Clear the frame pointer register (EBP) so that once we get into debugging C code, stack backtraces will be terminated properly.
         
    * Where is the first instruction of the kernel?  
       It's at 0xf010002f. 
    
    * How does the boot loader decide how many sectors it must read in order to fetch the entire kernel from disk? Where does it find this information?
        To  read the information in ELF header and with the offset increse. ELF header.  


> So apparently, boot loader execution order:machine boot -> entry 16-bit real mode -> enable A20 stage then can use the virtual address -> entry 32-bit protected mode -> call bootmain() ->  read bytes from disk to ram, the segment nums from ELF header -> call kernel file: entry -> finished boot loader.  

* 4: C pointer
* 5: in 'jos/boot/Makefrag' , the boot loader loading address was set to '0x7c00': case it could disable the interrupt so that the system will not keep restart. 
* 6: 


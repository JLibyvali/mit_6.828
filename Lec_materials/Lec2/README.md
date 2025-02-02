# LAB 2 Notes  

## PARTS

### PART I. Physical Page Management  

---  


* Ex.1: implement `boot_alloc() mem_init() (only up to the call to check_page_free_list(1)) page_init() page_alloc() page_free()` functions.  

After finished those function, you should pass the `check_page_free_list()` and `check_page_alloc()` function.  



### PART II. Virtual Memory  
---  

Protect mode address:  
> once we're in protected mode (which we entered first thing in boot/boot.S), there's no way to directly use a linear or physical address. All memory references are interpreted as virtual addresses and translated by the MMU, which means all pointers in C are virtual addresses.  

It represents that using segmentation address(segment selector + offset).  

Using type alias to help identify the type, because JOS kernel always need to manipulate Address as Opaque value or Integer,  
so help to  distinguish the two cases: the `type uintptr_t` represents opaque virtual addresses, and `physaddr_t` represents physical addresses.  

> IMPORTANT:  
> The kernel `can't sensibly dereference` a physical address, since the MMU translates all memory references.  

So if we try to cast a `physaddr_t` to `uint32_t*` and dereference it, We may be able to Load and Store to the resulting address (the hardware will interpret it as a virtual address), but you probably won't get the memory location you intended.  




---  
* Ex.2: Read `Intel 80386 Manual`, make sure understand the segmentation, page translation, page-base protection.  


--- 
* Ex.3: Using QEMU `xp` command to inspect the physical address. `info pg / info mem` to inspect pages.   


> `[Q]?`:  
> Assuming that the following JOS kernel code is correct, what type should variable x have, uintptr_t or physaddr_t?  

```c
	mystery_t x;
	char* value = return_a_pointer();
	*value = 10;
	x = (mystery_t) value;
```  

`mystery_t` should be uintptr_t.  

And for `address value` translation, `PADDR(kva) and KADDR(pa)` responsible for this work.  




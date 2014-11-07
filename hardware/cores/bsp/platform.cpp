
#include <avr32.h>

extern "C"
{
	void bsp_main(void);
	int main(void);
	
	void _init(void);
	void __libc_init_array(void);
}

void _init(void)
{
	sysclk_init();
	
	// Disable all interrupts.
	Disable_global_interrupt();

	// Initialize interrupt vectors.
	INTC_init_interrupts();
	
	// Enable all interrupts.
	Enable_global_interrupt();
}

void bsp_main(void)
{
	//
	// call static constructors
	//
	__libc_init_array(); // calls "_init()"
	
	// user code
	main();
}

void *operator new(size_t size)
{
	return malloc(size);
}

void operator delete(void * ptr)
{
	free(ptr);
}

void *operator new[](size_t size)
{
	return malloc(size);
}

void operator delete[](void * ptr)
{
	if (ptr)
		free(ptr);
}

__extension__ typedef int __guard __attribute__((mode (__DI__)));

extern "C"
{
	int __cxa_guard_acquire(__guard *);
	void __cxa_guard_release (__guard *);
	void __cxa_guard_abort (__guard *);
	void __cxa_pure_virtual(void);
}

int __cxa_guard_acquire(__guard *g) {return !*(char *)(g);};
void __cxa_guard_release (__guard *g) {*(char *)g = 1;};
void __cxa_guard_abort (__guard *) {};


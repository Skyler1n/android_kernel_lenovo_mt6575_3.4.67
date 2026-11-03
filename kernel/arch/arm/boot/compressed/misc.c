/*
 * misc.c
 * 
 * This is a collection of several routines from gzip-1.0.3 
 * adapted for Linux.
 *
 * malloc by Hannu Savolainen 1993 and Matthias Urlichs 1994
 *
 * Modified for ARM Linux by Russell King
 *
 * Nicolas Pitre <nico@visuaide.com>  1999/04/14 :
 *  For this code to run directly from Flash, all constant variables must
 *  be marked with 'const' and all other variables initialized at run-time 
 *  only.  This way all non constant variables will end up in the bss segment,
 *  which should point to addresses in RAM and cleared to 0 on start.
 *  This allows for a much quicker boot time.
 */

unsigned int __machine_arch_type;

#include <linux/compiler.h>	/* for inline */
#include <linux/types.h>
#include <linux/linkage.h>

static void putstr(const char *ptr);
extern void error(char *x);

#include <mach/uncompress.h>

#ifdef CONFIG_DEBUG_ICEDCC

#if defined(CONFIG_CPU_V6) || defined(CONFIG_CPU_V6K) || defined(CONFIG_CPU_V7)

static void icedcc_putc(int ch)
{
	int status, i = 0x4000000;

	do {
		if (--i < 0)
			return;

		asm volatile ("mrc p14, 0, %0, c0, c1, 0" : "=r" (status));
	} while (status & (1 << 29));

	asm("mcr p14, 0, %0, c0, c5, 0" : : "r" (ch));
}


#elif defined(CONFIG_CPU_XSCALE)

static void icedcc_putc(int ch)
{
	int status, i = 0x4000000;

	do {
		if (--i < 0)
			return;

		asm volatile ("mrc p14, 0, %0, c14, c0, 0" : "=r" (status));
	} while (status & (1 << 28));

	asm("mcr p14, 0, %0, c8, c0, 0" : : "r" (ch));
}

#else

static void icedcc_putc(int ch)
{
	int status, i = 0x4000000;

	do {
		if (--i < 0)
			return;

		asm volatile ("mrc p14, 0, %0, c0, c0, 0" : "=r" (status));
	} while (status & 2);

	asm("mcr p14, 0, %0, c1, c0, 0" : : "r" (ch));
}

#endif

#define putc(ch)	icedcc_putc(ch)
#endif

static void putstr(const char *ptr)
{
	char c;

	while ((c = *ptr++) != '\0') {
		if (c == '\n')
			putc('\r');
		putc(c);
	}

	flush();
}

/*
 * gzip declarations
 */
extern char input_data[];
extern char input_data_end[];

unsigned char *output_data;

unsigned long free_mem_ptr;
unsigned long free_mem_end_ptr;

#ifndef arch_error
#define arch_error(x)
#endif

void error(char *x)
{
	arch_error(x);

	putstr("\n\n");
	putstr(x);
	putstr("\n\n -- System halted");

	while(1);	/* Halt */
}

asmlinkage void __div0(void)
{
	error("Attempting division by 0!");
}

extern int do_decompress(u8 *input, int len, u8 *output, void (*error)(char *x));


void
decompress_kernel(unsigned long output_start, unsigned long free_mem_ptr_p,
		unsigned long free_mem_ptr_end_p,
		int arch_id)
{
	int ret;

	output_data		= (unsigned char *)output_start;
	free_mem_ptr		= free_mem_ptr_p;
	free_mem_end_ptr	= free_mem_ptr_end_p;
	__machine_arch_type	= arch_id;

	arch_decomp_setup();

	putstr("Uncompressing Linux...");
	ret = do_decompress(input_data, input_data_end - input_data,
			    output_data, error);
	if (ret)
		error("decompressor returned an error");
	else
		putstr(" done, booting the kernel.\n");
}

/* Called before GOT/BSS setup. Keep this routine freestanding: no globals,
 * no allocation, no library calls. Restrict to the old A690 U-Boot address.
 * 64MiB is inside the captured A690 ATAG_MEM range and well above the
 * current kernel's end (0x025e5ac0). The bootloader already loaded initrd
 * here before the kernel; moving it later in arm_memblock_init is too late.
 */
void a690_relocate_legacy_initrd(unsigned int *tags)
{
    unsigned int remaining = 4096;
    unsigned int n, start, size, i;
    volatile unsigned char *src, *dst;

    if ((unsigned int)tags < 0x01600000 ||
        (unsigned int)tags >= 0x20000000 || ((unsigned int)tags & 3))
        return;
    if (tags[1] != 0x54410001) /* ATAG_CORE */
        return;

    while (remaining >= 2) {
        n = tags[0];
        if (n < 2 || n > remaining)
            return;
        if ((tags[1] == 0x54420005 || tags[1] == 0x54410005) && n >= 4) {
            start = tags[2];
            size = tags[3];
            if (tags[1] == 0x54410005) {
                if (start < 0xc0000000)
                    return;
                start = start - 0xc0000000 + 0x01600000;
            }
            if (start != 0x02300000 || !size || size > 0x00800000)
                return;
            src = (volatile unsigned char *)start;
            dst = (volatile unsigned char *)0x04000000;
            for (i = 0; i < size; i++)
                dst[i] = src[i];
            tags[2] = 0x04000000;
            tags[1] = 0x54420005; /* physical ATAG_INITRD2 */
            return;
        }
        tags += n;
        remaining -= n;
    }
}

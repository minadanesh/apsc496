#ifndef __SRCVERSION_H_GUARD
#define __SRCVERSION_H_GUARD

#ifndef __USESRCVERSION
#define __SRCVERSION(id)
#else /* __USESRCVERSION */

#ifdef __QNXNTO__

#if defined __SRCVERSION
#undef __SRCVERSION
#endif /*__SRCVERSION */

#define __SRCVERSION(id) \
	__asm__(".section .ident,\"SM\",@progbits,1"); \
	__asm__(".asciz " #id); \
	__asm__(".previous");

#endif /* __QNXNTO__ */

#endif /* __USESRCVERSION */

#endif /* __SRCVERSION_H_GUARD */

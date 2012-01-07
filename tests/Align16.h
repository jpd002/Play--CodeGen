#ifndef _ALIGN16_H_
#define _ALIGN16_H_

#ifdef _MSC_VER
#define ALIGN16 __declspec(align(16))
#else
#define ALIGN16 __attribute__((aligned(16)))
#endif

#endif

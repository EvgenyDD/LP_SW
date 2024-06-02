#include "cobs.h"
#include <stddef.h>
#include <stdint.h>

#define StartBlock() (code_ptr = dst++, code = 1)
#define FinishBlock() (*code_ptr = code)

unsigned int cobs_encode(const uint8_t *ptr, uint16_t length, volatile uint8_t *dst)
{
	const volatile uint8_t *start = dst, *end = ptr + length;
	volatile uint8_t code, *code_ptr; /* Where to insert the leading count */

	StartBlock();
	while(ptr < end)
	{
		if(code != 0xFF)
		{
			uint8_t c = *ptr++;
			if(c != 0)
			{
				*dst++ = c;
				code++;
				continue;
			}
		}
		FinishBlock();
		StartBlock();
	}
	FinishBlock();
	return (unsigned int)(dst - start);
}

unsigned int cobs_decode(const uint8_t *ptr, uint16_t length, uint8_t *dst)
{
	const uint8_t *start = dst, *end = ptr + length;
	uint8_t code = 0xFF, copy = 0;

	for(; ptr < end; copy--)
	{
		if(copy != 0)
		{
			*dst++ = *ptr++;
		}
		else
		{
			if(code != 0xFF) *dst++ = 0;
			copy = code = *ptr++;
			if(code == 0) break; /* Source length too long */
		}
	}
	return (unsigned int)(dst - start);
}

#ifndef LP_H
#define LP_H

#include <stdbool.h>
#include <stdint.h>

#define CLR_NULL 2
#define CLR_B 4
#define CLR_R 5
#define CLR_G 3
#define CLR_WHT 6

inline const char *clr_to_str(uint32_t color)
{
	if(color == CLR_NULL) return "no";
	if(color == CLR_R) return "red";
	if(color == CLR_G) return "grn";
	if(color == CLR_B) return "blu";
	if(color == CLR_WHT) return "wht";
	return "-";
}

void lp_square(uint8_t color);
void lp_blank(void);

#endif // LP_H
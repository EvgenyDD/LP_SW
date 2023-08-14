#ifndef LP_H__
#define LP_H__

#include <stdbool.h>
#include <stdint.h>

#define CLR_NULL 2
#define COORD_X 1
#define COORD_Y 2
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

void lp_fill_image(void);

void lp_off(void);
void lp_on(void);

void lp_fb_reinit(void);
void lp_fb_append(float x, float y, float cr, float cg, float cb, float scale, float max_br, uint32_t us);

#endif // LP_H__
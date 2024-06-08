#include <math.h>
#include <stdint.h>
#include <stdio.h>

static double gamma_corr(double i)
{
	return powf(i / 4095.0, 4.5f) * 4095.0;
}

void main(void)
{
	printf("uint16_t gamma[4096]={");
	for(uint32_t i = 0; i < 4096; i++)
	{
		printf("%d", (uint16_t)gamma_corr(i));
		if(i != 4095)
		{
			printf(",");
			if((i % 128) == 127) printf("\n");
		}
	}
	printf("};\n");
}
#include "ilda.h"
#include <dirent.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FREE_PTR(x) \
	if(x)           \
	free(x)

#define S(x)                             \
	if((x))                              \
	{                                    \
		printf("[E]\t%s failed!\n", #x); \
		break;                           \
	}

static int get_buf_from_file(const char *f_name, char **p_buf, size_t *buf_size)
{
	FILE *fp = fopen(f_name, "r");
	// printf("[I]\tReading file '%s'...\n", f_name);
	if(!fp)
	{
		printf("[E]\tfopen (%s) failed!\n", f_name);
		return 1;
	}
	if(fseek(fp, 0L, SEEK_END) != 0)
	{
		printf("[E]\tfseek (%s) failed!\n", f_name);
		return 2;
	}
	long file_size = ftell(fp);
	if(file_size <= 0)
	{
		printf("[W]\tfile (%s) size is %ld!\n", f_name, file_size);
		return 3;
	}

	*p_buf = !*p_buf
				 ? (char *)malloc(file_size * sizeof(char))
				 : (char *)realloc(*p_buf, (*buf_size + file_size) * sizeof(char));
	if(!*p_buf)
	{
		printf("[E]\talloc failed!\n");
		return 4;
	}
	if(fseek(fp, 0L, SEEK_SET) != 0)
	{
		printf("[E]\tfseek failed!\n");
		return 5;
	}
	long new_len = fread(*p_buf + *buf_size, sizeof(char), file_size, fp);
	if(ferror(fp) != 0)
	{
		fputs("Error reading file", stderr);
		return 6;
	}
	if(new_len != file_size)
	{
		printf("[E]\tfile len read mismatch!\n");
		return 4;
	}
	*buf_size += file_size;

	return 0;
}

int main(int argc, char **argv)
{
	uint32_t biggest_point_count = 0;

// int s = strncmp("console?al", "con",3 );
// char *che = strstr("console?al", "8on");
// 	printf("AAAAAAA %d %c\n", s, *che);
// 	return 2;

	struct dirent *dir;
	// const char *src = argv[1];
	const char *src = "../../ilda_files/";
	DIR *d = opendir(src);
	int fc = 0;
	if(d)
	{
		while((dir = readdir(d)) != NULL)
		{
			if(strstr(dir->d_name, ".ild") || strstr(dir->d_name, ".ILD"))
			{
				do
				{
					char *buf = NULL;
					size_t buf_size = 0;
					// S(argc == 1);
					char fname[1024];
					strcpy(fname, src);
					strcat(fname, dir->d_name);
					S(get_buf_from_file(fname, &buf, &buf_size));
					printf("\tFile %s: %ld bytes ", fname, buf_size);
#if 1
					ilda_t i = {0};
					for(uint32_t of = 0;;)
					{
#define CHNK (ILDA_FILE_CHUNK / 2)
						int sts = ilda_file_parse_chunk(&i, (uint8_t *)&buf[of], buf_size - of >= CHNK ? CHNK : buf_size - of, true * 0);
						if(sts != 0)
						{
							printf("[E]\tFailed: %d\n", sts);
							return sts;
						}
						of += buf_size - of >= CHNK ? CHNK : buf_size - of;
						if(of >= buf_size)
						{
							sts = ilda_file_parse_chunk(&i, NULL, 0, true * 0);
							if(sts != 0)
							{
								printf("[E]\tFailed: %d\n", sts);
								return sts;
							}
							break;
						}
					}
#else
					ilda_t i = {0};
					int sts = ilda_file_parse_file(&i, (uint8_t *)buf, buf_size, true * 0);
					if(sts != 0)
					{
						printf("[E]\tFailed: %d\n", sts);
						return sts;
					}
#endif
					printf("\t\t%d frames %d points\n", i.frame_count, i.point_count);
					if(i.max_point_per_frame > biggest_point_count) biggest_point_count = i.max_point_per_frame;
					FREE_PTR(buf);
					ilda_file_free(&i);
				} while(0);
				fc++;
			}
			// if(fc >= 8) break;
		}
		closedir(d);
	}
	printf("Max points: %d\n", biggest_point_count);

	return 0;
}
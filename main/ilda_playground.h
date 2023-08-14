#ifndef ILDA_PLAYGROUND_H__
#define ILDA_PLAYGROUND_H__

enum
{
	ERR_OPEN = -100
};

int ilda_check_file(const char *path);
int ilda_file_load(const char *path);

#endif // ILDA_PLAYGROUND_H__
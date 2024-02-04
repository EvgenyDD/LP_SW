EXE_NAME=anlz
SOURCES += anlz.c ../main/laser/ilda_file.c
include core.mk
# EXT_LIBS += ws2_32
# TCHAIN = x86_64-w64-mingw32-
INCDIR += ../main/laser
FLAGS += -ggdb -g

run: $(EXECUTABLE)
	@./$(EXECUTABLE) 
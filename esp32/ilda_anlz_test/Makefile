EXE_NAME=anlz
SOURCES += $(call rwildcard,.,*.c)
include core.mk
# EXT_LIBS += ws2_32
# TCHAIN = x86_64-w64-mingw32-
INCDIR += ../main/laser
FLAGS += -ggdb -g

run: $(EXECUTABLE)
	@./$(EXECUTABLE) ../../ilda_files_orig/
EXE_NAME=anlz
SOURCES += $(call rwildcard,.,*.c)
include core.mk
# TCHAIN = x86_64-w64-mingw32-
INCDIR += ../main

run: $(EXECUTABLE)
	@./$(EXECUTABLE) ../../ilda_files/
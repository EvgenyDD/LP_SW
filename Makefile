PROJECT_NAME := LP
ESPPORT=COM5
include $(IDF_PATH)/make/project.mk

LISTING=build/$(PROJECT_NAME).lst

listing: $(LISTING)

$(LISTING): $(APP_ELF)
	$(OBJDUMP) -d -t -S $< >$(@)
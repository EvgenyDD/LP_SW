all:
	@idf.py build

full: web/page/dist/index.html
	@idf.py build

f: full
	idf.py -p COM5 flash

fm: f
	idf.py -p COM5 monitor

config:
	@idf.py menuconfig

web/page/dist/index.html:
	@make -C main/web/page --no-print-directory
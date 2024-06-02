COM_PORT=COM5

all:
	@idf.py build

full: web/page/dist/index.html
	@idf.py build

f: full
	idf.py -p $(COM_PORT) flash

fm: f
	idf.py -p $(COM_PORT) monitor

m:
	idf.py -p $(COM_PORT) monitor

config:
	@idf.py menuconfig

clean:
	idf.py fullclean

web/page/dist/index.html:
	@make -C main/web/page --no-print-directory
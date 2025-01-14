include comport_sel.mk

.PHONY: build

build:  web/page/dist/index.html
	@idf.py build

ff: build
	idf.py -p $(COM_PORT) flash

f: build
	idf.py -p $(COM_PORT) flash app

fm: f
	idf.py -p $(COM_PORT) monitor

m:
	idf.py -p $(COM_PORT) monitor

clean:
	idf.py fullclean

web/page/dist/index.html:
	@make -C main/web/page --no-print-directory

ota: build
	curl --request POST --data-binary "@build/lp.bin" http://192.168.4.1/update
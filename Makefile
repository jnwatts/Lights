.PHONY: build
all: build


IDF_PORT ?= /dev/ttyUSB1


build menuconfig:
	idf.py $@

flash monitor:
	idf.py -p $(IDF_PORT) $@

clean:
	-rm -Rf build
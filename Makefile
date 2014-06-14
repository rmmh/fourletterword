CFLAGS=-O2 -Wall -Wextra
CPPFLAGS += -MD -MP

SRC = $(wildcard *.c)

all: generate arduino

generate: generate.o decode.o

list.gen.h: generate
	./generate -g

arduino: arduino.c decode.h decode.c list.gen.h
	avr-gcc -D__AVR_ATmega16__ -Wall -Os $< -o $@

-include $(SRC:%.c=%.d)

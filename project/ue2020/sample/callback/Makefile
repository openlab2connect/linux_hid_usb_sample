src = $(wildcard *.c)
obj = $(src:.c=.o)
CC= gcc

CFLAGS = -Wno-unused-function -I../../api/include
LDFLAGS = -lusb-1.0 -ltm -lpthread

callback: $(obj)
	$(CC) -o $@ $^ $(LDFLAGS)

.PHONY: clean
clean:
	rm -f $(obj) callback

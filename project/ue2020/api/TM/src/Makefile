src = $(wildcard *.c)
obj = $(src:.c=.o)
# dep = $(obj:.o=.d)

CC= gcc
alib = libtm.a

CFLAGS = -I../../include -Wall
LIB_DIR = ../../lib

$(alib): $(obj)
	$(AR) rcs -o $(LIB_DIR)/$@ $^

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

.PHONY: clean
clean:
	rm -f $(obj) ../../lib/*.a
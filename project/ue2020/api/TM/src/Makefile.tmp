src = $(wildcard *.c)
obj = $(src:.c=.o)
# dep = $(obj:.o=.d)

CC= gcc
alib = libtm.a

CFLAGS = -I../../include -Wall -fPIC -m64
LIB_DIR = ../../lib

$(alib): $(obj)
	$(AR) rcs -o $(LIB_DIR)/$@ $^

%.o: %.c dirmake
	$(CC) $(CFLAGS) -o $@ -c $<

dirmake:
	@mkdir -p $(LIB_DIR)

.PHONY: clean
clean:
	rm -f $(obj) ../../lib/*.a
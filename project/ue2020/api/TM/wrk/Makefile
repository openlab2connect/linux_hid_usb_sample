src = $(wildcard ./../src/*.c)
obj = $(src:.c=.o)
# dep = $(obj:.o=.d)

local_path = $(shell pwd)
CC= gcc
alib = libtm.a
slib = libtm.so

CFLAGS = -I../../include -Wall -fPIC -m64
LIB_DIR = ../../lib

$(slib): $(obj)
	$(CC) --shared -o $(LIB_DIR)/$@ $^

%.o: %c dirmake
	$(CC) $(CFLAGS) -o $@ -c $<

dirmake:
	@mkdir -p $(LIB_DIR)

.PHONY: clean
clean:
	rm -f $(obj) ../../lib/*.so
	sudo rm /usr/lib/x86_64-linux-gnu/$(slib)

.PHONY: install
install:
	# @echo $(local_path)
	sudo ln -s $(local_path)/../../lib/$(slib) /usr/lib/x86_64-linux-gnu/$(slib)
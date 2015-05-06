CROSS ?= powerpc64le-linux-gnu
CC = $(CROSS)-gcc

ARCH_FLAGS = -msoft-float -mpowerpc64 -mabi=elfv2 -mlittle-endian -mno-strict-align -mno-multiple -mno-pointers-to-nested-functions -mcmodel=large

all: hello_kernel

%.o: %.S asm-utils.h
	$(CC) $(ARCH_FLAGS) -c -I./ -o $@ $<

%.o: %.c
	$(CC) $(ARCH_FLAGS) -S -I./ $< -o $@.s
	$(CC) $(ARCH_FLAGS) -c -I./ -o $@ $<

hello_kernel: hello_kernel.o main.o
	$(CC) $(ARCH_FLAGS)  -Wl,--build-id=none -Wl,--EL -T hello_kernel.ld -ffreestanding -nostdlib -Ttext=0x20010000 -lgcc -o $@ $^

clean:
	$(RM) hello_kernel *.o *.o.s
.PHONY: clean

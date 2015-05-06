CROSS ?= powerpc64le-linux-gnu
CC = $(CROSS)-gcc

ARCH_FLAGS = -mpowerpc64 -mabi=elfv2 -mlittle-endian -mno-strict-align -mno-multiple

all: hello_kernel

%.o: %.S asm-utils.h
	$(CC) $(ARCH_FLAGS) -c -I./ -o $@ $<

%.o: %.c
	$(CC) $(ARCH_FLAGS) -c -I./ -o $@ $<

hello_kernel: hello_kernel.o main.o
	$(CC) $(ARCH_FLAGS)  -Wl,--build-id=none -Wl,--EL -T hello_kernel.ld -ffreestanding -nostdlib -Ttext=0x20010000 -lgcc -o $@ $^

clean:
	$(RM) hello_kernel *.o *.d
.PHONY: clean

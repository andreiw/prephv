CROSS ?= powerpc64le-linux-gnu
CC = $(CROSS)-gcc

ARCH_FLAGS = -msoft-float -mpowerpc64 -mcpu=power8 -mtune=power8 -mabi=elfv2 -mlittle-endian -mno-strict-align -mno-multiple \
             -mno-pointers-to-nested-functions -mcmodel=large -fno-builtin -fno-stack-protector \
             -I./ -Wall -Werror

OBJ =  entry.o main.o console.o string.o fdt.o fdt_strerror.o fdt_ro.o exc-vecs.o exc.o time.o mmu.o
NAME =  ppc64le_hello

all: $(NAME)

test: $(NAME) skiboot/skiboot.lid
	./run_ppc64le_hello.sh

skiboot/skiboot.lid:
	git submodule update --init skiboot
	SKIBOOT_VERSION=foo make -C skiboot

-include $(OBJ:.o=.d)

# Default sed regexp - multiline due to syntax constraints
define sed-command
	"/^->/{s:->#\(.*\):/* \1 */:; \
	s:^->\([^ ]*\) [\$$#]*\([-0-9]*\) \(.*\):#define \1 \2 /* \3 */:; \
	s:^->\([^ ]*\) [\$$#]*\([^ ]*\) \(.*\):#define \1 \2 /* \3 */:; \
	s:->::; p;}"
endef

asm-offset.h: asm-offset.s
	sed -ne $(sed-command) $< > $@

asm-offset.s: asm-offset.c
	$(CC) $(ARCH_FLAGS) -S $< -o $@

%.o: %.S
	$(CC) $(ARCH_FLAGS) -D__ASSEMBLY__ -c -MMD -o $@ $<
	@cp -f $*.d $*.d.tmp
	@sed -e 's/.*://' -e 's/\\$$//' < $*.d.tmp | fmt -1 | \
	  sed -e 's/^ *//' -e 's/$$/:/' >> $*.d
	@rm -f $*.d.tmp

%.o: %.c
	$(CC) $(ARCH_FLAGS) -S $< -o $@.s
	$(CC) $(ARCH_FLAGS) -c -MMD -o $@ $<
	@cp -f $*.d $*.d.tmp
	@sed -e 's/.*://' -e 's/\\$$//' < $*.d.tmp | fmt -1 | \
	  sed -e 's/^ *//' -e 's/$$/:/' >> $*.d
	@rm -f $*.d.tmp

$(NAME): asm-offset.h $(OBJ)
	$(CC) $(ARCH_FLAGS)  -Wl,--build-id=none -Wl,--EL -T ld.script -ffreestanding -nostdlib -Ttext=0x20010000 -lgcc -o $@ $^

clean:
	$(RM) $(NAME) *.o *.o.s *.d asm-offset.h asm-offset.s
cleaner: clean
	git submodule deinit -f skiboot
.PHONY: clean cleaner

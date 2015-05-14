CROSS ?= powerpc64le-linux-gnu
CC = $(CROSS)-gcc

ARCH_FLAGS = -msoft-float -mpowerpc64 -mabi=elfv2 -mlittle-endian -mno-strict-align -mno-multiple -mno-pointers-to-nested-functions -mcmodel=large -fno-builtin -fno-stack-protector

OBJ =  entry.o main.o printf.o
NAME =  ppc64le_hello

all: $(NAME)

test: $(NAME) skiboot/skiboot.lid
	./run_ppc64le_hello.sh

skiboot/skiboot.lid:
	git submodule update --init skiboot
	SKIBOOT_VERSION=foo make -C skiboot

-include $(OBJ:.o=.d)

%.o: %.S
	$(CC) $(ARCH_FLAGS) -D__ASSEMBLY__ -c -MMD -I./ -o $@ $<
	@cp -f $*.d $*.d.tmp
	@sed -e 's/.*://' -e 's/\\$$//' < $*.d.tmp | fmt -1 | \
	  sed -e 's/^ *//' -e 's/$$/:/' >> $*.d
	@rm -f $*.d.tmp

%.o: %.c
	$(CC) $(ARCH_FLAGS) -c -MMD -I./ -o $@ $<
	$(CC) $(ARCH_FLAGS) -S -I./ $< -o $@.s
	@cp -f $*.d $*.d.tmp
	@sed -e 's/.*://' -e 's/\\$$//' < $*.d.tmp | fmt -1 | \
	  sed -e 's/^ *//' -e 's/$$/:/' >> $*.d
	@rm -f $*.d.tmp

$(NAME): $(OBJ)
	$(CC) $(ARCH_FLAGS)  -Wl,--build-id=none -Wl,--EL -T ld.script -ffreestanding -nostdlib -Ttext=0x20010000 -lgcc -o $@ $^

clean:
	$(RM) $(NAME) *.o *.o.s *.d
cleaner: clean
	git submodule deinit -f skiboot
.PHONY: clean cleaner

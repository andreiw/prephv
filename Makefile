CROSS ?= powerpc64le-linux-gnu
CC = $(CROSS)-gcc

ARCH_FLAGS = -msoft-float -mpowerpc64 -mabi=elfv2 -mlittle-endian -mno-strict-align -mno-multiple -mno-pointers-to-nested-functions -mcmodel=large
OBJ =  entry.o main.o
NAME =  ppc64le_hello

all: $(NAME)

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
.PHONY: clean

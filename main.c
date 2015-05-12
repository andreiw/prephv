#include <types.h>
#include <endian.h>

#define HELLO_MAMBO "Hello Mambo!\n"
#define HELLO_OPAL "Hello OPAL!\n"

void
c_main(u64 dt_base)
{
	u64 len = cpu_to_be64(sizeof(HELLO_OPAL));

	mambo_write(HELLO_MAMBO, sizeof(HELLO_MAMBO));
	opal_write(0, &len, HELLO_OPAL);
	mambo_write(HELLO_MAMBO, sizeof(HELLO_MAMBO));
	opal_write(0, &len, HELLO_OPAL);
}

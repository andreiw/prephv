#ifndef _LIBFDT_ENV_H
#define _LIBFDT_ENV_H

#include <defs.h>
#include <types.h>
#include <string.h>
#include <stdarg.h>
#include <endian.h>

#define fdt32_to_cpu(x) be32_to_cpu(x)
#define cpu_to_fdt32(x) fdt32_to_cpu(x)

#define fdt64_to_cpu(x) be64_to_cpu(x)
#define cpu_to_fdt64(x) fdt64_to_cpu(x)

#endif /* _LIBFDT_ENV_H */

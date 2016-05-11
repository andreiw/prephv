prephv - because I wasted my 90's playing with LEGOs.
=====================================================

This is mostly a huge ugly hack, derived from my
ppc64le_hello code. The running philosophy here is
to throw things together late at night with my family
asleep and see how far I get without a real design
or without a real desire to implement boring things
like IDE (*sigh*) emulation. PowerPC things though
are considered fun and crucial...anyway...

It's a 64-bit LE ``hypervisor'', running a single 32-bit
LE VM, with the intention of modelling some as of yet
unknown PReP machine...eventually hopefully enough to BSOD
inside the Windows NT 4.0 PowerPC kernel.

PReP is a long dead specification for consumer-ish PowerPC
servers and workstations, that basically amounted to a
pre-ACPI PC with a PowerPC CPU. Some PReP machines ran
Windows NT 4.0. Most ran AIX, an obscure Solaris port, or
gave Linux hackers many fun restless nights.

Relive '96... I spent that July basking on the beaches of
Cagliari. Somewhere in a dusty office in Seattle people
were dealing with stuff like this:

    Open Firmware ARC Interface  Version 3.0 (Jul 12 1996 - 18:46:44)
    Couldn't claim SYSTEM PARAMETER BLOCK
    Program complete - please reboot.

Status
------

Today I fake out enough of OpenFirware client interface to
successfully run the VENEER.EXE ARC shim and hand off to
SETUPLDR. Not much else. Specifically, the CPU VM state is
not modelled quite well (or at all...?).

![ARC veneer image](/docs/veneer.png?raw=true "In ARC menu")
![setupldr image](/docs/setupldr.png?raw=true "In SETUPLDR")

The "disk" is passed as initrd. This is nowhere near
being able to run any portion of NT kernel.

Building
--------

You will need a LE 64-bit toolchain to build (i.e. ppc64le-linux).
A good source of toolchains is kernel.org. For building skiboot
(for testing) you will also need the BE 64-bit toolchain
(i.e. powerpc64-linux).

Skiboot is pulled-in as a submodule. Prephv is a skiboot payload.
So hypothetically it could run on a real Power8 box.

You will need Benjamin Herrenschmidt's PowerNV QEMU tree. Actually
I suggest my fork of that tree, which might contain useful
fixes that aren't yet present elsewhere. This is here:
https://github.com/andreiw/ozbenh-qemu

Good directions for QEMU/PowerNV:
https://www.flamingspork.com/blog/2015/08/28/running-opal-in-qemu-the-powernv-platform/

    $ make

Running
-------

You now need an image.fat that contains at least the following
files from the NT 4.0 CD, which are obviously not distributable:
- \veneer.exe    from /PPC/VENEER.EXE
- \osloader.exe  from /PPC/SETUPLDR

I've used FAT16. Other FAT types are untested. Good luck.

    $ PPC64QEMU=/path/to/BenH/powernv/qemu make test

TODO
----

- Everything
- Threads (+implement an interactive monitor/debugger)

Resources
---------

Useful stuff:
- PowerISA_V2.07_PUBLIC.pdf             - Book 3S
- ABI53BitOpenPOWER_21July2014_pub.pdf  - ELFv2 ABI used for PPC64LE
- P8_um_external_v1.1_2015JAN29_pub.pdf - POWER8 User Manual

Contact Info
------------

Andrei Warkentin (andrey.warkentin@gmail.com).

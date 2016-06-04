PRePHV - Virtualizing PReP on PowerNV
=====================================

This is mostly a huge ugly hack, derived from my
ppc64le_hello code. The running philosophy here is
to throw things together late at night with my family
asleep and see how far I get without a real design
or without a real desire to implement boring things
like IDE (*sigh*) emulation. PowerPC things though
are considered fun and crucial...anyway...

It's a 64-bit LE ``hypervisor'', running a single 32-bit
LE VM, with the intention of modelling some simple
PReP machine...eventually hopefully enough to BSOD
inside the Windows NT 4.0 PowerPC kernel. Today we pretend
to be a Power Series 440 (6015) aka Sandalfoot, albeit
with a 604 instead of a 601. This was an arbitrary
choice, as I am nowhere near modelling any real I/O.

PReP is a long dead specification for consumer-ish PowerPC
servers and workstations, that basically amounted to a
pre-ACPI PC with a PowerPC CPU. Some PReP machines ran
Windows NT 4.0. Most ran AIX, an obscure Solaris port, or
gave Linux hackers many fun restless nights.

Relive '96... I spent that July basking on the beaches of
Cagliari. Somewhere in a dusty office in Seattle people
were dealing with stuff like:

    Open Firmware ARC Interface  Version 3.0 (Jul 12 1996 - 18:46:44)
    Couldn't claim SYSTEM PARAMETER BLOCK
    Program complete - please reboot.

Status
------

Today I fake out enough of OpenFirware client interface to
successfully run the VENEER.EXE ARC shim and hand off to
SETUPLDR. SETUPLDR is capable of loading the setup
information file, NT kernel, HAL, NLS data,
setupdd.sys (the text portion of setup), the early
boot drivers and hands off control to NTOSKRNL.
Not much else. Specifically, the CPU VM state is
not modelled quite well (or at all...?), so we
panic on the kernel's attempt to set the page
table base register (SDR1).

![ARC veneer image](/docs/veneer.png?raw=true "In ARC menu")
![setupldr Crashing in NTOSKRNL](/docs/ntoskrnl_crash.png?raw=true "Crashing in NTOSKRNL")

The "disk" is passed as initrd. The main achievement
of crashing in NT kernel proper has been unlocked, but
now I want to see some real output from the kernel to
get a better sense of satisfaction ;-). Sadly, this
is going to be extremely painful, because unlike
the veneer and SETUPLDR, the symbols are stripped
for the proper OS bits (well, separated into PDB).

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
- \ntkrnlmp.exe  from /PPC/NTKRNLMP.EXE
- \halppc.dll    from /PPC/HALPPC.DLL
- \txtsetup.sif  from /PPC/TXTSETUP.SIF
- \*.nls         from /PPC/*.NLS
- \*.sys         from /PPC/*.SYS

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

        # qemu -kernel loads the kernel at 0x80000000
        # and causes each hart (i.e. CPU) to jump there.
        # kernel.ld causes the following code to
        # be placed at 0x80000000.
.section .text
.global _entry
_entry:
        # 重点关注：创建 C 运行环境
        # set up a stack for C.
        # stack0 is declared in start.c,
        # with a 4096-byte stack per CPU.
        # sp = stack0 + (hartid * 4096)
        # la addr -> rd
        # la stack0 -> sp
        la sp, stack0
        # li imm -> rd
        # a0 -> function argument0 / return value 0
        # li 1024 * 4 -> a0
        li a0, 1024*4
        csrr a1, mhartid
        addi a1, a1, 1
        mul a0, a0, a1
        add sp, sp, a0
        # jump to start() in start.c
        call start
spin:
        j spin


        # sp = stack0 = 4096 * NCPU = 4096 * 8
        # a0 = 4096
        # a1 = mhartid (寄存器，指示运行当前代码的硬件线程)
        # a1 = a1 +1 = 1 (?)
        # a0 = a0 * a1
        # sp = sp + a0

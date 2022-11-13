## alarm 实验要求

此实验要求向 xv6 提供两个系统调用 `sigalarm()` 和 `sigreturn()`。

`sigalarm(int ticks, void(*handler)())` 指示 xv6 kernel 每过 ticks 个 CPU time 执行一次 `handler()`；在 `handler()` 结束前需要调用 `sigreturn()`。

结束 `handler()` 的周期执行的方式是调用 `sigalarm(0,0)`。

## 实验思路

首先，需要了解 xv6 中是如何定义 CPU time。在 `trap.c` 中可以看到声明了一个 `ticks`，猜测用来记录自启动后的 CPU time，结合 labs 的提示信息，在 `usertrap()` 中通过 `printf()` 打印到控制台，可以发现在没有任何动作的情况下，会远远不断地输出其值。

结合 trap 的三种触发情况，可以判断此时是由 timer interrupt 触发的，也就是说，这个 `ticks` 可以作为周期性调用 `handler()` 的触发依据。

通过记录 `sigalarm()` 的调用时间以及定义的调用周期，在 `usertrap()` 中与 `ticks` 做比较，判断在 timer interrupt 后是否调用 `handler()`。

了解了 `handler()` 的调用时机后，需要解决如何调用。依据 traps 机制，PC 从用户态转移到内核态，用户态的上下文被保存入当前进程的 trapframe 中，包括各种寄存器以及用户态的程序计数器以及栈帧指针等；在回到用户态时恢复上下文。

也就是说，可以在内核态中通过修改 trapframe 备份的用户态程序计数器，使其指向 `handler()` 的地址，回到用户态后直接调用。

然而仅是如此操作，在 `handler()` 调用结束后势必会破坏原本用户级程序的上下文环境，所以还需要在调用 `handler()` 前再备份 trapframe 的数据，通过 `sigreturn()` 返回到内核态后恢复 trapframe，使其正常回到之前的用户级程序。

最终通过 `sigalarm(0,0)` 结束周期性执行。

## 调用流程

1. 用户级程序中指定调用周期和 `handler()`，作为参数调用 `sigalarm()`；
2. 系统调用 `sigalarm()` 通过 ecall 指令进入内核态；
3. 在内核态中设置相关参数，如调用周期、`handler()` 和监控计数器；
4. 回到用户态，继续执行用户级程序；
5. 用户级程序周期性触发 timer interrupt，进入内核态；
6. `usertrap()` 在关于 timer interrupt 的相关代码中检查调用周期是否到期；
7. 轮转到调用周期时，备份当前进程的 trapframe，重置计数器，指定 epc 指向 `handler()` 的地址；
8. 退出内核态回到用户态，执行 `handler()`；
9. `handler()` 最后会调用 `sigreturn()` 回到内核态；
10. 恢复备份到 trapframe 中，准备下一次调用，回到用户态继续执行用户级程序；
11. 周期性执行 `handler()`；
12. 通过调用 `sigalarm(0,0)` 结束周期性执行。

## 注意要点

备份 trapframe 时，一开始决定备份 caller register 和 epc。这样在调用 `handler()` 后，可以回到用户级程序，但是因为没有备份栈帧指针，导致函数栈被破坏。

又考虑到这样挨个备份寄存器的操作很蠢笨，于是直接分配一个 page 用于完整备份 trapframe。

考虑到 `handler()` 调用过程中的会发生再调用的情况，在进程中用指针记录用于备份的 page 地址，每次调用 `handler()` 前分配 page，并在 `sigreturn()` 中释放 page，这样可以通过根据指针是否为 0 来判断是否处于 `handler()` 调用中。

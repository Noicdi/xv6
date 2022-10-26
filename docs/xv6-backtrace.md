```
fp   -> frame pointer
o_fp -> old fp


        |                            |
        |  old caller's stack frame  |
c_fp1->	+----------------------------+
        |       return  address      |
        |         previous fp        |
        |       saved registers      |
        |       local variables      |
        |           .......          |
c_fp2->	+----------------------------+
        |       return  address      |
        |            c_fp1           |
        |       saved registers      |
        |       local variables      |
        |           .......          |
c_fp3->	+----------------------------+
        |       return  address      |
        |            c_fp2           |
        |       saved registers      |
        |       local variables      |
        |           .......          |
        +----------------------------+
        |                            |
```
以帧指针 `c_fp3` 为例，首先通过 `r_rp()` 获取当前栈帧的帧指针，向下移动 8 个字节得到 return address，再往下移动 8 个字节得到 上一个栈帧的帧指针，转移并重复，直到所有栈帧被遍历。

如何确定当前栈帧是最后（最高）的栈帧？[所有的栈帧应该都在同一个 page 中](https://pdos.csail.mit.edu/6.828/2022/labs/traps.html#:~:text=A%20useful%20fact%20is%20that%20the%20memory%20allocated%20for%20each%20kernel%20stack%20consists%20of%20a%20single%20page%2Daligned%20page%2C%20so%20that%20all%20the%20stack%20frames%20for%20a%20given%20stack%20are%20on%20the%20same%20page.)，如此一来只需要判断当前帧指针是否处在同一个 page 即可。

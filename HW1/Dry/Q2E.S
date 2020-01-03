# frame opening
pushl %ebp
movl %ebp, %esp
subl $16, $esp
pushl %esi
pushl %edi
pushl %ebx
# shifting logic




















# mem args -> regs -> frame
movl 0x0, -4(%ebp)
movl 8(%ebp), %ebx
movl %ebx, -8(%ebp)
movl 0x0, -12(%ebp)
movl 12(%ebp), %ebx
movl %ebx, -16(%ebp)
# shifting logic
andS %eflags
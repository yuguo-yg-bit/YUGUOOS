.code64
.section .text

.macro SAVE_REGS
    pushq %rax
    pushq %rcx
    pushq %rdx
    pushq %rbx
    pushq %rbp
    pushq %rsi
    pushq %rdi
    pushq %r8
    pushq %r9
    pushq %r10
    pushq %r11
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15
.endm

.macro RESTORE_REGS
    popq %r15
    popq %r14
    popq %r13
    popq %r12
    popq %r11
    popq %r10
    popq %r9
    popq %r8
    popq %rdi
    popq %rsi
    popq %rbp
    popq %rbx
    popq %rdx
    popq %rcx
    popq %rax
.endm

.macro ISR_NOERR num
.globl isr\num
isr\num:
    cli
    pushq $0
    pushq $\num
    jmp isr_common
.endm

.macro ISR_ERR num
.globl isr\num
isr\num:
    cli
    pushq $\num
    jmp isr_common
.endm

.macro IRQ num, vector
.globl irq\num
irq\num:
    cli
    pushq $0
    pushq $\vector
    jmp irq_common
.endm

ISR_NOERR 0
ISR_NOERR 1
ISR_NOERR 2
ISR_NOERR 3
ISR_NOERR 4
ISR_NOERR 5
ISR_NOERR 6
ISR_NOERR 7
ISR_ERR   8
ISR_NOERR 9
ISR_ERR   10
ISR_ERR   11
ISR_ERR   12
ISR_ERR   13
ISR_ERR   14
ISR_NOERR 15
ISR_NOERR 16
ISR_ERR   17
ISR_NOERR 18
ISR_NOERR 19
ISR_NOERR 20
ISR_NOERR 21
ISR_NOERR 22
ISR_NOERR 23
ISR_NOERR 24
ISR_NOERR 25
ISR_NOERR 26
ISR_NOERR 27
ISR_NOERR 28
ISR_NOERR 29
ISR_ERR   30
ISR_NOERR 31

IRQ 0, 32
IRQ 1, 33
IRQ 2, 34
IRQ 3, 35
IRQ 4, 36
IRQ 5, 37
IRQ 6, 38
IRQ 7, 39
IRQ 8, 40
IRQ 9, 41
IRQ 10, 42
IRQ 11, 43
IRQ 12, 44
IRQ 13, 45
IRQ 14, 46
IRQ 15, 47

ISR_NOERR 128
ISR_NOERR 129

isr_common:
    SAVE_REGS
    movq %rsp, %rdi
    call isr_handler
    movq sched_next_proc(%rip), %rax
    testq %rax, %rax
    jz 1f
    movq 184(%rax), %rsp
    movq $0, sched_next_proc(%rip)
1:
    RESTORE_REGS
    addq $16, %rsp
    iretq

irq_common:
    SAVE_REGS
    movq %rsp, %rdi
    call irq_handler
    movq sched_next_proc(%rip), %rax
    testq %rax, %rax
    jz 1f
    movq 184(%rax), %rsp
    movq $0, sched_next_proc(%rip)
1:
    RESTORE_REGS
    addq $16, %rsp
    iretq
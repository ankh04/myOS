#include "debug.h"
#include "kernel/print.h"
#include "interrupt.h"

// 打印文件名，行号，函数，和panic条件
// 并使程序悬停
void panic_spin(char *filename, int line, const char *func, const char *condition)
{
    intr_disable();
    put_str("\n\n-------- error ---------\n");
    put_str("filename:");
    put_str(filename);
    put_str("\n");
    put_str("line:0x");
    put_int(line);
    put_str("\n");
    put_str("func:");
    put_str((char *)func);
    put_str("\n");
    put_str("condition:");
    put_str((char *)condition);
    put_str("\n");
    while (1)
        ;
}
#include "interrupt.h"
#include "stdint.h"
#include "global.h"
#include "io.h"
#include "kernel/print.h"

#define IDT_DESC_CNT 0x21 // 目前支持的中断数
#define PIC_M_CTRL 0x20
#define PIC_M_DATA 0x21
#define PIC_S_CTRL 0xa0
#define PIC_S_DATA 0xal
/**
 *  中断门描述结构
 */
struct gate_desc
{
    uint16_t func_offset_low_word;
    uint16_t selector;
    uint8_t dcount;
    uint8_t attribute;
    uint16_t func_offset_high_word;
};

static void make_idt_desc(struct gate_desc *p_gdesc, uint8_t attr, intr_handler function);
static struct gate_desc idt[IDT_DESC_CNT]; // 中断门描述符数组

char *intr_name[IDT_DESC_CNT];        // 用于保存异常的名字
intr_handler idt_table[IDT_DESC_CNT]; // 定义中断函数的数组

extern intr_handler intr_entry_table[IDT_DESC_CNT]; // kernel.s 中定义的intr_entry_table

// 配置8259A
static void pic_init(void)
{
    // 主片初始化
    outb(PIC_M_CTRL, 0x11); // ICW1
    outb(PIC_M_DATA, 0x20); // ICW2
    outb(PIC_M_DATA, 0x04); // ICW3
    outb(PIC_M_DATA, 0x01); // ICW3
    // 从片初始化
    outb(PIC_S_CTRL, 0x11); // ICW1
    outb(PIC_S_DATA, 0x28); // ICW1
    outb(PIC_S_DATA, 0x02); // ICW1
    outb(PIC_S_DATA, 0x01); // ICW1

    // 打开主片上的IR0
    outb(PIC_M_DATA, 0xfe);
    outb(PIC_S_DATA, 0xff);

    put_str("   pic_init done\n");
}

static void make_idt_desc(struct gate_desc *p_gdesc, uint8_t attr, intr_handler function)
{
    p_gdesc->func_offset_low_word = (uint32_t)function & 0x0000FFFF;
    p_gdesc->selector = SELECTOR_K_CODE;
    p_gdesc->dcount = 0;
    p_gdesc->attribute = attr;
    p_gdesc->func_offset_high_word = ((uint32_t)function & 0xFFFF0000) >> 16;
}

static void idt_desc_init(void)
{
    int i;
    for (i = 0; i < IDT_DESC_CNT; i++)
    {
        // 逐个注册0-32号中断函数
        make_idt_desc(&idt[i], IDT_DESC_ATTR_DPL0, intr_entry_table[i]);
    }
    put_str("   idt_desc_init done\n");
}

// 中断处理函数， 参数是中断向量号
static void general_intr_handler(uint8_t vec_nr)
{
    if (vec_nr == 0x27 | vec_nr == 0x2f)
    {
        return;
    }
    put_str("int vector : 0x");
    put_int(vec_nr);
    put_char('\n');
}

static void exception_init(void)
{
    int i;
    for (i = 0; i < IDT_DESC_CNT; i++)
    {
        idt_table[i] = general_intr_handler;
        intr_name[i] = "unknown"; // 先统一赋值为 unknown
    }
    intr_name[0] = "#DE Divide Error";
    intr_name[1] = "#DB Debug Exception";
    intr_name[2] = "NMI Interrupt";
    intr_name[3] = "#BP Breakpoint Exception";
    intr_name[4] = "#OF Overflow Exception";
    intr_name[5] = "#BR BOUND Range Exceeded Exception";
    intr_name[6] = "#UD Invalid Opcode Exception";
    intr_name[7] = "#NM Device Not Available Exception";
    intr_name[8] = "#DF Double Fault Exception";
    intr_name[9] = "Coprocessor Segment Overrun";
    intr_name[10] = "#TS Invalid TSS Exception";
    intr_name[11] = "#NP Segment Not Present";
    intr_name[12] = "#SS Stack Fault Exception";
    intr_name[13] = "#GP General Protection Exception";
    intr_name[14] = "#PF Page-Fault Exception";
    intr_name[16] = "#MF 0x87 FPU Floating-Point Error";
    intr_name[17] = "#AC Alignment Check Exception";
    intr_name[18] = "#MC Machine-Check Exception";
    intr_name[19] = "#XF SIMD Floating-Point Exception";
}

void idt_init()
{
    put_str("idt_init start\n");
    idt_desc_init(); // 初始化中断描述符
    exception_init();
    pic_init(); // 初始化8259A

    // 加载idt
    // 把idt地址变成一个48位的
    uint64_t idt_operand = ((sizeof(idt) - 1) | ((uint64_t)((uint32_t)idt << 16)));
    asm volatile("lidt %0"
                 :
                 : "m"(idt_operand));
    put_str("idt_init done\n");
}
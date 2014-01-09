#include "setup.h"

#define KERNEL_OFFSET	0x00060000
#define KERNEL_SIZE		0x200000
#define Image_HEAD		64

void nand_read(unsigned long *dest, unsigned long *src, int len);
static struct tag *params;

int strlen(char *str)
{
	int i = 0;

	while(str[i])
		i++;
	return i;
}

void strcpy(char *dest, char *src)
{
	while((*dest++ = *src++) != '\0');
}

/* ------------------------ tags ------------------------ */
void setup_start_tag(void)
{
	params = (struct tag *)0x30001000;

	params->hdr.tag = ATAG_CORE;
	params->hdr.size = tag_size(tag_core);

	params->u.core.flags = 0;
	params->u.core.pagesize = 0;
	params->u.core.rootdev = 0;

	params = tag_next(params);
}

void setup_mem_tag(void)
{
	params->hdr.tag = ATAG_MEM;
	params->hdr.size = tag_size(tag_mem32);

	/* 片1 */
	params->u.mem.start = 0x30000000;
	/* 256*1024*1024 -> 10^(4*7) -> 0x10000000 */
	params->u.mem.size = 0x10000000;
	params = tag_next(params);

	/* 片2 */
	params->u.mem.start = 0x40008000;
	params->u.mem.size = 0x10000000;
	params = tag_next(params);
}

void setup_cml_tag(char *cmdline)
{
	int len = strlen(cmdline) + 1;

	params->hdr.tag = ATAG_CMDLINE;
	params->hdr.size =
		(sizeof(struct tag_header) + len + 3) >> 2;
	strcpy(params->u.cmdline.cmdline, cmdline);

	params = tag_next(params);
}

void setup_end_tag(void)
{
	params->hdr.tag = ATAG_NONE;
	params->hdr.size = 0;
}

/* ------------------------ uart ------------------------ */
/*UART registers*/
#define GPHCON       (*(volatile unsigned long *)0x56000070)
#define GPHDAT       (*(volatile unsigned long *)0x56000074)
#define GPHUP        (*(volatile unsigned long *)0x56000078)

#define ULCON0       (*(volatile unsigned long *)0x50000000)
#define UCON0        (*(volatile unsigned long *)0x50000004)
#define UFCON0       (*(volatile unsigned long *)0x50000008)
#define UMCON0       (*(volatile unsigned long *)0x5000000c)
#define UTRSTAT0     (*(volatile unsigned long *)0x50000010)
#define UTXH0        (*(volatile unsigned char *)0x50000020)
#define URXH0        (*(volatile unsigned char *)0x50000024)
#define UBRDIV0      (*(volatile unsigned long *)0x50000028)

#define TXD0READY    (1<<2)
#define RXD0READY    (1)

#define PCLK           50000000 //start.S中设置PCLK为50MHz
#define UART_CLK       PCLK     //UART0的时钟源设为PCLK
#define UART_BAUD_RATE 115200   //波特率
#define UART_BRD       ((UART_CLK / (UART_BAUD_RATE * 16)) - 1)

void uart_init(void)
{
    GPHCON  |= 0xa0;    // GPH2,GPH3用作TXD0,RXD0
    GPHUP   = 0x0c;     // GPH2,GPH3内部上拉

    ULCON0  = 0x03;     // 8N1(8个数据位，无较验，1个停止位)
    UCON0   = 0x05;     // 查询方式，UART时钟源为PCLK
    UFCON0  = 0x00;     // 不使用FIFO
    UMCON0  = 0x00;     // 不使用流控
    UBRDIV0 = UART_BRD; // 波特率为115200
}

void putc(unsigned char c)
{
    /* 等待，直到发送缓冲区中的数据已经全部发送出去 */
    while (!(UTRSTAT0 & TXD0READY));
    
    /* 向UTXH0寄存器中写入数据，UART即自动将它发送出去 */
    UTXH0 = c;
}

void puts(char *str)
{
	int i = 0;
	while(str[i])
		putc(str[i++]);
}

/*
 * 接收字符
 */
char getc(void)
{
    /* 等待，直到接收缓冲区中的有数据 */
    while (!(UTRSTAT0 & RXD0READY));
    
    /* 直接读取URXH0寄存器，即可获得接收到的数据 */
    return URXH0;
}

/* ------------------------ main ------------------------ */
int main(void)
{
	void (*kernel_entry)(int zero, int arch, unsigned long params);

	/* 0. 设置串口 */
	uart_init();

	puts("cp kernel to ram\n\r");

	/* 1. 从Nand Flash里面把内核读到内存,supervivi->q->part show */
	nand_read((unsigned long *)0x30008000, (unsigned long *)KERNEL_OFFSET + Image_HEAD, KERNEL_SIZE);

	/* 2. 设置参数 */
	setup_start_tag();
	setup_mem_tag();
	setup_cml_tag("noinitrd root=/dev/mtdblock3 init=/linuxrc console=ttySAC0");
	setup_end_tag();

	puts("boot kernel\n\r");

	/* 3. 跳转执行 */
	kernel_entry = (void (*)(int, int, unsigned long))0x30008000;
	/* 参数二为机器码,参数三根据start_tag来写 */
	kernel_entry(0, 362, 0x30001000);

	puts("error\n\r");

	/* 如何正常启动,不会到达此处 */
	return -1;
}

void nand_read(unsigned long *dest, unsigned long *src, int len);

int isBootFromNor(void)
{
	volatile int *p = (volatile int *)0;

	int val = *p;
	*p = 0x98765432;
	if (*p == 0x98765432) {
		/* 说明是从Nand拷贝到sram执行 */
		*p = val;
		return 0;
	} else {
		/* Nor可读不可直接写 */
		return 1;
	}
}

void cp2Ram(unsigned long *src, unsigned long *dest, int len)
{
	int i = 0;

	if (isBootFromNor()) {
		for (; i < len; i++)
			dest[i] = src[i];
	} else {
		/* 在cp2ram之前调用 */
		//nand_init();
		nand_read(dest, src, len);
	}
}

void cleanBss(void)
{
	extern int __bss_start, __bss_end;
	int *p = &__bss_start;

	for (; p < &__bss_end; p++)
		*p = 0;
}

#define NFCONF	(*((volatile unsigned long *)0x4E000000))
#define NFCONT	(*((volatile unsigned long *)0x4E000004))
#define NFCMMD	(*((volatile unsigned char *)0x4E000008))
#define NFADDR	(*((volatile unsigned char *)0x4E00000C))
#define NFDATA	(*((volatile unsigned char *)0x4E000010))
#define NFSTAT	(*((volatile unsigned char *)0x4E000020))

#define PAGE_SIZE	2048

void nand_init()
{
#define TACLS	0
#define TWRPH0	1
#define TWRPH1	0
	/* 根据K9F2G08U0B设置时序 */
	NFCONF = (TACLS<<12) | (TWRPH0<<8) | (TWRPH1<<4);
	/* 使能NAND Flash控制器,初始化ECC,禁止片选 */
	NFCONT = (1<<4) | (1<<1) | (1<<0);
}

void nand_select(void)
{
	NFCONT &= ~(0x1<<1);
}

void nand_deselect(void)
{
	NFCONT |= (0x1<<1);
}

void nand_cmd(unsigned char cmd)
{
	volatile int delay;
	NFCMMD = cmd;
	for (delay = 0; delay < 10; delay++);
}

void nand_addr(unsigned long addr)
{
	unsigned long col = addr % PAGE_SIZE;
	/* row address */
	unsigned long page = addr / PAGE_SIZE;
	volatile int delay;

	/* 根据DataSheet P20 - Read Operation */
	NFADDR = col & 0xff;
	for (delay = 0; delay < 10; delay++);
	NFADDR = (col>>8) & 0xff;
	for (delay = 0; delay < 10; delay++);

	NFADDR = page & 0xff;
	for (delay = 0; delay < 10; delay++);
	NFADDR = (page>>8) & 0xff;
	for (delay = 0; delay < 10; delay++);
	NFADDR = (page>>16) & 0xff;
	for (delay = 0; delay < 10; delay++);
}

void nand_wait_ready(void)
{
	while(!(NFSTAT & 0x1));	
}

unsigned char nand_read_byte(void)
{
	return NFDATA;
}

void nand_read(unsigned long *dest, unsigned long *src, 
		int len)
{
	int page = (int)src / PAGE_SIZE;
	int i = 0;

	/* 1. 片选 */
	nand_select();

	while(i < len) {
		/* 2. 发出开始命令00h */
		nand_cmd(0x00);
		/* 3. 发出地址 */
		nand_addr((unsigned long)src);
		/* 4. 发出读命令30h */
		nand_cmd(0x30);
		/* 5. 判断状态 */
		nand_wait_ready();
		/* 6. 读数据(整页) */
		for(; (page < PAGE_SIZE) && (i < len); page++) {
			dest[i++] = nand_read_byte();
		}
		src += PAGE_SIZE;
	}
	/* 7. 取消片选 */
	nand_deselect();
}

.text
.global _start

#define WTCON			0x15300000
#define MPLLCON			0x4c000004
#define CLKDIVN			0x4c000014
#define MEM_CTL_BASE	0x48000000

#define STACK_TOP		0x50000000

							/* m = 0x5c+8=100, p = 1+2=3, s = 1
							 * Mpll = (2*m*Fin)/(p*2^s) = (2*100*12M)/(3*2^1) = 400M
							 */
#define S3C2440_MPLL_400MHz		((0x5c<<12)|(0x01<<4)|(0x01))

_start:
	/* 1、关看门狗 */
	ldr r0,=WTCON
	mov r1, #0x0
	str r1, [r0]

	/* 2.1、设置分频比,400M 200M 50M
	 * FCLK:HCLK:PCLK = 8:4:1
	 */
	ldr r0, =CLKDIVN
	mov r1, #5
	str r1, [r0]

	/* 2.2、Set Async Bus Mode */
	mrc p15,0,r0,c1,c0,0
	orr r0,r0,#0xc0000000
	mcr p15,0,r0,c1,c0,0

	/* 2.3、Set MPLL 400M
	 * 参考 http://blog.csdn.net/mr_raptor/article/details/6555734
	 */
	ldr r0, =MPLLCON
	ldr r1, =S3C2440_MPLL_400MHz
	str r1, [r0]

	/* 3.1、初始化SDRAM */
	ldr r0, =MEM_CTL_BASE
	adr r1, sdram_config
	add r2, r0, #13*4

1:
	ldr r3, [r1], #4
	str r3, [r0], #4
	cmp r0, r2
	bne 1b

	/* 3.2、初始化ICACHE */

	/* 4、重定位,内存大小为1GB */
	ldr sp, =STACK_TOP

	bl nand_init

	/* 代码段长度由链接脚本计算出 */
	mov r0, #0
	ldr r1, =_start
	ldr r2, =__bss_start
	sub r2, r2, r1

	bl cp2Ram
	bl cleanBss

	/* 5、main */
	ldr lr, =halt
	ldr pc, =main

halt:
	b halt


sdram_config:
	/* 存储控制器13个寄存器的值 */
	.long 0x22011110;     //BWSCON
	.long 0x00000700;     //BANKCON0
	.long 0x00000700;     //BANKCON1
	.long 0x00000700;     //BANKCON2
	.long 0x00000700;     //BANKCON3  
	.long 0x00000700;     //BANKCON4
	.long 0x00000700;     //BANKCON5
	.long 0x00018005;     //BANKCON6
	.long 0x00018005;     //BANKCON7

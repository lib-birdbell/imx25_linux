# imx25_linux
toolchain is gcc-linaro-4.9.4-2017.01-x86_64_arm-linux-gnueabi

History<BR>
linux boot complete.<BR>
 device-tree modified.<BR>
<BR>
이건 옮기는거<BR>
$ tftp 0x80800000 uImage;bootm<BR>

MACHINE_START에 등록하면 맞는 장치에 따라 호출됨<BR>
clock 부분을 수정해야 하는데 작업양이 너무 많음<BR>
init_irq까지만 수행됨<BR>
arch/arm/kernel/irq.c 에서 init_IRQ()를 호출하는데, 여기서 init_irq()를 호출함<BR>
다 수행되지 못함<BR>
mxc_init_irq()가 호환이 안되기에 함수 추가함<BR>
레지스터 접근이 안됨<BR>
IRQ 이전에 뭔가를 빼먹은거 같음<BR>

:4000000h;4000000h<4000000h=4000<BR>

arch/arm/plat-mxc/include/mach/mx25.h:84:#define AIPS1_BASE_ADDR       0x43F00000<BR>
arch/arm/plat-mxc/include/mach/mx25.h:85:#define AIPS1_BASE_ADDR_VIRT  0xFC000000<BR>

arch/arm/include/asm/pgtable.h:46:#define VMALLOC_START         (((unsigned long)high_memory + VMALLOC_OFFSET) & ~(VMALLOC_OFFSET-1)<BR>

arch/arm/kernel/head.S<BR>
init/main.c<BR>
arch/arm/mm/mmu.c -> devicemaps_init<BR>
mm/memblock.c -> memblock_alloc<BR>
mm/bootmem.c -> __alloc_arch_preferred_bootmem -> alloc_bootmem_core<BR>
(void *)(VMALLOC_END - (240 << 20) - VMALLOC_OFFSET);<BR>
arch/arm/include/asm/memory.h:306:#define __va(x)                       ((void *)__phys_to_virt((phys_addr_t)(x)))<BR>
이거만 하면 멈춤<BR>
#if defined(CONFIG_ARM_PATCH_PHYS_VIRT)<BR>
왜 call은 되는데 return은 안되는건지<BR>
return 값에 따라서 되기도 하네, 와 이게 무슨일이야 -> 괄호때문임<BR>
calibrate_delay() 에서 멈춤<BR>
timer initialization이 안되어있음<BR>
2.6.31에서는<BR>
clk_register() -> list_add에 추가함 clocks <- clk->node<BR>
4.14에서는<BR>
clk_register_clkdev()<BR>
timer_init만 해주면 되지 않을까?<BR>
drivers/clocksource/timer-imx-gpt.c<BR>
mxc_init_irq(MX21_IO_ADDRESS(MX21_AVIC_BASE_ADDR));<BR>
#define ASIC_BASE_ADDR       0x68000000<BR>
#define ASIC_BASE_ADDR_VIRT  0xFC400000<BR>
#define ASIC_SIZE            SZ_1M<BR>
#define AVIC_BASE_ADDR       ASIC_BASE_ADDR<BR>
*map_io를 2.6과 똑같이 따라함 -> iotable은 잘 설정 된듯(map_io 에 설정하는거였음)<BR>
-> 이렇게 하니까 UART2가 막힘 -> 레지스터 정리 해서 잘 동작함<BR>
이번에는 timer 초기화를 해야함<BR>
irq=54로 등록이 되었고, timer-imx-gpt.c 에서 mxc_clocksource_init() 에서 c값을 강제로 13300000로 넣어주니 일단돌아감<BR>
그러나 kzalloc으로 할당한 메모리를 초기화도 안하고 값을 넘기지도 않음(수정이 필요)<BR>
rest_init에서 abt로 빠짐<BR>
printk확인해보고 interrupt확인해보면 알 수 있지 않을까?<BR>
26번째 멈춤<BR>
0xFFFF0000 초기화 부분이 주석처리 되어있어서 문제가 되었음(arch/arm/mm/mmu.c)<BR>
그러나 schedule_preempt_disabled() 부분에서 schedule()을 끝내지 못했음(원래 그런건가?) kernel/sched/core.c<BR>
timer에서 멈춰있음<BR>
Attempted to kill init! exitcode=0x0000000b<BR>
initrd가 없으면 그럴수가 있다고 함<BR>
https://www.linuxquestions.org/questions/linux-kernel-70/my-linux-kernel-boot-meet-the-error-not-syncing-attempted-to-kill-init-exit-code%3D0x0000000b-4175610787/<BR>
CONFIG_CMDLINE의 내용을 넣으니 VFS: Unable to mount root fs on unknown-block(0,0) 요기까지 나옴<BR>
-> 이거 꼭 bootloader에서 받을필요 없음<BR>
printk는 earlyprintk=ttymxc1 넣으니까 나옴 >.< 신기신기<BR>
CONFIG_EARLY_PRINTK 도 있어야함<BR>
이렇게 하면 early_printk.c의 early_param이 earlyprintk로 등록이 되고, 부팅 극초반에 호출됨<BR>
일단 .config에 CONFIG_DEBUG_IMX_UART_PORT=2 가 되어 있어야 하고 CONFIG_DEBUG_IMX25_UART=y 도 되어 있어야 함<BR>
또 CONFIG_DEBUG_LL_INCLUDE="debug/imx.S" 가 되어있는데, arch/arm/include 위치임<BR>
CLOCK은 공부해서 올려야 하고, 어느정도 맞추니까 더 진행 되었음<BR>
2.6에서 uart_clk=66500000, 4.14도 uart_clk는 66500000<BR>
이번엔 NAND인데, CLK을 못찾음<BR>
NAND의 CLK은 자동으로 of_로 찾도록 되어있음<BR>
그 말인 즉 원본 코드 수정이 불가피함(drivers/mtd/nand/mxc_nand.c)<BR>
nand 종류가 추가 되어야 함(drivers/mtd/nand/nand_ids.c)<BR>
timer가 제대로 안돌고 있음, calibration은 통과 했지만 delay가 안되고 있음<BR>
delay는 되는데, wait_for_completioin_timeout 가 안되고있음<BR>
mxc_irq()가 안떠서 안되는듯<BR>
interrup만 뜨면 No working init found. 까지 진행 됨(udelay 1000 4번 넣으면 됨)<BR>


arch/arm/include/generated/asm/mach-types.h<BR>
#define MACH_TYPE_MX25_3DS             1771(6EB)<BR>
MAX_DMA_ADDRESS<BR>
MXC_GPIO_IRQS<BR>
arch/arm/plat-mxc/Kconfig:29:config ARCH_MX25<BR>
arch/arm/plat-mxc/irq.c 2.6<BR>
arch/arm/mach-imx/mm-imx21.c 4.14<BR>
        avic_base = IO_ADDRESS(AVIC_BASE_ADDR);<BR>
# linux-4.15.7
linux for i.MX257<BR>
<BR>
Characteristic<BR>
 Use device tree<BR>
<BR>
How to make define config<BR>
 &#35; make imx25_default_defconfig<BR>
<BR>
Compiler<BR>
 arm-buildroot-linux-uclibcgnueabi-gcc.br_real (Buildroot 2021.05-git) 9.3.0<BR>
<BR>
use buildroot 2021.05<BR>
<BR>
<BR>
# linux-4.14.170
linux for i.MX257<BR>
<BR>
Characteristic<BR>
 Didn't use device-tree<BR>
<BR>
<BR>
# uboot-2018.01
uboot for i.MX257<BR>
<BR>
Specification<BR>
 CPU : i.MX25<BR>
 NAND : 128MB<BR>
 CONSOLE : serial uart2(uart2_RXD_MUX, uart2_TXD_MUX)<BR>
<BR>
How to make define config<BR>
 &#35; make mx25default_defconfig<BR>
<BR>
Compiler<BR>
 arm-buildroot-linux-uclibcgnueabi-gcc.br_real (Buildroot 2021.05-git) 9.3.0<BR>
<BR>
use buildroot 2021.05<BR>

/*
 * Perform a loopback test on the pzsdr breakout board using the test fixture.
 */

#include "sleep.h"
#include "xil_cache.h"
#include "xil_io.h"
#include "xparameters.h"
#include "xstatus.h"

#define GPIO_WAIT_TIME 5000

#include <stdio.h>

void delay_clk_count(u32 clk_count) {
	u32 data_1;
	u32 data_0;

	Xil_Out32(0xf8f00208, 0x0);
	Xil_Out32(0xf8f00200, 0x0);
	Xil_Out32(0xf8f00204, 0x0);
	Xil_Out32(0xf8f00208, 0x1);

	while (1) {
		data_1 = Xil_In32(0xf8f00204);
		data_0 = Xil_In32(0xf8f00200);
		if (data_1 == Xil_In32(0xf8f00204)) {
			if (data_0 >= (clk_count*4)) {
				break;
			}
		}
	}
}

void delay_ms(u32 ms_count) {
	delay_clk_count(ms_count*100000);
}

void gpio_write(u32 value) {
	Xil_Out32((XPAR_AXI_GPREG_BASEADDR + (0x101 * 0x4)), value);
	Xil_Out32((XPAR_AXI_GPREG_BASEADDR + (0x111 * 0x4)), value);
	Xil_Out32((XPAR_AXI_GPREG_BASEADDR + (0x121 * 0x4)), value);
}

int gpio_read(u32 pin, u32 expected) {
	int ret = XST_SUCCESS;
	u32 rdata;

	rdata = Xil_In32(XPAR_AXI_GPREG_BASEADDR + (0x102 * 0x4));
	if (rdata != expected) {
		xil_printf("Loopback error on pin %d: "
			"wrote 0x%08x, read 0x%08x\r\n", pin, expected, rdata);
		ret = XST_FAILURE;
	}

	rdata = Xil_In32(XPAR_AXI_GPREG_BASEADDR + (0x112 * 0x4));
	if (rdata != expected) {
		xil_printf("Loopback error on pin %d: "
			"wrote 0x%08x, read 0x%08x\r\n", (pin + 32), expected, rdata);
		ret = XST_FAILURE;
	}

	if (pin < 24) {
		rdata = Xil_In32(XPAR_AXI_GPREG_BASEADDR + (0x122 * 0x4));
		if ((rdata & 0xffffff) != (expected & 0xffffff)) {
			xil_printf("Loopback error on pin %d: "
				"wrote 0x%08x, read 0x%08x\r\n", (pin + 64),
				expected, rdata);
			ret = XST_FAILURE;
		}
	}

	return ret;
}

void gpio_wait()
{
	u32 i;

	for(i = 0; i < GPIO_WAIT_TIME; i++) {
		asm("nop");
	}
}

int lb_gt(u32 base_addr, u32 no_of_instances)
{
	int ret = XST_SUCCESS;
	u32 m;
	u32 rdata;

	m = (0x1 << no_of_instances) -1;

	Xil_Out32((base_addr + (0x4 * 0x4)), 0x0);
	rdata = Xil_In32(base_addr + (0x5 * 0x4));
	if (rdata != m) {
		xil_printf("GT loopback error: received 0x%08x, expected 0x%08x\r\n", rdata, m);
		ret = XST_FAILURE;
	}

	Xil_Out32((base_addr + (0x4 * 0x4)), 0x1);
	delay_ms(1);

	Xil_Out32((base_addr + (0x5 * 0x4)), m);
	delay_ms(10);

	rdata = Xil_In32(base_addr + (0x5 * 0x4));
	if (rdata != 0) {
		xil_printf("GT loopback error: received 0x%08x, expected 0x%08x\r\n", rdata, 0);
		ret = XST_FAILURE;
	}

	return ret;
}

int main()
{
	int ret = XST_SUCCESS;
	u32 n, wdata;

	Xil_DCacheDisable();
	Xil_ICacheDisable();

	/* walking 1 */
	for(n = 0; n < 32; n++) {
		wdata = 1 << n;
		gpio_write(wdata);
		gpio_wait();
		if (gpio_read(n, wdata) != XST_SUCCESS)
			ret = XST_FAILURE;
	}

	/* walking 0 */
	for(n = 0; n < 32; n++) {
		wdata = 1 << n;
		wdata = ~wdata;
		gpio_write(wdata);
		gpio_wait();
		if (gpio_read(n, wdata) != XST_SUCCESS)
			ret = XST_FAILURE;
	}

	ret = lb_gt(XPAR_AXI_PZ_XCVRLB_BASEADDR, 4);

	return ret;
}

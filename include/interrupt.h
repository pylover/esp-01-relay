#ifndef INTERRUPT_H
#define INTERRUPT_H

#define REBOOTBTN_MUX		PERIPHS_IO_MUX_GPIO0_U
#define REBOOTBTN_NUM		0
#define REBOOTBTN_FUNC		FUNC_GPIO0

void interrupt_init();

#endif


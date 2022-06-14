#ifndef RELAY_H
#define RELAY_H


enum relay_status {
    RELAY_ON = 0,
    RELAY_OFF = 1
};

#define RELAY_NUM		       0
#define RELAY_FUNC		       FUNC_GPIO0
#define RELAY_MUX              PERIPHS_IO_MUX_GPIO0_U


#endif

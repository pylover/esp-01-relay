#ifndef _PARTITION_CONFIG_H__
#define _PARTITION_CONFIG_H__

#include <user_interface.h>

/*
Flash Maps
0= 512KB( 256KB+ 256KB)
2=1024KB( 512KB+ 512KB)
3=2048KB( 512KB+ 512KB)
4=4096KB( 512KB+ 512KB)
5=2048KB(1024KB+1024KB)
6=4096KB(1024KB+1024KB)
7=4096KB(2048KB+2048KB) not support ,just for compatible with nodeMCU board
8=8192KB(1024KB+1024KB)
9=16384KB(1024KB+1024KB)
*/

// TODO: upgrade.a
// TODO: what is irom.text


#define SECTFMT                         "0x%X"
#define SECT_SIZE                       0x1000
#define SYSTEM_PARTITION_OTA1_ADDR      0x01000


#if ((SPI_FLASH_SIZE_MAP == 0) || (SPI_FLASH_SIZE_MAP == 1))
#error "The flash map is not supported"

#elif (SPI_FLASH_SIZE_MAP == 2)
#define SYSTEM_PARTITION_OTA_SIZE				0x5A000
#define USER_PARTITION_PARAMS_ADDR  			0xE0000

#define SYSTEM_PARTITION_SSL_CLIENT_CA_ADDR     0x79000
#define USER_INDEXHTML_ADDR                     0x7A000
#define SYSTEM_PARTITION_OTA_2_ADDR				0x81000
#define SYSTEM_PARTITION_RF_CAL_ADDR			0xFB000
#define SYSTEM_PARTITION_PHY_DATA_ADDR			0xFC000
#define SYSTEM_PARTITION_SYSTEM_PARAMETER_ADDR	0xFD000


#elif (SPI_FLASH_SIZE_MAP == 3)
#define SYSTEM_PARTITION_OTA_SIZE				0x6A000

#define SYSTEM_PARTITION_SSL_CLIENT_CA_ADDR     0x79000
#define USER_INDEXHTML_ADDR                     0x7A000
#define USER_PARTITION_PARAMS_ADDR	    		0x7D000
#define SYSTEM_PARTITION_OTA_2_ADDR				0x81000
#define SYSTEM_PARTITION_RF_CAL_ADDR			0x1FB000
#define SYSTEM_PARTITION_PHY_DATA_ADDR			0x1FC000
#define SYSTEM_PARTITION_SYSTEM_PARAMETER_ADDR	0x1FD000

#elif (SPI_FLASH_SIZE_MAP == 4)
#define SYSTEM_PARTITION_OTA_SIZE				0x6A000

#define SYSTEM_PARTITION_SSL_CLIENT_CA_ADDR     0x79000
#define USER_INDEXHTML_ADDR                     0x7A000
#define USER_PARTITION_PARAMS_ADDR  			0x7D000
#define SYSTEM_PARTITION_OTA_2_ADDR				0x81000
#define SYSTEM_PARTITION_RF_CAL_ADDR			0x3FB000
#define SYSTEM_PARTITION_PHY_DATA_ADDR			0x3FC000
#define SYSTEM_PARTITION_SYSTEM_PARAMETER_ADDR	0x3FD000

#elif (SPI_FLASH_SIZE_MAP == 5)
#define SYSTEM_PARTITION_OTA_SIZE				0x6A000

#define SYSTEM_PARTITION_SSL_CLIENT_CA_ADDR     0x0F9000
#define USER_INDEXHTML_ADDR                     0x0FA000
#define USER_PARTITION_PARAMS_ADDR  			0x0FD000
#define SYSTEM_PARTITION_OTA_2_ADDR				0x101000
#define SYSTEM_PARTITION_RF_CAL_ADDR			0x1FB000
#define SYSTEM_PARTITION_PHY_DATA_ADDR			0x1FC000
#define SYSTEM_PARTITION_SYSTEM_PARAMETER_ADDR	0x1FD000

#elif (SPI_FLASH_SIZE_MAP == 6)
#define SYSTEM_PARTITION_OTA_SIZE				0x6A000

#define SYSTEM_PARTITION_SSL_CLIENT_CA_ADDR     0x0F9000
#define USER_INDEXHTML_ADDR                     0x0FA000
#define USER_PARTITION_PARAMS_ADDR  			0x0FD000
#define SYSTEM_PARTITION_OTA_2_ADDR				0x101000
#define SYSTEM_PARTITION_RF_CAL_ADDR			0x3FB000
#define SYSTEM_PARTITION_PHY_DATA_ADDR			0x3FC000
#define SYSTEM_PARTITION_SYSTEM_PARAMETER_ADDR	0x3FD000

#elif (SPI_FLASH_SIZE_MAP == 8)
#define SYSTEM_PARTITION_OTA_SIZE				0x6A000

#define SYSTEM_PARTITION_SSL_CLIENT_CA_ADDR     0x0F9000
#define USER_INDEXHTML_ADDR                     0x0FA000
#define USER_PARTITION_PARAMS_ADDR		    	0x0FD000
#define SYSTEM_PARTITION_OTA_2_ADDR				0x101000
#define SYSTEM_PARTITION_RF_CAL_ADDR			0x7FB000
#define SYSTEM_PARTITION_PHY_DATA_ADDR			0x7FC000
#define SYSTEM_PARTITION_SYSTEM_PARAMETER_ADDR	0x7FD000

#else
#error "The flash map is not supported"
#endif


static const partition_item_t at_partition_table[6] = {
    { 
        SYSTEM_PARTITION_BOOTLOADER, 	
        0x0, 
        SECT_SIZE
    },
    { 
        SYSTEM_PARTITION_OTA_1, 
        0x1000, 
        SYSTEM_PARTITION_OTA_SIZE
    },
    { 
        SYSTEM_PARTITION_OTA_2, 
        SYSTEM_PARTITION_OTA_2_ADDR, 
        SYSTEM_PARTITION_OTA_SIZE
    },
    { 
        SYSTEM_PARTITION_RF_CAL, 
        SYSTEM_PARTITION_RF_CAL_ADDR, 
        SECT_SIZE
    },
    { 
        SYSTEM_PARTITION_PHY_DATA, 
        SYSTEM_PARTITION_PHY_DATA_ADDR, 
        SECT_SIZE
    },
    { 
        SYSTEM_PARTITION_SYSTEM_PARAMETER, 
        SYSTEM_PARTITION_SYSTEM_PARAMETER_ADDR, 
        SECT_SIZE * 3
    },
    // {
    //     SYSTEM_PARTITION_SSL_CLIENT_CA,
    //     SYSTEM_PARTITION_SSL_CLIENT_CA_ADDR,
    //     SECT_SIZE
    // },
};

#endif


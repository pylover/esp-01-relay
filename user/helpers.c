#include <osapi.h>


ICACHE_FLASH_ATTR
uint8_t getbase(char **s) {
	uint8_t base = 10;	
    char *str = *s;

	if (str[0] == '0') {
		// May be other bases
		switch (str[1]) {
			case 'b':
			case 'B':
				base = 2;
				break;
			case 'o':
			case 'O':
				base = 8;
				break;
			case 'x':
			case 'X':
				base = 16;
				break;
			default:
				base = 10;
		}
	}
	if (base != 10) {
		str += 2;
	}
    *s = str;
    return base;
}


ICACHE_FLASH_ATTR
uint64_t parse_uint(char *str) {
	uint8_t base = getbase(&str);;	
	return strtoul(str, NULL, base);	
}

ICACHE_FLASH_ATTR
long parse_int(char *str) {
	uint8_t base = getbase(&str);;	
	return strtol(str, NULL, base);	
}

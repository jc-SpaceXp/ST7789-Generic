#ifndef USER_DEFINED_CALLBACKS_H
#define USER_DEFINED_CALLBACKS_H

#include <stdint.h>

// User must provide these functions and assign the pointers to these struct members
struct UserCallbacksSt7789 {
	void (*delay_us)(unsigned int);
};

#endif /* USER_DEFINED_CALLBACKS_H */

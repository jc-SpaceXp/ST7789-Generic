#ifndef USER_DEFINED_CALLBACKS_H
#define USER_DEFINED_CALLBACKS_H

#include <stdbool.h>

// User must provide these functions and assign the pointers to these struct members
struct UserCallbacksSt7789 {
	void (*delay_us)(unsigned int);
	bool (*tx_ready_to_transmit)(void);
	bool (*tx_complete)(void);
};

#endif /* USER_DEFINED_CALLBACKS_H */

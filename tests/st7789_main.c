#include "greatest.h"
#include "st7789_suite.h"


GREATEST_MAIN_DEFS();

int main(int argc, char **argv)
{
	(void) argc;
	(void) argv;

	GREATEST_MAIN_BEGIN();

	RUN_SUITE(st7789_driver);
	RUN_SUITE(st7789_driver_modes_transitions);

	GREATEST_MAIN_END(); // exapnds to a return statement
}

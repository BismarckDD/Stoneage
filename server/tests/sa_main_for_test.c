/* Link the production runtime without starting its network event loop. */
#define main saac_server_main_for_test
#include "../saac/main.c"

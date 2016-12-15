/* Bandwidth query
 *
 * Query bandwidth of interfaces
 * header file
 * 
 * Supported interface(s):
 * ethX
 *
 * Written by Andras Majdan.
 * Email: majdan.andras@gmail.com
 */
 
#include <string>

const int REQ_SPEED_CONNECTED = 1;
const int REQ_SPEED_MAX = 2;

int get_interface_speed(
	unsigned long &res_speed, int req_speed, std::string dev_name);
int ethernet_interface(const char *const name, int *const speed);
int get_max_supported_speed(unsigned int smask);
int is_network_interface(std::string dev_name);

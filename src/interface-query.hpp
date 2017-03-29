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
#include <unordered_set>

const int REQ_SPEED_CONNECTED = 1;
const int REQ_SPEED_MAX = 2;

using namespace std;

int is_loopback(string dev_name);
unordered_set<string> get_list_of_interfaces();
int get_interface_speed(
	unsigned long &res_speed, int req_speed, string dev_name);
int ethernet_interface(const char *const name, int *const speed);
int get_max_supported_speed(unsigned int smask);
int is_network_interface(std::string dev_name);

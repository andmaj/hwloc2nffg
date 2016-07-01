/* dpdk-query
 *
 * Query DPDK interfaces header
 * 
 * Written by Andras Majdan.
 * Email: majdan.andras@gmail.com
 */

#include <string>

void dpdk_init();
void dpdk_free();
bool is_dpdk_interface(std::string dev_id);
std::string get_dpdk_interface_name(std::string dev_id);

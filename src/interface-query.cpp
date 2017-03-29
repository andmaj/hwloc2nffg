/* Bandwidth query
 *
 * Query bandwidth of interfaces
 *
 * Supported interface(s):
 * all under /sys/class/net
 *
 * Written by Andras Majdan.
 * Email: majdan.andras@gmail.com
 */

#include <string.h>
#include <unordered_set>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/regex.hpp>
#include <boost/lexical_cast.hpp>

#include <unistd.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/ip.h>
#include <net/if.h>
#include <linux/ethtool.h>
#include <linux/sockios.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <limits.h>

#include "interface-query.hpp"

namespace fs = boost::filesystem;

using namespace std;

int is_loopback(string dev_name)
{
	// A network interface is a loopback if in /sys/class/net/<iface>/flags
	// IFF_LOOPBACK (1<<3) bit is set.
	fs::path p("/sys/class/net/" + dev_name + "/flags");
	if (fs::exists(p) && fs::is_regular_file(p))
	{
		int flags = 0;

		fs::ifstream fin(p);
		fin >> hex >> flags;
		fin.close();

		return flags & IFF_LOOPBACK;
	}
	return 0;
}

// Return list of interfaces except loopback
unordered_set<string> get_list_of_interfaces()
{
	unordered_set<string> ifaces;

	fs::path p("/sys/class/net/");
	if (fs::exists(p) && fs::is_directory(p))
	{
		boost::regex pattern("^[a-zA-Z]+[0-9a-zA-Z]*$");
		for(auto& entry : boost::make_iterator_range(fs::directory_iterator(p), {}))
		{
			boost::smatch match;
			std::string fn = entry.path().filename().string();
			if (boost::regex_match( fn, match, pattern))
			{
				if(!is_loopback(fn))
				{
					ifaces.insert(fn);
				}
			}
		}
	}

	return ifaces;
}

int is_network_interface(string dev_name)
{
	static const boost::regex dev_regex("^[a-zA-Z]+[0-9a-zA-Z]*$");
	if(boost::regex_match(dev_name, dev_regex))
	{
		fs::path p("/sys/class/net/" + dev_name);
		if(fs::exists(p))
		{
			return 1;
		}
	}

	return 0;
}

// Get interface speed
//
// Returns
// 0    success
// else failed
int get_interface_speed(
	unsigned long &res_speed, int req_speed, string dev_name)
{
	long long int curr_speed = -1;
	fs::path p("/sys/class/net/" + dev_name + "/speed");

	switch(req_speed)
	{
		case REQ_SPEED_CONNECTED:
			if (fs::exists(p) && fs::is_regular_file(p))
			{
				try {
					fs::ifstream fin(p);
					fin >> curr_speed;
					fin.close();
				} catch (...) {
					curr_speed = -1;
				}
			}
			break;

		case REQ_SPEED_MAX:
			int speed;

			if (ethernet_interface(dev_name.c_str(), &speed))
			{
				return 1;
			}
			curr_speed = speed;

			break;
	}

	if (curr_speed<0 || curr_speed>INT_MAX)
	{
		return 1;
	}

	res_speed = curr_speed;
	return 0;
}

// Based on:
// http://stackoverflow.com/questions/14264371/how-to-get-nic-details-from-a-c-program
//
// TODO: provide alternative in future (ethtool_cmd is DEPRECATED in new kernels)
int ethernet_interface(const char *const name, int *const speed)
{
    struct ifreq        ifr;
    struct ethtool_cmd  cmd;
    int                 fd, result;

    if (!name || !*name) {
        fprintf(stderr, "Error: NULL interface name.\n");
        fflush(stderr);
        return errno = EINVAL;
    }

    if (speed)  *speed = -1;

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) {
        const int err = errno;
        fprintf(stderr, "%s: Cannot create AF_INET socket: %s.\n", name, strerror(err));
        fflush(stderr);
        return errno = err;
    }

    strncpy(ifr.ifr_name, name, sizeof ifr.ifr_name);
    ifr.ifr_data = (__caddr_t)(void *)&cmd;
    cmd.cmd = ETHTOOL_GSET;
    if (ioctl(fd, SIOCETHTOOL, &ifr) < 0) {
        const int err = errno;
        do {
            result = close(fd);
        } while (result == -1 && errno == EINTR);
        fprintf(stderr, "%s: SIOCETHTOOL ioctl: %s.\n", name, strerror(err));
        return errno = err;
    }


   /* Connected link speed alternative query

      if (speed)
         *speed = ethtool_cmd_speed(&cmd);
    */

   unsigned int smask;

   smask = cmd.supported;
   *speed = get_max_supported_speed(smask);

   close(fd);
   return 0;
}

int get_max_supported_speed(unsigned int smask)
{
	int speed = -1;

	if(smask & SUPPORTED_10baseT_Half) speed = 10;
	if(smask & SUPPORTED_10baseT_Full) speed = 10;
	if(smask & SUPPORTED_100baseT_Half) speed = 100;
	if(smask & SUPPORTED_100baseT_Full) speed = 100;
	if(smask & SUPPORTED_1000baseT_Half) speed = 1000;
	if(smask & SUPPORTED_1000baseT_Full) speed = 1000;
	if(smask & SUPPORTED_1000baseKX_Full) speed = 1000;
	if(smask & SUPPORTED_2500baseX_Full) speed = 2500;
	if(smask & SUPPORTED_10000baseT_Full) speed = 10000;
	if(smask & SUPPORTED_10000baseKX4_Full) speed = 10000;
	if(smask & SUPPORTED_10000baseKR_Full) speed = 10000;
	if(smask & SUPPORTED_10000baseR_FEC) speed = 10000;
	if(smask & SUPPORTED_20000baseMLD2_Full) speed = 20000;
	if(smask & SUPPORTED_20000baseKR2_Full) speed = 20000;
	if(smask & SUPPORTED_40000baseKR4_Full) speed = 40000;
	if(smask & SUPPORTED_40000baseCR4_Full) speed = 40000;
	if(smask & SUPPORTED_40000baseSR4_Full) speed = 40000;
	if(smask & SUPPORTED_40000baseLR4_Full) speed = 40000;

	return speed;
}

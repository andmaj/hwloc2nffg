/* dpdk-query
 *
 * Query DPDK interfaces 
 * 
 * Supported driver(s):
 * igb-uio
 *
 * Written by Andras Majdan.
 * Email: majdan.andras@gmail.com
 */
 
 /* /---->[ Uninitialised state ]
  * |              |
  * |              |
  * |          dpdk_init()
  * |              |
  * |              |
  * |              v  
  * |      [ Initialised state ]<-------------\
  * |         |      |      |                 |
  * |         |      |      |                 |
  * |         |      |  is_dpdk_interface()   |
  * |         |      |      |                 |
  * |         |      |      \-----------------|
  * |         |      |                        |
  * |         |  get_dpdk_interface_name()    |
  * |         |      |                        |
  * |         |      \------------------------/
  * |         |
  * |     dpdk_free()
  * |         |
  * \---------/
  */
  
#include <string>
#include <vector>
#include <map>
#include <boost/filesystem.hpp>
#include <boost/regex.hpp>
#include <boost/range/iterator_range.hpp>

namespace fs = boost::filesystem;

using namespace std;

map<string,string> dpdk_interfaces;

// Fills DPDK interface map
void dpdk_init()
{
	//fs::path p("/sys/bus/pci/drivers/igb_uio/");
	fs::path p("/tmp/net/");
	if (fs::exists(p) && fs::is_directory(p))
	{
		vector<string> ifaces;
		
		boost::regex pattern("^[a-fA-F0-9]{4}:[a-fA-F0-9]{2}:[a-fA-F0-9]{2}[.][a-fA-F0-9]$");
		for(auto& entry : boost::make_iterator_range(fs::directory_iterator(p), {}))
		{
			boost::smatch match;
			std::string fn = entry.path().filename().string();
			if (boost::regex_match( fn, match, pattern))
			{
				ifaces.push_back(fn);
			}
		}
		
		sort(ifaces.begin(), ifaces.end());

		unsigned int n = 0;
		for (auto p: ifaces)
		{
			char dpdk_id[8];
			snprintf(dpdk_id, sizeof(dpdk_id), "dpdk%u", n++);
			dpdk_interfaces.insert( pair<string,string>(p, dpdk_id) );
			// cout << "Added " << dpdk_id << " with PCI busid " << p << endl;
		}
		
		ifaces.clear();
	}
				
}

// Free DPDK interface map
void dpdk_free()
{
	dpdk_interfaces.clear();
}

bool is_dpdk_interface(string dev_id)
{
	if(dpdk_interfaces.count(dev_id) > 0)
		return true;
	else
		return false;
}

string get_dpdk_interface_name(string dev_id)
{	
	return dpdk_interfaces.at(dev_id);
}

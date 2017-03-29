/* hwloc2nffg
 *
 * Converts hwloc data (hierarchical map of computing and network elements)
 * to an NFFG representation in JSON format. This data can be processed by
 * an orchestrator.
 *
 * Written by Andras Majdan.
 * Email: majdan.andras@gmail.com
 */

#include <iostream>
#include <string>
#include <algorithm>
#include <unordered_set>
#include <boost/program_options.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <jsoncpp/json/json.h>
#include <hwloc.h>
#include <sys/utsname.h>

#include "dpdk-query.hpp"
#include "interface-query.hpp"

using namespace std;

namespace po = boost::program_options;

// TODO: git versioning
const string version = "unknown";

const unsigned long INTERFACE_SPEED_DEFAULT = 1001;
//int cpusocket = 0;

class ID
{
	private:
	unsigned int lastfreeid=0;

	public:
	map<string, unsigned int> lastfreeidfortype;

	unsigned int get_next_id_for_type(string nodetype)
	{
		unsigned int n;

		if(lastfreeidfortype.find(nodetype) == lastfreeidfortype.end())
			lastfreeidfortype.insert(make_pair(nodetype, 0));

		n = lastfreeidfortype[nodetype];
		lastfreeidfortype[nodetype] += 1;
		return n;
	}

	unsigned int get_next_global_id()
	{
		return lastfreeid++;
	}
};

struct OPTIONS
{
	bool merge = false;
	bool dpdk = false;
	bool notreported = false;
};

string get_link_speed(string dev_name)
{
	if (is_network_interface(dev_name))
	{
	
		int res;
		unsigned long speed;
	
		res = get_interface_speed(speed, REQ_SPEED_CONNECTED, dev_name);
		if(!res)
		{
			return to_string(speed);
		}
	
		res = get_interface_speed(speed, REQ_SPEED_MAX, dev_name);
	
		if(!res)
		{
			return to_string(speed);
		}
	}
	return to_string(INTERFACE_SPEED_DEFAULT);
}

void add_parameters(Json::Value &root)
{
	struct utsname unamedata;
	uname(&unamedata);

	root["id"] = unamedata.nodename;
	root["name"] = string("NFFG-") + string(unamedata.nodename);

	// TODO: real versioning
	root["version"] = "1.0";
}

// Check if node is a network sap
bool network_sap(hwloc_obj_t node)
{
	if(node->type==HWLOC_OBJ_OS_DEVICE)
	{
		int num_of_infos = node->infos_count;

		for(int info_i=0; info_i<num_of_infos; info_i++)
			if(!strcasecmp(node->infos[info_i].name, "address"))
				return true;
	}
	return false;
}

string busid_from_pcidev(hwloc_obj_t node)
{
	char busid[14];
	snprintf(busid, sizeof(busid), "%04x:%02x:%02x.%01x",
      node->attr->pcidev.domain, node->attr->pcidev.bus,
      node->attr->pcidev.dev, node->attr->pcidev.func);
    return busid;
}


// Check if node is a DPDK sap
bool dpdk_sap(hwloc_obj_t node, OPTIONS &options)
{
	// Check for DPDK include option and also for proper device
	if (options.dpdk && node->type==HWLOC_OBJ_PCI_DEVICE)
	{
		string busid = busid_from_pcidev(node);
		if (is_dpdk_interface(busid))
		{
			return true;
		}
	}
	return false;
}

// Check if node is required (based on node's type)
bool required_by_type(hwloc_obj_t node, OPTIONS &options)
{
	hwloc_obj_type_t type = node->type;

	if (type == HWLOC_OBJ_PU)
		return true;
	else if (type == HWLOC_OBJ_OS_DEVICE)
		return network_sap(node);
	else if (type == HWLOC_OBJ_PCI_DEVICE)
		return dpdk_sap(node, options);
	else
		return false;
}

string get_node_type(hwloc_obj_t obj)
{
	char ctype[32];
	string type;

	hwloc_obj_type_snprintf(ctype, sizeof(ctype), obj, 0);
	type = string(ctype);
	
	return type;
}

string sanitize(string s)
{
	// in-place replace
	boost::replace_all(s, " ", "_");
	boost::replace_all(s, "\t", "_");
	boost::replace_all(s, "\n", "_");
	return s;
}

string get_node_name(hwloc_obj_t obj, ID &id, OPTIONS &options)
{
	if ( network_sap(obj) && obj->name != NULL)
		return sanitize(string(obj->name));

	string type = get_node_type(obj);
	//if(!type.compare("Socket")) cpusocket++;

	if ( (obj->type == HWLOC_OBJ_PU || obj->type == HWLOC_OBJ_CORE ||
		  obj->type == HWLOC_OBJ_MACHINE) &&
		  (obj->os_index != (unsigned) -1) )
		if(!type.compare("Core"))
			//return sanitize(type + "#" + to_string(obj->os_index) + "#" + 
			//	to_string(cpusocket));
			return sanitize(type + "#" + to_string(obj->os_index) + "!" +
				to_string(id.get_next_id_for_type(type)));
		else
			return sanitize(type + "#" + to_string(obj->os_index));
	else if ( dpdk_sap(obj, options) )
		return sanitize(busid_from_pcidev(obj));
	else
		return sanitize(type + "!" + to_string(id.get_next_id_for_type(type)));
}

typedef deque<pair<unsigned int, string>*> NodePorts;

void merge_with_child(NodePorts *ports, hwloc_obj_t obj)
{
	// TODO: merging logic

	// Parent: obj
	// Child: obj->children[0]
	// Child's port_gid and node_name in ports

	return;
}

// Process nodes
NodePorts *add_nodes(
	Json::Value &node_infras,
	Json::Value &node_saps,
	Json::Value &node_edges,
	ID &id,
	hwloc_topology_t &topology,
	hwloc_obj_t obj,
	int depth,
	OPTIONS &options)
{
	auto *allports = new deque<NodePorts*>;

	// Merge this node in case of one child
	if (options.merge && obj->arity == 1)
	{
		auto *ports = add_nodes(node_infras, node_saps,
			node_edges, id, topology, obj->children[0], depth + 1, options);
		merge_with_child(ports, obj);
		return ports;
	}

	for (unsigned int i = 0; i < obj->arity; i++) {
		auto *ports = add_nodes(node_infras, node_saps,
			node_edges, id, topology, obj->children[i], depth + 1, options);
		if (ports != NULL)
			allports->push_back(ports);
    }

    // Add phantom port in case of DPDK
    if (dpdk_sap(obj, options))
    {
		unsigned int pgid = id.get_next_global_id();
		string nname = get_dpdk_interface_name(busid_from_pcidev(obj));
		auto *nports = new NodePorts;
		nports->push_back(new pair<unsigned int, string>(pgid, nname));
		allports->push_back(nports);

		Json::Value sap;
		Json::Value ports;
		Json::Value portsid;
		portsid["id"] = pgid;
		ports.append(portsid);

		sap["id"] = sap["name"] = nname;
		sap["ports"] = ports;
		node_saps.append(sap);
	}

    if (!allports->empty() || required_by_type(obj, options))
    {
		Json::Value node;
		Json::Value ports;

		string node_name = get_node_name(obj, id, options);
		node["id"] = node["name"] = node_name;

		if (!allports->empty())
		{
			for (auto ait = allports->begin(); ait != allports->end(); ait++)
			{
				for (auto pit = (*ait)->begin(); pit != (*ait)->end(); pit++)
				{
					Json::Value edge;
					unsigned int port_gid;

					edge["id"] = id.get_next_global_id();
					edge["src_node"] = node_name;
					port_gid = id.get_next_global_id();
					edge["src_port"] = port_gid;
					edge["dst_node"] = (*pit)->second;
					edge["dst_port"] = (*pit)->first;
					edge["delay"] = 0.1;
					edge["bandwidth"] = get_link_speed((*pit)->second);
					node_edges.append(edge);

					Json::Value portid;
					portid["id"] = port_gid;
					ports.append(portid);
				}
			}
		}

		Json::Value portid;
		unsigned int port_gid = id.get_next_global_id();
		portid["id"] = port_gid;
		ports.append(portid);

		if (network_sap(obj))
		{
			Json::Value sap;
			sap["id"] = sap["name"] = node_name;
			sap["ports"] = ports;
			node_saps.append(sap);
		}
		else
		{
			Json::Value node;
			node["id"] = node["name"] = node_name;
			node["ports"] = ports;
			node["domain"] = "INTERNAL";

			if (obj->type == HWLOC_OBJ_PU)
			{
				Json::Value supported;
				supported.append("headerDecompressor");
				node["type"] = "EE";
				node["supported"] = supported;
				Json::Value res;
				res["cpu"] = 1;
				res["mem"] = 32000;
				res["storage"] = 150;
				res["delay"] = 0.5;
				res["bandwidth"]= 1000;
				node["resources"] = res;
			}
			else
			{
				node["type"] = "SDN-SWITCH";
				Json::Value res;
				res["cpu"] = 0;
				res["mem"] = 0;
				res["storage"] = 0;
				res["delay"] = 0.5;
				res["bandwidth"]= 1000;
				node["resources"] = res;
			}
			node_infras.append(node);
		}

		auto *node_ports = new NodePorts;
		auto *pair_to_push = new pair<unsigned int, string>(port_gid, node_name);
		node_ports->push_back(pair_to_push);
		return node_ports;
	}

	return NULL;
}

void add_not_reported_network_interfaces(
	Json::Value &node_infras,
	Json::Value &node_saps,
	Json::Value &node_edges,
	ID &id)
{
	unordered_set<string> ifaces = get_list_of_interfaces();
	
	for (auto i = node_saps.begin(); i != node_saps.end(); ++i)
	{
		Json::FastWriter fastWriter;
		string lookfor = fastWriter.write((*i)["id"]);
		lookfor.erase(remove(lookfor.begin(), lookfor.end(), '\n'), lookfor.end());
		lookfor.erase(remove(lookfor.begin(), lookfor.end(), '\"'), lookfor.end());
		unordered_set<string>::const_iterator got = ifaces.find (lookfor);
		if (got != ifaces.end())
		{
			// It is in the set, have to remove
			ifaces.erase(lookfor);
		}
	}
	
	// Now ifaces only contains not reported network interfaces
	
	if(ifaces.size()<1)
		return; 
	
	Json::Value nrbus;
	nrbus["id"] = nrbus["name"] = "NRBUS";
	nrbus["domain"] = "INTERNAL";
	nrbus["type"] = "SDN-SWITCH";
	Json::Value res;
	res["cpu"] = 0;
	res["mem"] = 0;
	res["storage"] = 0;
	res["delay"] = 0.5;
	res["bandwidth"]= 1000;
	nrbus["resources"] = res;
	
	Json::Value nrbus_ports;
		
	for (auto i = ifaces.begin(); i != ifaces.end(); ++i)
	{
		string iface = *i;
		
		// Add a SAP
		unsigned int sap_port_id = id.get_next_global_id();
		Json::Value sap;
		Json::Value sap_ports;
		Json::Value sap_portsid;
		sap_portsid["id"] = sap_port_id;
		sap_ports.append(sap_portsid);
		sap["id"] = sap["name"] = iface;
		sap["ports"] = sap_ports;
		node_saps.append(sap);
		
		// Add a port to nrbus ports
		unsigned int nrbus_port_id = id.get_next_global_id();
		Json::Value nrbus_portsid;
		nrbus_portsid["id"] = nrbus_port_id;
		nrbus_ports.append(nrbus_portsid);
		
		// Add an edge link
		Json::Value edge;
		unsigned int port_gid;

		edge["id"] = id.get_next_global_id();
		edge["src_node"] = nrbus["id"];
		edge["src_port"] = nrbus_port_id;
		edge["dst_node"] = sap["id"];
		edge["dst_port"] = sap_port_id;
		edge["delay"] = 0.1;
		edge["bandwidth"] = get_link_speed(iface);
		node_edges.append(edge);
	}
	
	nrbus["ports"] = nrbus_ports;
	node_infras.append(nrbus);
}

void add_topology_tree(Json::Value &root, OPTIONS &options)
{
	if(options.dpdk)
	{
		dpdk_init();
	}

	hwloc_topology_t topology;

	// Allocate and initialize topology object.
	hwloc_topology_init(&topology);

	// Add PCI devices for detection
	hwloc_topology_set_flags(topology, HWLOC_TOPOLOGY_FLAG_WHOLE_IO);

	// Perform the topology detection.
	hwloc_topology_load(topology);

	// Add NFFG parameters
	Json::Value parameters;
	add_parameters(parameters);
	root["parameters"] = parameters;

	ID id;
	Json::Value node_infras, node_saps, node_edges;

	add_nodes(node_infras, node_saps, node_edges,
		id, topology, hwloc_get_root_obj(topology), 0, options);

	
	if (options.notreported)
	{
		// Include not reported network interfaces
		add_not_reported_network_interfaces(node_infras, node_saps, node_edges, id);
	}

	root["node_saps"] = node_saps;
	root["node_infras"] = node_infras;
	root["edge_links"] = node_edges;
}

int main(int argc, char* argv[])
{
	OPTIONS options;

	po::options_description desc("Allowed options");
	desc.add_options()
		("help", "Prints help message")
		("version", "Prints version number")
		("merge", "Merge nodes which have only one child")
		("dpdk", "Include DPDK interfaces")
		("notreported", "Include not reported network interfaces")
	;

	po::variables_map vm;
	po::store(po::parse_command_line(argc, argv, desc), vm);
	po::notify(vm);

	if (vm.count("help")) {
		cout << desc << endl;
		return 0;
	}

	if (vm.count("version")) {
		cout << "Version " << version << endl;
		return 0;
	}

	if (vm.count("merge")) {
		options.merge = true;
	}

	if (vm.count("dpdk")) {
		options.dpdk = true;
	}
	
	if (vm.count("notreported")) {
		options.notreported = true;
	}

	Json::Value root;
	add_topology_tree(root, options);

	Json::StyledWriter writer;
	string json_string = writer.write(root);
	cout << json_string;
	return 0;
}



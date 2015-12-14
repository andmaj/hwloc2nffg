/* hwloc2nffg
 *
 * Converts hwloc data (hierarchical map of computing and network elements)
 * to an NFFG representation in JSON format. This data can be processed by
 * an orchestrator.
 *
 * Written by Andras Majdan.
 * Email: majdan.andras@gmail.com
 */

#ifndef VERSION
#define VERSION "unknown"
#endif

#include <hwloc.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <sys/utsname.h>
#include <json.h>

int id = 0;

json_object *create_topology_tree();

void print_help()
{
	printf("Usage: hwloc2nffg <option>\n\n"
		"Options\n"
		"--help\n"
		"--version\n\n"
		"Written by Andras Majdan\n"
		"Email: majdan.andras@gmail.com\n");
}

void print_version()
{
	printf("Version %s\n", VERSION);
}

int main(int argc, char *argv[])
{
	// Too many parameters
	if(argc > 2)
	{
		fprintf(stderr, "Unexpected parameters:");
		for(int n=1; n<argc; n++)
			fprintf(stderr, " %s", argv[n]);
		fprintf(stderr, "\n");
		exit(1);
	}
	
	// Exactly one parameter
	if(argc==2)
	{
		if(!strcmp(argv[1], "--help"))
			print_help();
		else if(!strcmp(argv[1], "--version"))
			print_version();
		else
		{
			fprintf(stderr, "Unexpected parameter: %s\n", argv[1]);
			exit(1);
		}
			
		return 0;
	}
	
	
	// No parameters
	create_topology_tree();
	
	return 0;
}

json_object *create_parameters()
{
	json_object *params = json_object_new_object();
	
	struct utsname unamedata;
	uname(&unamedata);
	
	json_object_object_add(params, "id", 
		json_object_new_string(unamedata.nodename)); 
		
	char name[256];
	snprintf(name, sizeof name, "%s%s", "NFFG-", unamedata.nodename);
		
	json_object_object_add(params, "name", 
		json_object_new_string(unamedata.nodename));
		
  // TODO: real versioning
	json_object_object_add(params, "version", 
		json_object_new_string("1.0"));
	
	return params;
}

json_object *create_saps(hwloc_topology_t topology)
{
	json_object *saps = json_object_new_array();
	json_object *eth, *ports, *portid;
	
	hwloc_obj_t obj;
	
	int num_of_objs = hwloc_get_nbobjs_by_type(topology, HWLOC_OBJ_OS_DEVICE);
	for(int obj_i=0; obj_i<num_of_objs; obj_i++)
	{
		obj = hwloc_get_obj_by_type(topology, HWLOC_OBJ_OS_DEVICE, obj_i);
		
		// TODO: use HWLOC_OBJ_OSDEV_NETWORK somehow instead of address 
		int num_of_infos = obj->infos_count;
		
		for(int info_i=0; info_i<num_of_infos; info_i++)
		{
			if(!strcasecmp(obj->infos[info_i].name, "address"))
			{
				eth = json_object_new_object();
				json_object_object_add(eth, "id", json_object_new_string(obj->name));
				json_object_object_add(eth, "name", json_object_new_string(obj->name));
				
				ports = json_object_new_array();
				portid = json_object_new_object();
				
				json_object_object_add(portid, "id", json_object_new_int(id++));
				json_object_array_add(ports, portid);
				
				
				json_object_object_add(eth, "ports", ports);
				json_object_array_add(saps, eth);
			}
		}
	
  }
  return saps;
}

json_object *create_pci_switch()
{
	json_object *pcisw = json_object_new_object();
	
	json_object_object_add(pcisw, "id", json_object_new_string("pcisw"));
	json_object_object_add(pcisw, "name", json_object_new_string("pcisw"));
	
	json_object *ports = json_object_new_array();
	
	for(int n=0; n<id; n++)
	{
			json_object *portid = json_object_new_object();
			json_object_object_add(portid, "id", json_object_new_int(n));
			json_object_array_add(ports, portid);
	}
	
  json_object_object_add(pcisw, "ports", ports);
	
	return pcisw;
}

json_object *create_topology_tree()
{
	hwloc_topology_t topology;
	
	// Create root of JSON tree
	json_object *root = json_object_new_object();
	
  // Allocate and initialize topology object.
  hwloc_topology_init(&topology);
  
  // Detect PCI devices
  hwloc_topology_set_flags(topology, HWLOC_TOPOLOGY_FLAG_IO_DEVICES);
  
	// Perform the topology detection.
	hwloc_topology_load(topology);
	
	json_object *params = create_parameters();
	json_object_object_add(root, "parameters", params);
	
	json_object *saps = create_saps(topology);
	json_object_object_add(root, "node_saps", saps);
	
	json_object *infras = json_object_new_array();
	
	json_object *pcisw = create_pci_switch();
	json_object_array_add(infras, pcisw);
	
	json_object_object_add(root, "node_infras", infras);
	
	printf("%s\n", 
		json_object_to_json_string_ext(root, JSON_C_TO_STRING_PRETTY));
	
	// TODO
	
	// Destroy topology object.
	hwloc_topology_destroy(topology);
	
}

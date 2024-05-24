#include "kernel/yosys.h"
#include "kernel/drivertools.h"
#include "kernel/topo_scc.h"
#include "kernel/functional.h"
#include "libs/json11/json11.hpp"

USING_YOSYS_NAMESPACE
PRIVATE_NAMESPACE_BEGIN

using namespace json11;

struct ExampleFn {
  IdString name;
  dict<IdString, Const> parameters;

  ExampleFn(IdString name) : name(name) {}
  ExampleFn(IdString name, dict<IdString, Const> parameters) : name(name), parameters(parameters) {}

  bool operator==(ExampleFn const &other) const {
    return name == other.name && parameters == other.parameters;
  }

  unsigned int hash() const {
    return mkhash(name.hash(), parameters.hash());
  }
};

typedef ComputeGraph<ExampleFn, int, IdString, IdString> ExampleGraph;
struct ExampleWorker
{
  DriverMap dm;
  Module *module;

  ExampleWorker(Module *module) : module(module) {
    dm.celltypes.setup();
  }
};

struct ComputeGraphFromNetlistPass : public Pass
{
  NetlistFromComputeGraphPass() : Pass("compute_graph_netlist_from", "drivertools example") {}

  void help() override
  {
    //   |---v---|---v---|---v---|---v---|---v---|---v---|---v---|---v---|---v---|---v---|
    log("\n");
  }
   
  void execute(std::vector<std::string> args, RTLIL::Design *design) override
  {
    // std::ifstream file(filename);
    // std::string file_contents((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    // std::string err;
    // json11::Json json = json11::Json::parse(file_contents, err);
    // if (!err.empty()) {
    //     log_error("JSON parsing error: %s\n", err.c_str());
    //     return;
    // }

    // // Example assumes JSON has a nodes array and outputs array
    // json11::Json::array nodes = json["nodes"].array_items();
    // json11::Json::array outputs = json["outputs"].array_items();

    // // Process nodes
    // for (auto &node : nodes) {
    //     std::string node_type = node["type"].string_value();
    //     std::string node_id = node["id"].string_value();
    //     json11::Json::object parameters = node["parameters"].object_items();

    //     if (node_type == "$$input") {
    //         // Create an input port in RTLIL
    //         RTLIL::Wire *wire = new RTLIL::Wire();
    //         wire->name = RTLIL::IdString(node_id);
    //         wire->port_input = true;
    //         design->add(wire);
    //     }
    //     // Handle other node types similarly
    // }

    // // Outputs handling could involve setting certain wires as outputs based on the JSON

    // log("Imported design from %s\n", filename.c_str());
  }
} ExampleDtPass;

PRIVATE_NAMESPACE_END

#include "partitioner.hpp"

void partition(const std::string &fpga_graph_file, char* netlist_graph_file) {
  mt_kahypar_error_t error{};

  // Create FPGA Graph
  std::vector<std::vector<int32_t>> fpga_graph = fpga_graph_from_file(fpga_graph_file);
  int32_t num_fpgas = fpga_graph.size();

  std::cout << num_fpgas << std::endl;
  std::cout << netlist_graph_file << std::endl;

  // Initialize
  mt_kahypar_initialize(
    std::thread::hardware_concurrency() /* use all available cores */,
    true /* activate interleaved NUMA allocation policy */ );


  // Setup partitioning context
  mt_kahypar_context_t* context = mt_kahypar_context_from_preset(DEFAULT);
  // In the following, we partition a hypergraph into two blocks
  // with an allowed imbalance of 3% and optimize the connective metric (KM1)
  mt_kahypar_set_partitioning_parameters(context,
    num_fpgas /* number of blocks */, 0.10 /* imbalance parameter */,
    KM1 /* objective function */);
  mt_kahypar_set_seed(42 /* seed */);
  // Enable logging
  mt_kahypar_status_t status =
    mt_kahypar_set_context_parameter(context, VERBOSE, "1", &error);
  assert(status == SUCCESS);

  // Load Hypergraph for DEFAULT preset
  mt_kahypar_hypergraph_t hypergraph =
    mt_kahypar_read_hypergraph_from_file(netlist_graph_file,
      context, HMETIS /* file format */, &error);
  if (hypergraph.hypergraph == nullptr) {
    std::cout << error.msg << std::endl; std::exit(1);
  }

  // Partition Hypergraph
  mt_kahypar_partitioned_hypergraph_t partitioned_hg =
    mt_kahypar_partition(hypergraph, context, &error);
  if (partitioned_hg.partitioned_hg == nullptr) {
    std::cout << error.msg << std::endl; std::exit(1);
  }

  // Route Hypergraph
  router router(fpga_graph, hypergraph, partitioned_hg);
  router.route();

  mt_kahypar_free_context(context);
  mt_kahypar_free_hypergraph(hypergraph);
  mt_kahypar_free_partitioned_hypergraph(partitioned_hg);
}

std::vector<std::vector<int32_t>> fpga_graph_from_file(const std::string &filename) {
    std::ifstream infile(filename);
    if (!infile.is_open()) {
        throw std::runtime_error("Error opening file: " + filename);
    }

    int32_t numNodes, numEdges;
    infile >> numNodes >> numEdges;
    infile.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // move to next line

    // Initialize adjacency matrix with -1 (no edge)
    std::vector<std::vector<int32_t>> adj(numNodes, std::vector<int32_t>(numNodes, -1));

    int32_t edgeCount = 0;
    std::string line;
    for (int32_t u = 0; u < numNodes; ++u) {
        if (!std::getline(infile, line)) {
            throw std::runtime_error("Unexpected end of file while reading adjacency list");
        }

        std::istringstream iss(line);
        int32_t neighbor, weight;
        while (iss >> neighbor >> weight) {
            if (neighbor < 0 || neighbor >= numNodes) {
                throw std::runtime_error("Neighbor index out of range in file");
            }
            adj[u][neighbor] = weight;
            ++edgeCount;
        }
    }

    // Validate edge count
    if (edgeCount != numEdges) {
        throw std::runtime_error(
            "Edge count mismatch: expected " + std::to_string(numEdges) +
            ", but found " + std::to_string(edgeCount)
        );
    }

    // All FPGAs are connected to themselves with a distance of 0
    // thus the diagonal of the matrix is all 0
    for (int32_t diag = 0; diag < adj.size(); diag++) {
      adj[diag][diag] = 0;
    }

    return adj;
}

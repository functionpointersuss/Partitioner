#include "partitioner.hpp"

void partition(const std::string &fpga_graph_file, char* netlist_graph_file, const int32_t fpga_size) {
  mt_kahypar_error_t error{};

  // Initialize
  mt_kahypar_initialize(
    std::thread::hardware_concurrency() /* use all available cores */,
    true /* activate interleaved NUMA allocation policy */ );


  // Setup partitioning context
  mt_kahypar_context_t* context = mt_kahypar_context_from_preset(DEFAULT);
  // In the following, we partition a hypergraph into two blocks
  // with an allowed imbalance of 3% and optimize the connective metric (KM1)
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

  // Only partition to a subset of FPGAs, based on the design size
  int num_gates = mt_kahypar_num_hyperedges(hypergraph);
  int num_fpgas = (num_gates+fpga_size-1) / fpga_size;

  std::cout << "Design Size: " << num_gates << '\n';
  std::cout << "FPGA Size: " << fpga_size << '\n';
  std::cout << "Num FPGAs Used : " << num_fpgas << '\n';

  // Create FPGA Graph
  auto [fpga_delay_graph, fpga_band_graph] = fpga_graph_from_file(fpga_graph_file, num_fpgas);

  std::cout << num_fpgas << std::endl;
  std::cout << netlist_graph_file << std::endl;

  mt_kahypar_set_partitioning_parameters(context,
    num_fpgas /* number of blocks */, 0.10 /* imbalance parameter */,
    KM1 /* objective function */);


  // Partition Hypergraph
  mt_kahypar_partitioned_hypergraph_t partitioned_hg =
    mt_kahypar_partition(hypergraph, context, &error);
  if (partitioned_hg.partitioned_hg == nullptr) {
    std::cout << error.msg << std::endl; std::exit(1);
  }

  // Route Hypergraph
  std::cout << "********************************************************************************\n";
  std::cout << "*                             System Routing...                                *\n";
  std::cout << "********************************************************************************\n";

  router router(fpga_delay_graph, fpga_band_graph, hypergraph, partitioned_hg);
  router.route();

  mt_kahypar_free_context(context);
  mt_kahypar_free_hypergraph(hypergraph);
  mt_kahypar_free_partitioned_hypergraph(partitioned_hg);
}

std::pair<std::vector<std::vector<int32_t>>, std::vector<std::vector<int32_t>>> fpga_graph_from_file(const std::string &filename, const int32_t num_fpgas) {
    std::ifstream infile(filename);
    if (!infile.is_open()) {
        throw std::runtime_error("Error opening file: " + filename);
    }

    int32_t numNodes, numEdges;
    infile >> numNodes >> numEdges;
    infile.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // move to next line
    if (numNodes < num_fpgas)
      throw std::runtime_error("More FPGAs required than provided by graph.\n");

    // Initialize adjacency matrix with -1 (no edge)
    std::vector<std::vector<int32_t>> delay_adj(num_fpgas, std::vector<int32_t>(num_fpgas, -1));
    std::vector<std::vector<int32_t>> band_adj(num_fpgas, std::vector<int32_t>(num_fpgas, -1));

    int32_t edgeCount = 0;
    std::string line;
    for (int32_t u = 0; u < num_fpgas; ++u) {
        if (!std::getline(infile, line)) {
            throw std::runtime_error("Unexpected end of file while reading adjacency list");
        }

        std::istringstream iss(line);
        int32_t neighbor, bandwidth, delay;
        while (iss >> neighbor >> bandwidth >> delay) {
            if (neighbor < 0 || neighbor >= numNodes) {
                throw std::runtime_error("Neighbor index out of range in file");
            }
            if (neighbor >= num_fpgas) {
              continue;
            }
            delay_adj[u][neighbor] = delay * bandwidth;
            band_adj[u][neighbor] = bandwidth;
            ++edgeCount;
        }
    }

    // Validate edge count
    // if (edgeCount != numEdges) {
    //     throw std::runtime_error(
    //         "Edge count mismatch: expected " + std::to_string(numEdges) +
    //         ", but found " + std::to_string(edgeCount)
    //     );
    // }

    // All FPGAs are connected to themselves with a distance of 0, and a bandwidth of 1 since we don't really care
    // thus the diagonal of the matrix is all 0
    for (int32_t diag = 0; diag < delay_adj.size(); diag++) {
      delay_adj[diag][diag] = 0;
      band_adj[diag][diag] = 1;
    }

    return std::make_pair(delay_adj, band_adj);
}

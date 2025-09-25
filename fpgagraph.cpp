#include "fpgagraph.hpp"

std::tuple<std::vector<std::vector<int32_t>>, std::vector<std::vector<int32_t>>, std::vector<std::vector<int32_t>>> fpga_graph_from_file(const std::string &filename, const int32_t num_fpgas) {
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
    std::vector<std::vector<int32_t>> hop_adj(num_fpgas, std::vector<int32_t>(num_fpgas, -1));

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
            hop_adj[u][neighbor] = 1;
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

    return std::make_tuple(delay_adj, band_adj, hop_adj);
}

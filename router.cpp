#include "router.hpp"

router::router(int32_t num_fpgas, std::vector<std::vector<int32_t>> fpga_graph,
               mt_kahypar_hypergraph_t& hypergraph, mt_kahypar_partitioned_hypergraph_t& partitioned_hypergraph)
             : num_fpgas(fpga_graph.size()), fpga_graph(fpga_graph), fpga_route_graph(fpga_graph),
               hypergraph(hypergraph), partitioned_hypergraph(partitioned_hypergraph) {}

// Calculates route effort
void router::route() {

  // -------------------------
  // FPGA Graph Floyd Warshall
  // -------------------------
  int32_t fpga_path_lengths[num_fpgas][num_fpgas];
  int32_t fpga_hop_counts[num_fpgas][num_fpgas];
  // Initialize fpga_path_lengths and fpga_hop_counts from the graph
  for (int32_t row = 0; row < num_fpgas; row++) {
    for (int32_t col = 0; col < num_fpgas; col++) {
      fpga_path_lengths[row][col] = fpga_graph[row][col];
      fpga_hop_counts[row][col] = (fpga_graph[row][col] > 0) ? 1 : fpga_graph[row][col];
    }
  }

  // Build a LUT of distances and hop counts to calculate path lengths later
  for (int32_t i = 0; i < num_fpgas; i++) {
    for (int32_t j = 0; j < num_fpgas; j++) {
      for (int32_t k = 0; k < num_fpgas; k++) {
        // Disconnected edges are represented by -1
        if (fpga_hop_counts[i][k] != -1 && fpga_hop_counts[k][j] != -1) {
          if (fpga_hop_counts[i][j] == -1)
            fpga_hop_counts[i][j] = fpga_hop_counts[i][k] + fpga_hop_counts[k][j];
          else
            fpga_hop_counts[i][j] = std::min(fpga_hop_counts[i][j], fpga_hop_counts[i][k] + fpga_hop_counts[k][j]);
        }
        if (fpga_path_lengths[i][k] != -1 && fpga_path_lengths[k][j] != -1) {
          if (fpga_path_lengths[i][j] == -1)
            fpga_path_lengths[i][j] = fpga_path_lengths[i][k] + fpga_path_lengths[k][j];
          else
            fpga_path_lengths[i][j] = std::min(fpga_path_lengths[i][j], fpga_path_lengths[i][k] + fpga_path_lengths[k][j]);
        }
      }
    }
  }

  // Get the maximum hop count in this graph
  int32_t max_hop_count = 0;
  for (int32_t row = 0; row < num_fpgas; row++)
    for (int32_t col = 0; col < num_fpgas; col++)
      max_hop_count = std::max(max_hop_count, fpga_hop_counts[row][col]);


  // --------------------
  // Edge list processing
  // --------------------

  std::vector<edge_path_t> edge_paths;
  int32_t max_edge_path_length = 0;
  // Run through all edges to get path lengths and max page length
  for (int32_t edge = 0; edge < mt_kahypar_num_hyperedges(hypergraph); edge++) {
    // Get source partition (we will say the 0th hypernode for now)
    mt_kahypar_hypernode_id_t num_nodes_in_edge = mt_kahypar_hyperedge_size(hypergraph, edge);
    mt_kahypar_hypernode_id_t *pin_buffer = (mt_kahypar_hypernode_id_t*) malloc(sizeof(mt_kahypar_hypernode_id_t)*num_nodes_in_edge);
    mt_kahypar_get_hyperedge_pins(hypergraph, edge, pin_buffer);
    mt_kahypar_partition_id_t source_part = mt_kahypar_block_id(partitioned_hypergraph, pin_buffer[0]);

    // Get all the path lengths for all the drain nodes and add the path to the list of edges
    for (int32_t drain_node_idx = 1; drain_node_idx < num_nodes_in_edge; drain_node_idx++) {
      mt_kahypar_partition_id_t drain_part = mt_kahypar_block_id(partitioned_hypergraph, pin_buffer[drain_node_idx]);

      // We only care about signals that actually cross partitions, so skip same partition signals
      if (drain_part == source_part)
        continue;

      int32_t edge_path_length = (fpga_path_lengths[source_part][drain_part]);
      edge_paths.push_back({.length = edge_path_length, .source_part = source_part, .drain_part = drain_part});
      max_edge_path_length = std::max(max_edge_path_length, edge_path_length);
    }
    free(pin_buffer);
  }

  // Run through all edges to get effort and add it to the edge list with the source and drain indices
  std::vector<std::tuple<int32_t, mt_kahypar_partition_id_t, mt_kahypar_partition_id_t>> edge_path_efforts;
  for (auto edge_path : edge_paths) {
    int32_t effort = ((edge_path.length * max_hop_count) + max_edge_path_length - 1) / max_edge_path_length;

    edge_path_efforts.push_back({effort, edge_path.source_part, edge_path.drain_part});
  }
  std::sort(edge_path_efforts.begin(), edge_path_efforts.end(), [](const std::tuple<int32_t, mt_kahypar_partition_id_t, mt_kahypar_partition_id_t>& a,
                                                                   const std::tuple<int32_t, mt_kahypar_partition_id_t, mt_kahypar_partition_id_t>& b) {
    return std::get<0>(a) > std::get<0>(b);
  });

  //----------------
  // Initial Routing
  //----------------
  std::vector<std::vector<int32_t>> edge_node_paths;
  for (const auto& edge_path_effort : edge_path_efforts) {
    int32_t effort = std::get<0>(edge_path_effort);
    int32_t source = std::get<1>(edge_path_effort);
    int32_t drain  = std::get<2>(edge_path_effort);

    std::vector<int32_t> edge_path = route_path(source, drain, effort);

    // Add the path to the FPGA graph, while updating the route graph
    for (int32_t path_node = 0; path_node < edge_path.size()-1; path_node++) {
      fpga_route_graph[edge_path[path_node]][edge_path[path_node+1]] += 1;
    }

    // Add it to the list of edge paths for later metrics calculation
    edge_node_paths.push_back(edge_path);
  }

  //---------------------
  // TODO: Rip-up Routing
  //---------------------

  //--------------------
  // Performance Metrics
  //--------------------
  int32_t max_delay = 0;
  std::vector<int32_t> max_path;
  for (const auto& edge_path : edge_node_paths) {
    // Traverse the path, getting all the delays and adding them up
    int32_t delay = 0;
    for (int32_t path_node = 0; path_node < edge_path.size()-1; path_node++) {
      delay += fpga_route_graph[edge_path[path_node]][edge_path[path_node+1]] + fpga_route_graph[edge_path[path_node+1]][edge_path[path_node]];
    }
    if (delay > max_delay) {
      max_delay = delay;
      max_path  = edge_path;
    }
  }
  std::cout << "Max Delay for this system: " << max_delay << std::endl;
  std::cout << "Max Path for this system: ";
  for (const auto& path : max_path)
   std::cout << path << " ";
  std::cout << std::endl;
  return;
}

std::vector<int32_t> router::route_path(int32_t source_node, int32_t target_node, int32_t maximum_hop_count) {
    using State = std::tuple<int32_t, int32_t, int32_t>; // (dist, node, hops)
    auto cmp = [](const State &a, const State &b) {
        return std::get<0>(a) > std::get<0>(b); // min-heap by distance
    };
    std::priority_queue<State, std::vector<State>, decltype(cmp)> pq(cmp);

    // dist[node][hops] = shortest distance to node using exactly hops
    std::vector<std::vector<int32_t>> dist(
        num_fpgas, std::vector<int32_t>(num_fpgas + 1, std::numeric_limits<int32_t>::max())
    );
    // predecessor[node][hops] for path reconstruction
    std::vector<std::vector<int32_t>> prev(
        num_fpgas, std::vector<int32_t>(num_fpgas + 1, -1)
    );

    dist[source_node][0] = 0;
    pq.push({0, source_node, 0});

    bool found_within_limit = false;
    int32_t best_hops = -1;

    while (!pq.empty()) {
        auto [d, u, hops] = pq.top();
        pq.pop();

        if (hops > num_fpgas) continue; // safeguard (no path longer than V-1 edges)

        if (u == target_node) {
            if (hops <= maximum_hop_count) {
                // Found valid path within hop count
                found_within_limit = true;
                best_hops = hops;
                break;
            } else if (best_hops == -1) {
                // First solution beyond hop count
                best_hops = hops;
                break;
            }
        }

        for (int32_t v = 0; v < num_fpgas; v++) {
            int32_t weight = fpga_graph[u][v];
            if (weight > 0) {
                if (dist[u][hops] + weight < dist[v][hops + 1]) {
                    dist[v][hops + 1] = dist[u][hops] + weight;
                    prev[v][hops + 1] = u;
                    pq.push({dist[v][hops + 1], v, hops + 1});
                }
            }
        }
    }

    std::vector<int32_t> path;
    if (best_hops == -1) {
        return path; // no path at all
    }

    // Reconstruct path from target_node at best_hops
    for (int32_t at = target_node, h = best_hops; at != -1; at = prev[at][h], --h) {
        path.push_back(at);
    }
    std::reverse(path.begin(), path.end());
    return path;
}

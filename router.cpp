#include "router.hpp"

// Calculates route effort
void route(std::vector<std::vector<int32_t>> fpga_delay_graph, std::vector<std::vector<int32_t>> fpga_band_graph, std::vector<std::vector<int32_t>> fpga_hop_graph,
                   std::vector<hypergraph_edge_t> edge_partition_list) {

  int32_t num_fpgas = fpga_delay_graph.size();
  std::vector<std::vector<int32_t>> fpga_orig_graph = fpga_delay_graph;


  // Generate LUTs for the FPGA Graph
  auto [fpga_node_dists, fpga_node_hops] = create_fpga_graph_luts(num_fpgas, fpga_delay_graph, fpga_band_graph);

  // Get the maximum hop count in this graph
  int32_t max_hop_count = 0;
  for (int32_t row = 0; row < fpga_node_hops.size(); row++)
    for (int32_t col = 0; col < fpga_node_hops[row].size(); col++)
      max_hop_count = std::max(max_hop_count, fpga_node_hops[row][col]);


  // --------------------
  // Edge list processing
  // --------------------
  std::vector<routable_edge_t> routable_edge_list;
  int32_t max_edge_node_dist = 0;
  int32_t edge_node_dist = 0;
  uint64_t source_part, drain_part;

  // Run through all edges to get path lengths and max node distance, and add them to a list of routable edges
  for (const auto& edge : edge_partition_list) {
    source_part = edge.source;
    for (const auto& drain : edge.drains) {
      drain_part = drain;

      // We only care about signals that actually cross partitions, so skip same partition signals
      if (drain_part == source_part)
        continue;

      int32_t edge_node_dist = (fpga_node_dists[source_part][drain_part]);
      routable_edge_list.push_back({.dist = edge_node_dist, .effort = 0, .source = source_part, .drain = drain_part});
      max_edge_node_dist = std::max(max_edge_node_dist, edge_node_dist);
    }
  }

  // Using the max distance we calculate the path's relative effort
  for (auto& routable_edge : routable_edge_list) {
    routable_edge.effort = ((routable_edge.dist * max_hop_count) + max_edge_node_dist - 1) / max_edge_node_dist;
  }
  // Sort the list of routable edges based on highest effort being first
  std::sort(routable_edge_list.begin(), routable_edge_list.end(), [](const routable_edge_t& a, const routable_edge_t& b) {
    return a.effort > b.effort;
  });

  //----------------
  // Initial Routing
  //----------------
  std::vector<std::vector<int32_t>> edge_node_paths;
  for (const auto& routable_edge : routable_edge_list) {
    int32_t allowed_hop_count = routable_edge.effort;
    std::vector<int32_t> edge_path = route_path(fpga_delay_graph, num_fpgas, routable_edge.source, routable_edge.drain, allowed_hop_count);
    while (edge_path.size() == 0) {
      std::cout << "Increasing hop count due to failed route\n";
      allowed_hop_count++;
      edge_path = route_path(fpga_delay_graph, num_fpgas, routable_edge.source, routable_edge.drain, allowed_hop_count);
    }

    // Add the path to the FPGA graph, while updating the route graph
    for (int32_t path_node = 0; path_node < edge_path.size()-1; path_node++) {
      fpga_delay_graph[edge_path[path_node]][edge_path[path_node+1]] += 1;
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
      delay += (fpga_delay_graph[edge_path[path_node]][edge_path[path_node+1]] + fpga_delay_graph[edge_path[path_node+1]][edge_path[path_node]]
             - fpga_orig_graph[edge_path[path_node]][edge_path[path_node+1]]) / (fpga_band_graph[edge_path[path_node]][edge_path[path_node+1]]);
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

  std::cout << "Max Path Delay for this system: ";
  for (int32_t max_path_node = 0; max_path_node < max_path.size()-1; max_path_node++) {
    std::cout << fpga_delay_graph[max_path[max_path_node]][max_path[max_path_node+1]] + fpga_delay_graph[max_path[max_path_node+1]][max_path[max_path_node]] << " ";
  }
  std::cout << std::endl;
  return;
}


std::pair<std::vector<std::vector<int32_t>>, std::vector<std::vector<int32_t>>> create_fpga_graph_luts (int32_t num_fpgas,
                                                                                                          std::vector<std::vector<int32_t>> fpga_delay_graph,
                                                                                                          std::vector<std::vector<int32_t>> fpga_hop_graph) {
  // -------------------------
  // FPGA Graph Floyd Warshall
  // -------------------------
  // Build a LUT of distances and hop counts to calculate path lengths later
  for (int32_t i = 0; i < num_fpgas; i++) {
    for (int32_t j = 0; j < num_fpgas; j++) {
      for (int32_t k = 0; k < num_fpgas; k++) {
        // Disconnected edges are represented by -1, do not use those values
        if (fpga_hop_graph[i][k] != -1 && fpga_hop_graph[k][j] != -1) {
          if (fpga_hop_graph[i][j] == -1)
            fpga_hop_graph[i][j] = fpga_hop_graph[i][k] + fpga_hop_graph[k][j];
          else
            fpga_hop_graph[i][j] = std::min(fpga_hop_graph[i][j], fpga_hop_graph[i][k] + fpga_hop_graph[k][j]);
        }
        if (fpga_delay_graph[i][k] != -1 && fpga_delay_graph[k][j] != -1) {
          if (fpga_delay_graph[i][j] == -1)
            fpga_delay_graph[i][j] = fpga_delay_graph[i][k] + fpga_delay_graph[k][j];
          else
            fpga_delay_graph[i][j] = std::min(fpga_delay_graph[i][j], fpga_delay_graph[i][k] + fpga_delay_graph[k][j]);
        }
      }
    }
  }

  return std::make_pair(fpga_delay_graph, fpga_hop_graph);
}

std::vector<int32_t> route_path(const std::vector<std::vector<int32_t>>& fpga_delay_graph, int32_t num_fpgas, int32_t source_node, int32_t drain_node, int32_t maximum_hop_count) {

    // Distance array, initialized to "infinity"
    std::vector<int32_t> dist(num_fpgas, std::numeric_limits<int32_t>::max());
    // Predecessor array for reconstructing path
    std::vector<int32_t> prev(num_fpgas, -1);
    // Min-heap priority queue (distance, vertex, hops)
    using State = std::tuple<int32_t, int32_t, int32_t>; // (dist, node, hops)
    auto cmp = [](const State &a, const State &b) {
        return std::get<0>(a) > std::get<0>(b);
    };
    std::priority_queue<State, std::vector<State>, decltype(cmp)> pq(cmp);

    // Initialize
    dist[source_node] = 0;
    pq.push({0, source_node, 0});

    while (!pq.empty()) {
      auto [d, u, hops] = pq.top();
      pq.pop();

      // Early exit if target reached within hop limit
      if (u == drain_node && hops <= maximum_hop_count) break;
      if (d > dist[u]) continue; // Skip outdated entry
      if (hops >= maximum_hop_count) continue; // Cannot expand further

      for (int32_t v = 0; v < num_fpgas; v++) {
        int32_t weight = fpga_delay_graph[u][v];
        if (weight > 0) { // Assuming 0 = no edge
          if (dist[u] + weight < dist[v]) {
            dist[v] = dist[u] + weight;
            prev[v] = u;
            pq.push({dist[v], v, hops + 1});
          }
        }
      }
    }

    // Reconstruct path if valid
    std::vector<int32_t> path;
    if (dist[drain_node] == std::numeric_limits<int32_t>::max()) {
      return path; // no path
    }

    // Rebuild path and check hop count
    for (int32_t at = drain_node; at != -1; at = prev[at]) {
      path.push_back(at);
    }
    std::reverse(path.begin(), path.end());

    if ((int32_t)path.size() - 1 > maximum_hop_count) {
        return {}; // exceeds hop count
    }
    return path;
}

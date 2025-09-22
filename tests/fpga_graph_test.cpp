#include <cstdint>
#include <vector>
#include <string>
#include "../partitioner.hpp"

int main() {
  std::vector<std::vector<int32_t>> fpga_graph_exp = {
    { 0,  1,  1, -1},
    { 1,  0, -1,  1},
    { 1, -1,  0,  1},
    {-1,  1,  1,  0}
  };

  std::vector<std::vector<int32_t>> fpga_graph_act = fpga_graph_from_file("../tests/basic_fpga.gr");
  for (int i = 0; i < fpga_graph_act.size(); i++) {
    if (i >= fpga_graph_exp.size()) {
      std::cout << "Actual Graph has too many rows\n";
      return 1;
    }
    for (int j = 0; j < fpga_graph_act[i].size(); j++) {
      if (j >= fpga_graph_exp[i].size()) {
        std::cout << "Actual Graph has too many cols\n";
        return 1;
      }
      if (fpga_graph_act[i][j] != fpga_graph_exp[i][j]) {
        std::cout << "Mismatch at " << i << ", " << j << " Expected: " << fpga_graph_exp[i][j] << " Actual: " << fpga_graph_act[i][j];
        return 1;
      }
    }
  }

  return 0;
}

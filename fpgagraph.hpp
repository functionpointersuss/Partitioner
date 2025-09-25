#pragma once
#include <iostream>
#include <fstream>
#include <sstream>
#include <tuple>
#include <vector>
#include <limits>

std::tuple<std::vector<std::vector<int32_t>>, std::vector<std::vector<int32_t>>, std::vector<std::vector<int32_t>>> fpga_graph_from_file(const std::string &filename, const int32_t num_fpgas);

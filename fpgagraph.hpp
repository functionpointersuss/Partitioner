#pragma once
#include <iostream>
#include <fstream>
#include <sstream>

std::pair<std::vector<std::vector<int32_t>>, std::vector<std::vector<int32_t>>> fpga_graph_from_file(const std::string &filename, const int32_t fpga_size);

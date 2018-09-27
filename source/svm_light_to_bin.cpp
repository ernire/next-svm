//
// Created by Ernir Erlingsson on 19.9.2018.
//

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <functional>
#include <cstring>
#include <iomanip>
#include "next_svm_io.h"

int main(int argc, char** argv) {
    if (argc == 1) {
        // Perform Tests
    } else if (argc == 3) {
        char* in_file = argv[1];
        char* out_file = argv[2];
        std::cout << "Parsing file size: " << get_file_size(in_file) << std::endl;
//        std::ifstream infile(in_file, std::ios::in | std::ifstream::binary);
//        std::ofstream outfile(out_file, std::ios::in | std::ofstream::binary);
//        if (infile.is_open() && outfile.is_open()) {
        if (!convert_light_to_bin(in_file, out_file, [](int total_samples, int no_of_features, int zero_features) ->
            void {
                double sparsity = ((double)zero_features / (double)(total_samples * no_of_features));
                std::cout << "Number of samples: " << total_samples << std::endl;
                std::cout << "Number of features: " << no_of_features << std::endl;
                std::cout << "Total Empty features: " << zero_features << std::endl;
                std::cout << "Sparsity ratio: " << std::fixed << std::setprecision(4) << sparsity*100 << "%"
                    << std::endl;
            })) {
            std::cout << "Unable to open input or output files: " << in_file << ", " << out_file << std::endl;
        }
    } else {
        std::cout << "Wrong number of arguments, should be 2 (or 0 for unit testing)" << std::endl <<
        "1: The input file to process" << std::endl << "2: The Output file" << std::endl;
        return -1;
    }
    return 0;
}

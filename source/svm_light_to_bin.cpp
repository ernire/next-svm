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
#include <cmath>
#include "next_svm_io.h"
#include "test_util.h"

void perform_unit_tests() {
    std::cout << "***Unit Test Suite of NextSVM IO***" << std::endl;
    char *in_file, *out_file;
    #ifdef _WIN32
        in_file = const_cast<char *>("..\\input\\small_test_sparse.txt");
        out_file = const_cast<char *>("..\\output\\out.bin");
    #else
        in_file = const_cast<char *>("../input/small_test_sparse.txt");
        out_file = const_cast<char *>("../output/out.bin");
    #endif
    // 1
    do_unit_test(const_cast<char *>("1. Testing file size"), [&in_file]() {
        return get_file_size(in_file) == 350617;
    });
    // 2
    do_unit_test(const_cast<char *>("2. Testing svm_light to bin conversion"), [&in_file, &out_file]() {
        bool r = false;
        if (!convert_light_to_bin(in_file, out_file, [&r](int total_samples, int no_of_features, int zero_features) {
            r = total_samples == 1000 && no_of_features == 30 && zero_features == 5;
        })) {}
        return r;
    });
    // 3
    do_unit_test(const_cast<char *>("3. Confirming bin file"), [&out_file]() {
        int max_features, total_samples, class_sample_1, class_sample_2;
        float feature_sample_1, feature_sample_2;
        std::ifstream is(out_file, std::ios::in | std::ifstream::binary);
        is.read((char*)&total_samples,sizeof(int));
        is.read((char*)&max_features,sizeof(int));
        is.read((char*)&class_sample_1,sizeof(int));
        is.seekg(sizeof(int)*(total_samples+1), std::istream::beg);
        is.read((char*)&class_sample_2,sizeof(int));
        is.read((char*)&feature_sample_1,sizeof(float));
        is.seekg(-1*sizeof(float), std::istream::end);
        is.read((char*)&feature_sample_2,sizeof(float));
        is.close();
        return max_features == 30 && total_samples == 1000 && class_sample_1 == 54 && class_sample_2 == 52
        && fabs(feature_sample_1 - 0.693624) < 0.0001 && fabs(feature_sample_2 -0.466426) < 0.0001;
    });
}

void light_to_bin(char *in_file, char *out_file) {
    std::cout << "Parsing file size: " << get_file_size(in_file) << std::endl;
    if (!convert_light_to_bin(in_file, out_file, [](int total_samples, int no_of_features, int zero_features) -> void {
        double sparsity = ((double) zero_features / (double) (total_samples * no_of_features));
        std::cout << "Number of samples: " << total_samples << std::endl;
        std::cout << "Number of features: " << no_of_features << std::endl;
        std::cout << "Total Empty features: " << zero_features << std::endl;
        std::cout << "Sparsity ratio: " << std::fixed << std::setprecision(4) << sparsity * 100 << "%"
                  << std::endl;
    })) {
        std::cout << "Unable to open input or output files: " << in_file << ", " << out_file << std::endl;
    }
}

int main(int argc, char **argv) {
    if (argc == 1) {
        perform_unit_tests();
    } else if (argc == 3) {
        char *in_file = argv[1];
        char *out_file = argv[2];
        light_to_bin(in_file, out_file);
    } else {
        std::cout << "Wrong number of arguments, should be 2 (or 0 for unit testing)" << std::endl <<
                  "1: The input file to process" << std::endl << "2: The Output file" << std::endl;
        return -1;
    }
    return 0;
}

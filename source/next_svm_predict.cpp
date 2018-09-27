//
// Created by Ernir Erlingsson on 21.9.2018.
//

#include <iostream>
#include <fstream>
#include "next_svm_predict.h"

int get_read_size(const int total_samples, const int total_readers, const int rank) {
    int part = (total_samples / total_readers);
    int reserve = total_samples % total_readers;
    // Some processes will need one more sample if the data size does not fit completely with the number of processes
    if (reserve > 0 && reserve < rank-1) {
        return part + 1;
    }
    return part;
}

next_svm_data load_samples(std::istream &is, const int total_readers, const int rank) {
    int max_features, total_samples;

    is.read((char*)&total_samples,sizeof(int));
    is.read((char*)&max_features,sizeof(int));
    auto* data_svm = new next(total_samples, max_features);

    std::cout << "Total samples: " << total_samples << std::endl;
    std::cout << "Features per sample: " << max_features << std::endl;

    int my_part_size = get_read_size(total_samples, max_features, total_readers, rank);

    auto classes = new int[total_samples];
    auto features = new float*[total_samples];

    for (int i = 0; i < total_samples; i++) {

    }
}

int main(int argc, char** argv) {

    if (argc != 2) {
        std::cout << "Wrong number of arguments, should be 2" << std::endl;
        return -1;
    }

    auto* in_file = argv[1];
    std::ifstream infile(in_file, std::ios::in | std::ifstream::binary);

    if (infile.is_open()) {
        load_samples(infile, 1, 0);
    }
}
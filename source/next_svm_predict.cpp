//
// Created by Ernir Erlingsson on 21.9.2018.
//

#include <iostream>
#include <fstream>
#include "next_svm_predict.h"
#include "next_svm_io.h"
#include "test_util.h"

void perform_io_unit_tests() {
    char *in_file = const_cast<char *>("..\\output\\out.bin");
    // 1
    do_unit_test(const_cast<char *>("1. Testing file size"), [&in_file]() {
        bool r = true;
        std::cout << "Starting Test" << std::endl;
        auto *data = new next_svm_data(in_file, 2, 0, 500);
        std::cout << "Reading bytes" << std::endl;
        int read_bytes = data->read_next_samples();
        std::cout << "Total samples " << data->total_number_of_samples << std::endl;
        std::cout << "Total features " << data->total_number_of_features << std::endl;
        std::cout << "remaining samples " << data->remaining_samples << std::endl;
        std::cout << "read bytes: " << read_bytes << std::endl;

        for (int i = 0; i < 4; i++) {
            std::cout << i << ":" << data->classes[i] << " ";
        }
        std::cout << std::endl;
//
//        for (int i = 0; i < 50; i++) {
//            std::cout << i%data->total_number_of_features << ":" << data->features[i] << " ";
//            if (i == data->total_number_of_features)
//                std::cout << std::endl;
//        }


        for (int j = 0; j < 500; j++) {
            for (int i = 0; i <= data->total_number_of_features; i++) {
                std::cout << i << ":" << data->features[j*(data->total_number_of_features+1)+i] << " ";
            }
            std::cout << std::endl;
        }

        delete data;
        return r;
    });
}

void perform_unit_tests() {
    perform_io_unit_tests();
}

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

    /*
    auto* data_svm = new next(total_samples, max_features);

    std::cout << "Total samples: " << total_samples << std::endl;
    std::cout << "Features per sample: " << max_features << std::endl;

    int my_part_size = get_read_size(total_samples, max_features, total_readers, rank);

    auto classes = new int[total_samples];
    auto features = new float*[total_samples];

    for (int i = 0; i < total_samples; i++) {

    }
     */
}

int main(int argc, char** argv) {

    if (argc == 1) {
        perform_unit_tests();
        return 0;
    } else if (argc != 2) {
        std::cout << "Wrong number of arguments, should be 2" << std::endl;
        return -1;
    }

    auto* in_file = argv[1];
    std::ifstream infile(in_file, std::ios::in | std::ifstream::binary);

    if (infile.is_open()) {
        load_samples(infile, 1, 0);
    }
}
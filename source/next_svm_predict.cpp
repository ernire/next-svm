//
// Created by Ernir Erlingsson on 21.9.2018.
//

#include <iostream>
#include <fstream>
#include "next_svm_predict.h"
#include "next_svm_io.h"
#include "test_util.h"

void perform_io_unit_tests() {
    auto *in_file = const_cast<char *>("..\\output\\out.bin");
    // 1
    do_unit_test(const_cast<char *>("1. Testing file read I"), [&in_file]() {
        bool r = true;
        // Single block, complete data
        auto *data = new next_svm_data(in_file, 1, 0, 10000);
        int read_bytes = data->read_next_samples();
        if (data->total_number_of_samples != 1000 || data->total_number_of_features != 30
            || data->remaining_samples != 0 || read_bytes != 124000 || data->classes[0] != 54
            || data->classes[data->total_number_of_samples - 1] != 52 || data->features[0] - 0.693624 > 0.001
            || data->features[data->total_number_of_samples * data->total_number_of_features - 1] - 0.466426 > 0.001) {
            r = false;
        }
        return r;
    });
    // 2
    do_unit_test(const_cast<char *>("2. Testing file read II"), [&in_file]() {
        bool r = true;
        // 5 blocks, 1 block index, 100 read samples
        auto *data = new next_svm_data(in_file, 5, 1, 100);
        int read_bytes = data->read_next_samples();
        if (data->total_number_of_samples != 1000 || data->total_number_of_features != 30 ||
            data->remaining_samples != 100 || read_bytes != 12400 || data->classes[0] != 10 ||
            data->features[0] - 0.858864 > 0.001 || data->classes[99] != 17
            || data->features[100*data->total_number_of_features-1] - 0.468884 > 0.001) {
            r = false;
        }
        delete data;
        return r;
    });
    // 3
    do_unit_test(const_cast<char *>("3. Testing file read III"), [&in_file]() {
        bool r = true;
        // 2 blocks, 0 block index, 100 read samples
        auto *data = new next_svm_data(in_file, 2, 0, 100);
        data->read_next_samples();
        data->read_next_samples();
        int read_bytes = data->read_next_samples();
        if (data->total_number_of_samples != 1000 || data->total_number_of_features != 30 ||
            data->remaining_samples != 200 || read_bytes != 12400 || data->classes[0] != 10 ||
            data->features[0] - 0.858864 > 0.001 || data->classes[99] != 17
            || data->features[100*data->total_number_of_features-1] - 0.468884 > 0.001) {
            r = false;
        }
        delete data;
        return r;
    });
    // 4
    do_unit_test(const_cast<char *>("4. Testing file read IV"), [&in_file]() {
        bool r = true;
        // 3 blocks, 0 block index, 100 read samples
        auto *data = new next_svm_data(in_file, 3, 0, 100);
        int read_bytes = data->read_next_samples();
        if (read_bytes != 12400 || data->remaining_samples != 234) {
            r = false;
        }
        delete data;
        return r;
    });
    // 5
    do_unit_test(const_cast<char *>("5. Testing file read V"), [&in_file]() {
        bool r = true;
        // 3 blocks, 1 block index, 100 read samples
        auto *data = new next_svm_data(in_file, 3, 1, 100);
        int read_bytes = data->read_next_samples();
        if (read_bytes != 12400 || data->remaining_samples != 233) {
            r = false;
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
    if (reserve > 0 && reserve < rank - 1) {
        return part + 1;
    }
    return part;
}

next_svm_data load_samples(std::istream &is, const int total_readers, const int rank) {
    int max_features, total_samples;

    is.read((char *) &total_samples, sizeof(int));
    is.read((char *) &max_features, sizeof(int));
}

int main(int argc, char **argv) {

    if (argc == 1) {
        perform_unit_tests();
        return 0;
    } else if (argc != 2) {
        std::cout << "Wrong number of arguments, should be 2" << std::endl;
        return -1;
    }

    auto *in_file = argv[1];
    std::ifstream infile(in_file, std::ios::in | std::ifstream::binary);

    if (infile.is_open()) {
        load_samples(infile, 1, 0);
    }
}
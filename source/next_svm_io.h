//
// Created by Ernir Erlingsson on 27.9.2018.
//

#ifndef NEXT_SVM_NEXT_SVM_IO_H
#define NEXT_SVM_NEXT_SVM_IO_H


#include <istream>
#include <functional>

const int DEFAULT_BUFFER_SAMPLE_SIZE = 1000;

class next_svm_data {
private:
    const char *in_file;
    const int number_of_blocks, block_index, max_samples_per_batch;
    int class_offset = -1;
    int feature_offset = -1;

    void init_meta_data(std::istream &is);

public:
    int *classes;
    float *features;
    int remaining_samples;
    int total_number_of_samples = 0, total_number_of_features = 0;

    next_svm_data(char *in_file, int number_of_blocks, int block_index, int max_samples_per_batch);

    ~next_svm_data();

    int read_next_samples();
};

std::streampos get_file_size(const char *filePath);

bool convert_light_to_bin(char *in_file, char *out_file, const std::function<void(int, int, int)> &meta_callback);

#endif //NEXT_SVM_NEXT_SVM_IO_H

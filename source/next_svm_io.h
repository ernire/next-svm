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
    const int number_of_blocks;
    const int block_index;
    const char* in_file;
    int number_of_samples = 0;
    int number_of_features = 0;
    int class_offset = -1;
    int feature_offset = -1;
    int* classes;
    float* features;
    void init_meta_data(std::istream &is);
public:
    next_svm_data(char* in_file, int number_of_blocks, int block_index);
    bool read_next_samples(int max_samples, const std::function<void(int*, float**, int, int)> &read_callback);
};
std::streampos get_file_size(const char *filePath);
bool convert_light_to_bin(char* in_file, char* out_file, const std::function<void(int, int, int)> &meta_callback);

#endif //NEXT_SVM_NEXT_SVM_IO_H

//
// Created by Ernir Erlingsson on 27.9.2018.
//

#ifndef NEXT_SVM_NEXT_SVM_IO_H
#define NEXT_SVM_NEXT_SVM_IO_H


#include <istream>
#include <functional>

class next_svm_data {
private:
    const int number_of_parts;
    const int part_index;
    int number_of_samples;
    int number_of_features;
    int get_part_offset();
    int get_part_offset(int part_index);
    int get_part_size(int part_index);
public:
    next_svm_data(int number_of_parts, int part_index);
    bool load_data(std::istream &is);
    int get_part_size();
};

std::streampos get_file_size(const char *filePath);
bool convert_light_to_bin(char* in_file, char* out_file, const std::function<void(int, int, int)> &meta_callback);

#endif //NEXT_SVM_NEXT_SVM_IO_H

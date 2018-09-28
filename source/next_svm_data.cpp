//
// Created by Ernir Erlingsson on 26.9.2018.
//

/*
#include "next_svm_data.h"

int next_svm_data::get_part_size() {
    return get_part_size(block_index);
}

next_svm_data::next_svm_data(int number_of_blocks, int block_index) :
number_of_blocks(number_of_blocks), block_index(block_index) {

}

bool next_svm_data::load_data(std::istream &is) {

    is.read((char*)&number_of_samples,sizeof(int));
    is.read((char*)&number_of_features,sizeof(int));
    auto** matrix = new float*[number_of_samples];
    for (int i = 0; i < number_of_samples; i++){
        matrix[i] = new float[number_of_features];
    }
    int offset = 2*sizeof(int) + get_part_offset()*sizeof(float);
    is.seekg(offset, std::istream::beg);


    return false;
}

int next_svm_data::get_part_offset() {
    return get_part_offset(block_index);
}

int next_svm_data::get_part_size(const int index) {
    int part = (number_of_samples / number_of_blocks);
    int reserve = number_of_samples % number_of_blocks;
    // Some processes will need one more sample if the data size does not fit completely with the number of processes
    if (reserve > 0 && reserve < index-1) {
        return part + 1;
    }
    return part;
}

int next_svm_data::get_part_offset(int index) {
    int offset = 0;
    for (int i = 0; i < index; i++) {
        offset += get_part_size(i);
    }
    return offset;
}
*/

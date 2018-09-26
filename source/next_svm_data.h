//
// Created by Ernir Erlingsson on 26.9.2018.
//

#ifndef NEXT_SVM_NEXT_SVM_DATA_H
#define NEXT_SVM_NEXT_SVM_DATA_H

#include <istream>

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


#endif //NEXT_SVM_NEXT_SVM_DATA_H

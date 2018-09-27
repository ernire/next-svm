//
// Created by Ernir Erlingsson on 27.9.2018.
//

#ifndef NEXT_SVM_NEXT_SVM_IO_H
#define NEXT_SVM_NEXT_SVM_IO_H


#include <istream>
#include <functional>

class next_svm_io {

};

std::streampos get_file_size(const char *filePath);
void convert_light_to_bin(std::istream &is, std::ostream &os, const std::function<void(int, int, int)> &meta_callback);

#endif //NEXT_SVM_NEXT_SVM_IO_H

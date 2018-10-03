//
// Created by Ernir Erlingsson on 1.10.2018.
//

#ifndef NEXT_SVM_TEST_UTIL_H
#define NEXT_SVM_TEST_UTIL_H


#include <functional>

void do_unit_test(char *string, const std::function<bool()> &test_case);


#endif //NEXT_SVM_TEST_UTIL_H

#include "pybind11/pybind11.h"

int add(int i, int j) {
    return i + j;
}



PYBIND11_MODULE(cstuff, m) {
    m.doc() = "stuff here";
    m.def("add", &add, "add 2 numbers");
}
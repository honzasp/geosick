#include <pybind11/chrono.h>
#include <pybind11/functional.h>
#include <pybind11/stl.h>
#include <pybind11/stl_bind.h>

#include "py.hpp"

std::string hello_world() {
    return "Hello World!";
}

PYBIND11_MODULE(_geosick, m) {
    namespace py = pybind11;

    m.def("hello_world", &hello_world);
}

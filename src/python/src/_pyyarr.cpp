//
// Created by wittgen on 5/8/21.
//
#include "ScanConsole.h"
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;
PYBIND11_MODULE(_pyyarr, m) {
    m.doc() = "YARR python bindings";
    py::class_<ScanConsole>(m, "ScanConsole")
    .def(py::init<>(),
           py::call_guard<py::gil_scoped_release>())
    .def("loadConfig", py::overload_cast<const char*>(&ScanConsole::loadConfig),
         py::call_guard<py::gil_scoped_release>())
    .def("loadConfig", py::overload_cast<>(&ScanConsole::loadConfig),
        py::call_guard<py::gil_scoped_release>())
    .def("init", py::overload_cast<const std::vector<std::string> &>(&ScanConsole::init),
            py::call_guard<py::gil_scoped_release>())
    .def("setupScan", &ScanConsole::setupScan,
         py::call_guard<py::gil_scoped_release>())
    .def("configure", &ScanConsole::configure,
         py::call_guard<py::gil_scoped_release>())
    .def("initHardware", &ScanConsole::initHardware,
         py::call_guard<py::gil_scoped_release>())
    .def("cleanup", &ScanConsole::cleanup,
         py::call_guard<py::gil_scoped_release>())
    .def("run", &ScanConsole::run,
         py::call_guard<py::gil_scoped_release>())
    .def("plot", &ScanConsole::plot,
         py::call_guard<py::gil_scoped_release>())
    .def("dump", &ScanConsole::dump,
         py::call_guard<py::gil_scoped_release>())
    .def("getResults", py::overload_cast<>(&ScanConsole::getResults),
         py::call_guard<py::gil_scoped_release>());
    m.def("parseConfig", &ScanConsole::parseConfig, py::call_guard<py::gil_scoped_release>());
    m.def("setupLogger", &ScanConsole::setupLogger, py::call_guard<py::gil_scoped_release>());
    m.def("getLog", &ScanConsole::getLog, py::call_guard<py::gil_scoped_release>());
}

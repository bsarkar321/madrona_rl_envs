#include "mgr.hpp"

#include <madrona/macros.hpp>

#if defined(MADRONA_CLANG) || defined(MADRONA_GCC)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weverything"
#endif
#include <nanobind/nanobind.h>
#include <nanobind/tensor.h>
#if defined(MADRONA_CLANG) || defined(MADRONA_GCC)
#pragma GCC diagnostic pop
#endif

namespace nb = nanobind;

namespace SimpleSpread {

NB_MODULE(madrona_simple_spread_example_python, m) {
    nb::enum_<Manager::ExecMode>(m, "ExecMode")
        .value("CPU", Manager::ExecMode::CPU)
        .value("CUDA", Manager::ExecMode::CUDA)
        .export_values();
    
    nb::class_<Manager> (m, "SimpleSpreadSimulator")
        .def("__init__", [](Manager *self,
                            Manager::ExecMode exec_mode,
                            int64_t gpu_id,
                            int64_t num_worlds,
                            bool debug_compile) {
            new (self) Manager(Manager::Config {
                .execMode = exec_mode,
                .gpuID = (int)gpu_id,
                .numWorlds = (uint32_t)num_worlds,
                .debugCompile = debug_compile,
            });
        }, nb::arg("exec_mode") , nb::arg("gpu_id"), nb::arg("num_worlds"),
           nb::arg("debug_compile") = true)
        .def("step", &Manager::step)
        .def("reset_tensor", &Manager::resetTensor)
        .def("action_tensor", &Manager::actionTensor)
        .def("observation_tensor", &Manager::stateTensor)
        .def("reward_tensor", &Manager::rewardTensor)
        .def("world_id_tensor", &Manager::worldIDTensor)
    ;
}

}

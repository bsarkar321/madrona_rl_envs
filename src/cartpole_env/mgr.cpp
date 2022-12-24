#include "mgr.hpp"
#include "sim.hpp"

#include <madrona/mw_gpu.hpp>
#include <madrona/cuda_utils.hpp>
#include <madrona/utils.hpp>
#include <madrona/importer.hpp>

#include <charconv>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <string>

using namespace madrona;
using namespace madrona::py;

namespace Cartpole {

struct Manager::Impl {
    Config cfg;
    EpisodeManager *episodeMgr;
    TrainingExecutor mwGPU;

    static inline Impl * init(const Config &cfg);
};

Manager::Impl * Manager::Impl::init(const Config &cfg)
{
    EpisodeManager *episode_mgr = 
        (EpisodeManager *)cu::allocGPU(sizeof(EpisodeManager));

    // Set the current episode count to 0
    REQ_CUDA(cudaMemset(episode_mgr, 0, sizeof(EpisodeManager)));

    HeapArray<WorldInit> world_inits(cfg.numWorlds);

    for (int64_t i = 0; i < (int64_t)cfg.numWorlds; i++) {
        world_inits[i] = WorldInit {
            episode_mgr,
        };
    }

    TrainingExecutor mwgpu_exec({
        .worldInitPtr = world_inits.data(),
        .numWorldInitBytes = sizeof(WorldInit),
        .numWorldDataBytes = sizeof(Sim),
        .worldDataAlignment = alignof(Sim),
        .numWorlds = cfg.numWorlds,
        // Increase this number before exporting more tensors
        .numExportedBuffers = 4, 
        .gpuID = (uint32_t)cfg.gpuID,
        .renderWidth = 0,
        .renderHeight = 0,
    }, {
        "",
        { CARTPOLE_SRC_LIST },
        { CARTPOLE_COMPILE_FLAGS },
        cfg.debugCompile ? CompileConfig::OptMode::Debug :
            CompileConfig::OptMode::LTO,
        CompileConfig::Executor::TaskGraph,
    });

    return new Impl {
        cfg,
        episode_mgr,
        std::move(mwgpu_exec),
    };
}

MADRONA_EXPORT Manager::Manager(const Config &cfg)
    : impl_(Impl::init(cfg))
{}

MADRONA_EXPORT Manager::~Manager() {}

MADRONA_EXPORT void Manager::step()
{
    impl_->mwGPU.run();
}

MADRONA_EXPORT GPUTensor Manager::resetTensor() const
{
    void *dev_ptr = impl_->mwGPU.getExported(0);

    return GPUTensor(dev_ptr, GPUTensor::ElementType::Int32,
                     {impl_->cfg.numWorlds, 1}, impl_->cfg.gpuID);
}

MADRONA_EXPORT GPUTensor Manager::actionTensor() const
{
    void *dev_ptr = impl_->mwGPU.getExported(1);

    return GPUTensor(dev_ptr, GPUTensor::ElementType::Int32,
                     {impl_->cfg.numWorlds, 1, 1}, impl_->cfg.gpuID);
}

MADRONA_EXPORT GPUTensor Manager::stateTensor() const
{
    void *dev_ptr = impl_->mwGPU.getExported(2);

    return GPUTensor(dev_ptr, GPUTensor::ElementType::Float32,
                     {impl_->cfg.numWorlds, 1, 4}, impl_->cfg.gpuID);
}

MADRONA_EXPORT GPUTensor Manager::rewardTensor() const
{
    void *dev_ptr = impl_->mwGPU.getExported(3);

    return GPUTensor(dev_ptr, GPUTensor::ElementType::Float32,
                     {impl_->cfg.numWorlds, 1, 1}, impl_->cfg.gpuID);
}

}

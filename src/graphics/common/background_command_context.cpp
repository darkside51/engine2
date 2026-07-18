#include "background_command_context.h"

namespace engine::graphics {
    BackgroundCommandContext::BackgroundCommandContext(threads::Worker * worker, rhi::runtime::Rhi & api, rhi::runtime::Device & device, uint8_t frames) :
        _api(api), 
        _device(device), 
        _worker(worker), 
        _frameIndex(std::max(frames, uint8_t{2u})) // min 2u to begin new command buffer after grab task on current
    {
        _worker->emplaceTasks(stl::make_vector(threads::WorkerTask{[this]() mutable {
                auto * commandPool = _device.acquireCommandPool(engine::rhi::queue_flags::transfrer);
                _commandLists.resize(_frameIndex.size());
                for (auto & commandList : _commandLists) {
                    commandList = _api.createCommandList(commandPool, engine::rhi::CommandBufferLevel::ePrimary);
                }
                _commandLists[0u]->begin(engine::rhi::CommandBufferUsageFlags::eOne_Time_Submit);
            }}));
    }

    BackgroundCommandContext::~BackgroundCommandContext() {
        destroy();
    }

    void BackgroundCommandContext::destroy() {
        if (_worker) {
            _worker->emplaceTask(threads::WorkerTask{[this]() mutable {
                _commandLists.clear();
            }});
            _worker = nullptr;
        }
    }
}
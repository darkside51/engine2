#pragma once

#include "graphics/render/rhi/common/runtime.h"
#include "graphics/render/rhi/common/command_list.h"
#include "graphics/render/rhi/common/utils.h"
#include "threads/worker.h"

#include "stl/vector.h"

namespace engine::graphics {

class BackgroundCommandContext {
public:
    using Command = threads::function<void(rhi::runtime::CommandList & commandList, stl::vector<threads::WorkerTask> & callbacks)>;

    BackgroundCommandContext(threads::Worker * worker, rhi::runtime::Rhi & api, rhi::runtime::Device & device, uint8_t frames);
    ~BackgroundCommandContext();

    void destroy();

    void submit(stl::vector<Command> && commands, auto && grabber) {
        if (commands.empty()) {
            return;
        }

        _worker->emplaceTask(threads::WorkerTask{[this, grabber = std::move(grabber), commands = std::move(commands)]() mutable {
            const auto frameIdx = ++_frameIndex;
            auto & commandList = *_commandLists[frameIdx];
            _commandLists[frameIdx]->begin(engine::rhi::CommandBufferUsageFlags::eOne_Time_Submit);
            
            stl::vector<threads::WorkerTask> callbacks;
            callbacks.reserve(commands.size());

            for (auto & command : commands) {
                command(commandList, callbacks);
            }

            commandList.end();
            commandList.record(frameIdx);

            grabber(&commandList, std::move(callbacks));
        }});
        commands.clear();
    }

    rhi::runtime::Rhi & getApi() noexcept { return _api; }
    rhi::runtime::Device & getDevice() noexcept { return _device; }

private:
    rhi::runtime::Rhi & _api;
    rhi::runtime::Device & _device;
    threads::Worker * _worker;

    stl::vector<rhi::runtime::CommandListPtr> _commandLists;
    rhi::FrameIndex _frameIndex;
};

}
#pragma once

#include "graphics/render/rhi/common/runtime.h"
#include "graphics/render/rhi/common/command_list.h"
#include "graphics/render/rhi/common/utils.h"
#include "threads/worker.h"

#include <vector>

namespace engine::graphics {

class BackgroundCommandContext {
public:
    using Command = threads::function<void(rhi::runtime::CommandList & commandList, std::vector<threads::WorkerTask> & callbacks)>;

    BackgroundCommandContext(threads::Worker * worker, rhi::runtime::Rhi & api, rhi::runtime::Device & device, uint8_t frames);
    ~BackgroundCommandContext();

    void destroy();

    void submit(std::vector<Command> && commands, auto && grabber) {
        if (commands.empty()) {
            return;
        }

        _worker->emplaceTask(threads::WorkerTask{[this, grabber = std::move(grabber), commands = std::move(commands)]() mutable {
            std::vector<threads::WorkerTask> callbacks;
            callbacks.reserve(commands.size());

            const auto frameIdx = _frameIndex();
            auto & commandList = *_commandLists[frameIdx];
            for (auto & command : commands) {
                command(commandList, callbacks);
            }

            commandList.end();
            commandList.record(frameIdx);

            grabber(&commandList, std::move(callbacks));

            _commandLists[++_frameIndex]->begin(engine::rhi::CommandBufferUsageFlags::eOne_Time_Submit);
        }});
        commands.clear();
    }

private:
    rhi::runtime::Rhi & _api;
    rhi::runtime::Device & _device;
    threads::Worker * _worker;

    std::vector<rhi::runtime::CommandListPtr> _commandLists;
    rhi::FrameIndex _frameIndex;
};

}
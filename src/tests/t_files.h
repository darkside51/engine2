#pragma once

#include "../file/file_manager.h"
#include "../file/common_file_system.h"
#include "../log/log.h"
#include "../utils/assert.h"
#include <cstdio>

namespace engine::tests {

    struct FileTest {
        void main() {
            engine::CommonFileSystem fs;
            const auto files = fs.getFiles(true);
            const auto data = fs.readFile(".ninja_log");
            printf("%s\n", reinterpret_cast<const char*>(data.data()));

            auto result = fs.createDirectory("res/1/2");
            LOG_I("folder create: %s", result ? "success" : "error");

            result = fs.writeFile("res/1/2/1.txt", nullptr, 0);
            LOG_I("file write: %s", result ? "success" : "error");

            result = fs.removeDirectory("res/1");
            LOG_I("folder remove: %s", result ? "success" : "error");

            engine::FileManager fm;
            auto * fsystem = fm.createFileSystem<engine::CommonFileSystem>("main", engine::CommonFileSystem::getCurrentDirectory() + "files_test/");
            //fm.createFileSystem<engine::CommonFileSystem>("main");
            const auto * fsystem2 = fm.getFileSystem("main");
            fm.mapFileSystem(fsystem);

            ENGINE_BREAK_IF_NOT(fsystem2 == fsystem);

            result = fsystem->createDirectory("res/1/2");
            LOG_I("folder create: %s", result ? "success" : "error");
            result = fsystem->removeDirectory("res");
            LOG_I("folder remove: %s", result ? "success" : "error");
        }
    };

}
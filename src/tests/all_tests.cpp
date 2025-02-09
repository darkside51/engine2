#include "all_tests.h"

#include "../common/random.h"
#include "../log/log.h"

#include "test.h"
#include "t_logger.h"
#include "t_files.h"
#include "t_allocators.h"
#include "t_threads.h"
#include "t_check_leaks.h"
#include "t_properties.h"

namespace engine::tests {

    void small_test_collection() {
        #ifdef ENGINE_PLATFORM_WINDOWS
        LOG_I("platform Windows");
	    #elif ENGINE_PLATFORM_LINUX
        LOG_I("platform Linux");
        #endif


        // leaks
        new int();

        // random
        const auto r_float = common::random(-100.0f, 100.0f);
        const auto r_int = common::random(-100, 100);
        const auto r_float_hq = common::random_HQ(-100.0f, 100.0f);
        
    }

    void execute_tests() noexcept {
        small_test_collection();

        engine::tests::execute("Logger", engine::tests::LoggerTest{});
        engine::tests::execute("File", engine::tests::FileTest{});
        engine::tests::execute("Allocator", engine::tests::AllocatorsTest{});
        engine::tests::execute("Threads", engine::tests::ThreadsTest{});
        engine::tests::execute("Properties", engine::tests::PropertyTest{});
        engine::tests::execute("Leaks", engine::tests::LeaksTest{});
        
    }

}
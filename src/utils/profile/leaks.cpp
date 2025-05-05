#include "leaks.h"

#include "../../log/log.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>

#include <mutex>
#include <string_view>
#include <tuple>

#include <algorithm>
#include <array>
#include <cassert>
#include <limits>
#include <vector>

#ifdef CHECK_MEMORY_LEAKS

thread_local const char *__file__ = "<unknown>";
thread_local uint32_t __line__ = 0xff'ff'ff'ffu;

namespace engine::leaks {

    namespace {
        // https://en.cppreference.com/w/cpp/named_req/Allocator
        template<class T>
        struct Mallocator {
            typedef T value_type;
        
            Mallocator() = default;
                
            T* allocate(size_t n) noexcept {
                if (n > std::numeric_limits<size_t>::max() / sizeof(T)) {
                    assert(false);
                }
        
                if (auto p = static_cast<T*>(malloc(n * sizeof(T)))) {
                    return p;
                }
        
                assert(false);
                return nullptr;
            }
        
            void deallocate(T* p, std::size_t n) noexcept {
                free(p);
            }
        };

        inline const auto symplificateFileName(std::string_view fileName) noexcept {
            return fileName.substr(fileName.rfind("/") + 1u); 
        }
    }

    struct AllocationMetaInfo {
        bool array = false;
        size_t size = 0u;
        uint32_t line = 0u;
        const char *file = nullptr;
        
        AllocationMetaInfo * left = nullptr;
        AllocationMetaInfo * right = nullptr;

        AllocationMetaInfo(size_t size, uint32_t line, const char *file) :
            size(size), 
            line(line),
            file(file) {}

        ~AllocationMetaInfo() {
            left = nullptr;
            right = nullptr;
            size = 0u;
            line = 0u;
            file = nullptr;
        }

        void *operator new(size_t size) {
            return malloc(size);
        }

        void *operator new(size_t size, void* m) {
            return m;
        }

        void operator delete(void *ptr) noexcept {
            free(ptr);
        }

        static void eject(AllocationMetaInfo* info) noexcept {
            if (info->left) {
                info->left->right = info->right;
            }

            if (info->right) {
                info->right->left = info->left;
            }
        }

        static void insertBefore(AllocationMetaInfo* current, AllocationMetaInfo* info) noexcept {
            info->right = current;
            info->left = current->left;
            if (current->left) {
                current->left->right = info;
            }
            current->left = info;
        }

        static void insertAfter(AllocationMetaInfo* current, AllocationMetaInfo* info) noexcept {
            info->left = current;
            info->right = current->right;
            if (current->right) {
                current->right->left = info;
            }
            current->right = info;
        }
    };

    constexpr uint8_t kAlign = sizeof(size_t);
    constexpr uint8_t kAlignMask = kAlign - 1u;
    constexpr auto kAllocationAlign = ((sizeof(AllocationMetaInfo) + kAlignMask) & ~kAlignMask);

    inline void* getMemory(void * m) noexcept {
        return static_cast<std::byte*>(m) - kAllocationAlign;
    }

    AllocationMetaInfo *gAllocationInfo = nullptr;
    std::mutex gMutex;

    void poisonMemory(void * memory, size_t size, bool array = false) {
        //__file__=__FILE__;
        //__line__=__LINE__;

        auto * meta = new(memory) AllocationMetaInfo(size, __line__, __file__);
        meta->array = array;
        std::lock_guard<std::mutex> lock(gMutex);
        if (gAllocationInfo != nullptr) {
            meta->left = gAllocationInfo;
            gAllocationInfo->right = meta;
        }
        gAllocationInfo = meta;

        __line__ = 0xff'ff'ff'ffu;
        __file__ = "<unknown>";
    }

    bool unpoisonMemory(void* memory, bool array = false) noexcept {
        auto * info = static_cast<AllocationMetaInfo*>(memory);

        if (info->array != array) {
            const auto fileName = symplificateFileName(info->file);
            LOG_E("incorrect operator delete for allocation: file: %s, line: %d", fileName.data(), info->line);
        }

        std::lock_guard<std::mutex> lock(gMutex);
        AllocationMetaInfo::eject(info);
 
        if (info == gAllocationInfo) {
            gAllocationInfo = info->left;
        }

        return true;
    }

    void sortAllocations() noexcept {
        auto * root = gAllocationInfo;
        while (root) {
            auto * current = root->left;
            while (current) {   
                if (std::tie(current->file, current->line) < std::tie(root->file, root->line)) {
                    auto tmp = current->left;

                    AllocationMetaInfo::eject(current);
                    AllocationMetaInfo::insertAfter(root, current);

                    if (root == gAllocationInfo) {
                        gAllocationInfo = current;
                    }
                    root = current;
                    current = tmp;
                } else {
                    current = current->left;
                }
            }
            root = root->left;
        }
    }

    MemoryLeaksChecker::MemoryLeaksChecker() {
        static bool gLeaksCheckerCreated = false;
        assert(!gLeaksCheckerCreated);
        gLeaksCheckerCreated = true;
    }

    MemoryLeaksChecker::~MemoryLeaksChecker() noexcept {
        uint64_t counter = 0u;
        uint64_t bytes = 0u;
        if (gAllocationInfo != nullptr) {
            LOG_W("\t+++++++ detected memory leaks:");
            
            // sortAllocations();

            struct Leak {
                std::string_view str;
                uint32_t line = 0u;
                size_t size = 0u;
                uint32_t count = 0u;
                size_t total = 0u;

                Leak(std::string_view a, uint32_t l, size_t s) : str(a), line(l), size(s), count(1u), total(s) {}

                inline bool operator==(const Leak & l) const noexcept {
                    return str == l.str && line == l.line;
                }
            };

            constexpr size_t kShowLeaksCount = 1024u;
            std::vector<Leak, Mallocator<Leak>> leaks;
            leaks.reserve(kShowLeaksCount);
        
            // collect leaks
            while (gAllocationInfo) {
                const std::string_view str{gAllocationInfo->file};

                if (leaks.size() < leaks.capacity()) {
                    Leak l{str, gAllocationInfo->line, gAllocationInfo->size};
                    if (auto it = std::find(leaks.begin(), leaks.end(), l); it != leaks.end()) {
                        ++it->count;
                        it->total += l.size;
                    } else {
                        leaks.push_back(l);
                    }
                }

                //const auto file = str.substr(str.rfind("/") + 1u);
                //LOG_U("+ %s, line: %d\t(%zu bytes)", file.data(), gAllocationInfo->line, gAllocationInfo->size);
                ++counter;
                bytes += gAllocationInfo->size;
                auto * mem = gAllocationInfo;
                gAllocationInfo = gAllocationInfo->left;
                free(mem);
            }

            // sort leaks by files and lines
            std::sort(leaks.begin(), leaks.end(), [](const Leak & a, const Leak & b) noexcept {
                return std::tuple(a.str, a.line) < std::tuple(b.str, b.line);
            });

            for (const auto l : leaks) {
                if (l.line != 0xff'ff'ff'ffu) {
                    LOG_I("+ %s, line: %d\ttotal: %zu(bytes)\t(size: %zu(bytes), count: %u)", 
                        symplificateFileName(l.str).data(), l.line, l.total, l.size, l.count);
                } else {
                    LOG_I("+ %s, line: unknown \ttotal: %zu(bytes)\t(size: ?(bytes), count: %u)", 
                        symplificateFileName(l.str).data(), l.total, l.count);
                }
            }
        }
        LOG_I("\t+++++++ total leaks: %lu (%lu bytes)", counter, bytes);
    }

    void* alloc(size_t size, bool array) {
        const uintptr_t kAllocationSize = size + kAllocationAlign;

        void * memory = malloc(kAllocationSize);
        poisonMemory(memory, size, array);

        return static_cast<std::byte*>(memory) + kAllocationAlign;
    }

    void dealloc(void *ptr, bool array) noexcept {
        auto * memory = getMemory(ptr);
        if (unpoisonMemory(memory, array)) {
            free(memory);
        }
    }
}

void* operator new(size_t size) {
    return engine::leaks::alloc(size, false);
}

void* operator new[](size_t size) {
    return engine::leaks::alloc(size, true);
}

void operator delete(void *ptr) noexcept {
    engine::leaks::dealloc(ptr, false);
}

void operator delete[](void* ptr) noexcept {
    engine::leaks::dealloc(ptr, true);
}

#endif
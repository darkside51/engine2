#pragma once

#include "file_system.h"
#include "../common/hash.h"
#include "../threads/threads.h"
#include "../utils/assert.h"
#include "../utils/format_string.h"

#include <memory>
#include <unordered_map>
#include <vector>

namespace engine {

    class FileManager final {
		using Systems = std::unordered_map<std::string, std::unique_ptr<IFileSystem>, hash::String_hash, std::equal_to<>>;
    public:
        template<typename T, typename... Args>
		T* createFileSystem(std::string_view name, Args && ...args) {
			threads::WriteLock lock(_rwLock);
            auto [it, success] = _fileSystems.emplace(name, std::make_unique<T>(std::forward<Args>(args)...));
			ENGINE_BREAK_CONDITION2(success, fmt_string("file system with name \"%s\" already exist", name.data()));
			return static_cast<T*>(it->second.get());
        }
		
        void removeFileSystem(std::string_view name) noexcept {
			threads::RWSpinLock::readLock(_rwLock);
			auto it = _fileSystems.find(name);
			threads::RWSpinLock::readUnlock(_rwLock);
			if (it != _fileSystems.end()) {
				threads::WriteLock lock(_rwLock);
				unmapFileSystem(it->second.get());
				_fileSystems.erase(it);
			}
		}

		void removeFileSystem(const IFileSystem* fs) {
			threads::RWSpinLock::readLock(_rwLock);
			auto it = std::find_if(_fileSystems.begin(), _fileSystems.end(),
                       [&fs](const auto & p) { return p.second.get() == fs; });
			threads::RWSpinLock::readUnlock(_rwLock);
			if (it != _fileSystems.end()) {
				threads::WriteLock lock(_rwLock);
				unmapFileSystem(it->second.get());
				_fileSystems.erase(it);
			} else {
				// no such element
			}
		}

        const Systems & getFileSystems() const noexcept { return _fileSystems; }

		inline const IFileSystem* getFileSystem(std::string_view name) const noexcept {
			threads::ReadLock lock(_rwLock);
			auto it = _fileSystems.find(name);
			if (it != _fileSystems.end()) {
				return it->second.get();
			}

			return nullptr;
		}

		inline const IFileSystem* getFileSystemByFile(std::string_view path) const noexcept {
			threads::ReadLock lock(_rwLock);
			auto it = _mappedFiles.find(path);
			if (it != _mappedFiles.end()) {
				return it->second[0u];
			}

			for (auto const & [_, fs] : _fileSystems) {
				if (fs->hasFile(path)) {
					return fs.get();
				}
			}
			return nullptr;
		}

        void mapFileSystem(const IFileSystem* fs) {
			auto const files = fs->getFiles(true);
			for (const auto & file : files) {
				_mappedFiles[file].push_back(fs);
			}
		}

		void unmapFileSystem(const IFileSystem* fs) noexcept {
			for (auto it = _mappedFiles.begin(); it != _mappedFiles.end(); ) {
				auto & v = it->second;
				v.erase(std::remove(v.begin(), v.end(), fs), v.end());
				if (v.empty()) {
					it = _mappedFiles.erase(it);
				} else {
					++it;
				}
			}
		}

        //
        bool hasFile(std::string_view path) const noexcept {
			return getFileSystemByFile(path) != nullptr;
		}

        size_t fileSize(std::string_view path) const noexcept {
			if (const auto * fs = getFileSystemByFile(path)) {
				return fs->fileSize(path);
			}
			return 0u;
		}

		std::string getFullPath(std::string_view path) const {
			if (const auto * fs = getFileSystemByFile(path)) {
				return fs->fullPath(path);
			}
			return "";
		}

        stl::vector<std::byte> readFile(std::string_view path) const {
            if (const auto * fs = getFileSystemByFile(path)) {
				return fs->readFile(path);
			}
            return {};
        }

		bool readFile(std::string_view path, stl::vector<std::byte>& data) const {
            if (const auto * fs = getFileSystemByFile(path)) {
				return fs->readFile(path, data);
			}
            return false;
        }

		bool removeFile(std::string_view path) const {
			if (const auto * fs = getFileSystemByFile(path)) {
				return fs->removeFile(path);
			}
			return false;
		}

    private:
		mutable std::atomic<int32_t> _rwLock = {0};
        Systems _fileSystems;
        std::unordered_map<std::string, stl::vector<const IFileSystem*>, hash::String_hash, std::equal_to<>> _mappedFiles;
    };

}
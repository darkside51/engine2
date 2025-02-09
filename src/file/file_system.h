#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace engine {

    struct File;

	enum class SeekFile : uint8_t {
		eSet = 0u, // set file offset to offset
		eCur = 1u, // set file offset to current plus offset
		eEnd = 2u  // set file offset to EOF plus offset
	};

    class IFileSystem {
    public:
        virtual ~IFileSystem() = default;

		//
		virtual bool createDirectory(std::string_view path) const = 0;
		virtual bool removeDirectory(std::string_view path) const = 0;

        // file works
		virtual File* file_open(const char* path, const char* mode) const = 0;
		virtual void file_close(const File* f) const = 0;
		virtual int file_seek(File* f, uint64_t offset, SeekFile seek) const = 0;
		virtual size_t file_read(File* f, size_t size, size_t count, void* ptr) const = 0;
		virtual size_t file_write(File* f, size_t size, size_t count, const void* ptr) const = 0;
		virtual uint64_t file_tell(File* f) const = 0;

		//
		virtual std::vector<std::string> getFiles(bool recursive) const = 0;
		virtual bool hasFile(std::string_view path) const noexcept = 0;
		virtual size_t fileSize(std::string_view path) const noexcept = 0;

		virtual std::vector<std::byte> readFile(std::string_view path) const = 0;
		virtual bool readFile(std::string_view path, std::vector<std::byte>& data) const = 0;
		virtual bool writeFile(std::string_view path, const void* data, size_t sz, const char* mode = "wb") const = 0;
		virtual bool removeFile(std::string_view path) const = 0;

		virtual std::string fullPath(std::string_view path) const = 0;
		virtual std::string_view root() const noexcept = 0;
    };

}
#pragma once

#include "file_system.h"

namespace engine {
    
    class CommonFileSystem final : public IFileSystem {
    public:
		static std::string getCurrentDirectory() noexcept;
		
        ~CommonFileSystem() override = default;

        CommonFileSystem();
        CommonFileSystem(std::string_view root) : _root(root) {
			createRootDirectory();
		}

 		bool createDirectory(std::string_view path) const override;
		bool removeDirectory(std::string_view path) const override;

        File* file_open(const char* path, const char* mode) const override;
		void file_close(const File* f) const override;
		int file_seek(File* f, uint64_t offset, SeekFile seek) const override;
		size_t file_read(File* f, size_t size, size_t count, void* ptr) const override;
		size_t file_write(File* f, size_t size, size_t count, const void* ptr) const override;
		uint64_t file_tell(File* f) const override;

		std::vector<std::string> getFiles(bool recursive) const override;
		bool hasFile(std::string_view path) const noexcept override;
		size_t fileSize(std::string_view path) const noexcept override;
		size_t fileSize(std::string_view path, bool isFullPath) const noexcept;

		std::vector<std::byte> readFile(std::string_view path) const override;
		bool readFile(std::string_view path, std::vector<std::byte>& data) const override;

		bool writeFile(std::string_view path, const void* data, size_t sz, const char* mode = "wb") const override;

		bool removeFile(std::string_view path) const override;

		std::string fullPath(std::string_view path) const override;
		std::string_view root() const noexcept override;

    private:
		void createRootDirectory() const;
        std::string _root;
    };

}

#include "common_file_system.h"
#include "../utils/string_helper.h"

#include <filesystem>
#include <sys/stat.h>
#include <unistd.h>

namespace engine {

    void round(const std::filesystem::path& path, std::vector<std::string>& files, size_t rootLen, bool recursive) {
		for (const auto & entry : std::filesystem::directory_iterator(path)) {
			if (std::filesystem::is_directory(entry.status()) && recursive) {
				round(entry, files, rootLen, recursive);
			} else if (std::filesystem::is_regular_file(entry.status())) {
				//std::string file_path = entry.path().u8string();
				std::string file_path = entry.path().string();
				file_path = file_path.substr(rootLen, file_path.length() - rootLen);
				stringReplace(file_path, "\\", "/");
				files.push_back(file_path);
			}	
		}
	}

    std::string CommonFileSystem::getCurrentDirectory() noexcept {
        // return std::filesystem::current_path().string() + "/";
        char buff[FILENAME_MAX];
        getcwd(buff, FILENAME_MAX);
        return std::string(buff) + "/";
    }

	void CommonFileSystem::createRootDirectory() const {
		const std::filesystem::path pathToFile{ _root };
		std::error_code error;
		std::filesystem::create_directories(pathToFile, error);
	}

    CommonFileSystem::CommonFileSystem() : _root(getCurrentDirectory()) {}

    File* CommonFileSystem::file_open(const char* path, const char* mode) const {
        return reinterpret_cast<File*>(std::fopen(path, mode));
    }   

    void CommonFileSystem::file_close(const File* f) const {
        std::fclose(reinterpret_cast<FILE*>(const_cast<File*>(f)));
    }

	int CommonFileSystem::file_seek(File* f, uint64_t offset, SeekFile seek) const {
        return std::fseek(reinterpret_cast<FILE*>(f), offset, static_cast<int>(seek));
    }

	size_t CommonFileSystem::file_read(File* f, size_t size, size_t count, void* ptr) const {
        return std::fread(ptr, size, count, reinterpret_cast<FILE*>(f));
    }

	size_t CommonFileSystem::file_write(File* f, size_t size, size_t count, const void* ptr) const {
        return std::fwrite(ptr, size, count, reinterpret_cast<FILE*>(f));
    }

	uint64_t CommonFileSystem::file_tell(File* f) const {
        return std::ftell(reinterpret_cast<FILE*>(f));
    }

	//
    std::vector<std::string> CommonFileSystem::getFiles(bool recursive) const {
        std::vector<std::string> files;
        round(std::filesystem::path(_root), files, _root.length(), recursive);
		return files;
    }

	bool CommonFileSystem::hasFile(std::string_view path) const noexcept {
        struct stat s;
		return (stat(fullPath(path).c_str(), &s) == 0);
    }

	size_t CommonFileSystem::fileSize(std::string_view path) const noexcept {
        return fileSize(path, false);
    }

    size_t CommonFileSystem::fileSize(std::string_view path, bool isFullPath) const noexcept {
        struct stat s;
		if (isFullPath) {
			if (stat(path.data(), &s) == 0) { return s.st_size; }
		} else {
			if (stat(fullPath(path).c_str(), &s) == 0) { return s.st_size; }
		}
		return 0;
    }

	std::vector<std::byte> CommonFileSystem::readFile(std::string_view path) const {
        const std::string full_path = fullPath(path);
		const auto size = fileSize(full_path, true);
		if (size == 0u) return {};

		if (File* file = file_open(full_path.c_str(), "rb")) {
            std::vector<std::byte> data;
            data.resize(size);

			if (file_read(file, size, 1, data.data())) {
				file_close(file);
				return data;
			}

			file_close(file);
		}

		return {};
    }

	bool CommonFileSystem::readFile(std::string_view path, std::vector<std::byte>& data) const {
        const std::string full_path = fullPath(path);
		const size_t size = fileSize(full_path, true);
		if (size == 0u) return false;

		if (File* file = file_open(full_path.c_str(), "rb")) {
			data.resize(size);
			if (file_read(file, size, 1, data.data())) {
				file_close(file);
				return true;
			}
			file_close(file);
		}

		return false;
    }

	bool CommonFileSystem::writeFile(std::string_view path, const void* data, size_t sz, const char* mode) const {
        const std::string full_path = fullPath(path);
		const std::filesystem::path pathToFile{ full_path };
		std::error_code error;
		std::filesystem::create_directories(pathToFile.parent_path(), error); // create path folders

		if (error.value() != 0) {
			return false;
		}

		// write file data
		if (File* file = file_open(full_path.c_str(), mode)) {
			if (sz == 0u) {
				file_close(file);
				return true;
			}

			if (file_write(file, sz, 1, data)) {
				file_close(file);
				return true;
			}
			file_close(file);
		}
		return false;
    }

	bool CommonFileSystem::removeFile(std::string_view path) const {
		const std::filesystem::path pathTo{ fullPath(path) };
		return std::filesystem::remove(pathTo);
	}

	bool CommonFileSystem::createDirectory(std::string_view path) const {
		const std::filesystem::path pathToFile{ fullPath(path) };
		std::error_code error;
		std::filesystem::create_directories(pathToFile, error);
		return error.value() == 0; // create path folders
	}
	
	bool CommonFileSystem::removeDirectory(std::string_view path) const {
		const std::filesystem::path pathTo{ fullPath(path) };
		std::error_code error;
		const auto result = std::filesystem::remove_all(pathTo, error);
		return result;
	}

	std::string CommonFileSystem::fullPath(std::string_view path) const {
        return _root + std::string(path);
    }

	std::string_view CommonFileSystem::root() const noexcept {
		return _root;
	}

}
#pragma once

#include <cstdint>
#include <string>

namespace engine {
    inline uint16_t stringReplace(std::string & source, const std::string & find, const std::string & replace) noexcept {
		if (find.empty()) { return 0; }

		uint16_t num = 0;
		const size_t fLen = find.size();
		const size_t rLen = replace.size();
		for (size_t pos = 0; (pos = source.find(find, pos)) != std::string::npos; pos += rLen) {
			num++;
			source.replace(pos, fLen, replace);
		}

		return num;
	};
}
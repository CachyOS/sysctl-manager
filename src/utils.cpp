// Copyright (C) 2022-2025 Vladislav Nepogodin
//
// This file is part of CachyOS sysctl manager.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

#include "utils.hpp"

#include <cstdint>  // for int32_t
#include <cstdio>   // for FILE, fclose, fopen, fseek

#include <fstream>  // for ofstream
#include <ranges>   // for ranges::*
#include <string>   // for string

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wsign-conversion"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wuseless-cast"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wsuggest-attribute=pure"
#endif

#include <QProcess>

#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

namespace utils {

auto join_vec(std::span<std::string_view> lines, std::string_view delim) noexcept -> std::string {
    return lines | std::ranges::views::join_with(delim) | std::ranges::to<std::string>();
}

auto runCmdTerminal(QString cmd, bool escalate) noexcept -> std::int32_t {
    QProcess proc;
    cmd += "; read -p 'Press enter to exit'";
    auto paramlist = QStringList();
    if (escalate) {
        paramlist << "-s"
                  << "pkexec /usr/lib/cachyos-sysctl-manager/rootshell.sh";
    }
    paramlist << cmd;

    proc.start("/usr/lib/cachyos-sysctl-manager/terminal-helper", paramlist);
    proc.waitForFinished(-1);
    return proc.exitCode();
}

auto read_whole_file(std::string_view filepath) noexcept -> std::string {
    // Use std::fopen because it's faster than std::ifstream
    auto* file = std::fopen(filepath.data(), "rb");
    if (file == nullptr) {
        std::perror("read_whole_file");
        return {};
    }

    std::fseek(file, 0u, SEEK_END);
    const auto size = static_cast<std::size_t>(std::ftell(file));
    std::fseek(file, 0u, SEEK_SET);

    std::string buf;
    buf.resize(size);

    const std::size_t read = std::fread(buf.data(), sizeof(char), size, file);
    if (read != size) {
        std::perror("read_whole_file");
        std::fclose(file);
        return {};
    }
    std::fclose(file);

    return buf;
}

auto write_to_file(std::string_view filepath, std::string_view data) noexcept -> bool {
    std::ofstream file{std::string{filepath}};
    if (!file.is_open()) {
        std::perror("write_to_file");
        return false;
    }
    file << data;
    return true;
}

}  // namespace utils

// Copyright (C) 2022-2023 Vladislav Nepogodin
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

#include <algorithm>  // for transform
#include <cstdint>    // for int32_t
#include <cstdio>     // for FILE, fclose, fopen, fseek

#include <fmt/core.h>

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wold-style-cast"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnull-dereference"
#pragma GCC diagnostic ignored "-Wuseless-cast"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#endif

#include <range/v3/algorithm/for_each.hpp>
#include <range/v3/algorithm/reverse.hpp>
#include <range/v3/range/conversion.hpp>
#include <range/v3/view/filter.hpp>
#include <range/v3/view/join.hpp>
#include <range/v3/view/split.hpp>
#include <range/v3/view/transform.hpp>

#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

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

auto make_multiline(const std::string_view& str, char delim) noexcept -> std::vector<std::string> {
    static constexpr auto functor = [](auto&& rng) {
        return std::string_view(&*rng.begin(), static_cast<size_t>(ranges::distance(rng)));
    };
    static constexpr auto second = [](auto&& rng) { return rng != ""; };

    auto&& view_res = str
        | ranges::views::split(delim)
        | ranges::views::transform(functor);

    std::vector<std::string> lines{};
    ranges::for_each(view_res | ranges::views::filter(second), [&](auto&& rng) { lines.emplace_back(rng); });
    return lines;
}

auto join_vec(const std::span<std::string_view>& lines, const std::string_view&& delim) noexcept -> std::string {
    return lines | ranges::views::join(delim) | ranges::to<std::string>();
}

int runCmdTerminal(QString cmd, bool escalate) noexcept {
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

auto read_whole_file(const std::string_view& filepath) noexcept -> std::string {
    // Use std::fopen because it's faster than std::ifstream
    auto* file = std::fopen(filepath.data(), "rb");
    if (file == nullptr) {
        std::perror("read_whole_file");
        return {};
    }

    std::fseek(file, 0u, SEEK_END);
    const std::size_t size = static_cast<std::size_t>(std::ftell(file));
    std::fseek(file, 0u, SEEK_SET);

    std::string buf;
    buf.resize(size);

    const std::size_t read = std::fread(buf.data(), sizeof(char), size, file);
    if (read != size) {
        std::perror("read_whole_file");
        return {};
    }
    std::fclose(file);

    return buf;
}

}  // namespace utils

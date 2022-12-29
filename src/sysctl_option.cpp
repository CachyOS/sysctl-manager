// Copyright (C) 2022 Vladislav Nepogodin
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

#include "sysctl_option.hpp"
#include "utils.hpp"

#include <array>
#include <fstream>
#include <string>

#include <fmt/compile.h>
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

#include <range/v3/algorithm/contains.hpp>

#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

namespace fs = std::filesystem;

namespace {

static constexpr std::string_view DOC_ENDPOINT = "https://www.kernel.org/doc/html/latest/admin-guide/sysctl";
static constexpr std::array<std::string_view, 3> DEPRECATED{
    "base_reachable_time",
    "retrans_time",
    ""
};

constexpr inline std::string_view get_appendix_if_available(const std::string_view& entry) noexcept {
    if (entry.starts_with("net/ipv4")) {
        return "#proc-sys-net-ipv4-ipv4-settings";
    }
    if (entry.starts_with("net/core")) {
        return "#proc-sys-net-core-network-core-options";
    }
    if (entry.starts_with("fs/binfmt_misc")) {
        return "#proc-sys-fs-binfmt-misc";
    }
    if (entry.starts_with("fs/mqueue")) {
        return "#proc-sys-fs-mqueue-posix-message-queues-filesystem";
    }
    if (entry.starts_with("fs/epoll")) {
        return "#proc-sys-fs-epoll-configuration-options-for-the-epoll-interface";
    }
    return "";
}

constexpr inline std::string_view get_category(const std::string_view& entry) noexcept {
    return entry.substr(0, entry.find_first_of('/'));
}

}  // namespace

std::vector<SysctlOption> SysctlOption::get_options() noexcept {
    std::vector<SysctlOption> options{};

    for (const auto& dir_entry : fs::recursive_directory_iterator{PROC_PATH}) {
        if (fs::is_directory(dir_entry)) {
            // Skip directories
            continue;
        }

        // Remove proc path, to leave just the option path,
        // within the proc directory.
        std::string_view file_path = dir_entry.path().c_str();
        if (file_path.starts_with(PROC_PATH)) {
            file_path.remove_prefix(PROC_PATH.size());
        }

        // Skip `debug` and `dev`.
        if (file_path.starts_with("debug") || file_path.starts_with("dev")) {
            continue;
        }

        // Skip deprecated.
        if (ranges::contains(DEPRECATED, dir_entry.path().filename())) {
            continue;
        }

        // Generate doc link.
        std::string&& doc_link = fmt::format("{}/{}.html{}", DOC_ENDPOINT, get_category(file_path), get_appendix_if_available(file_path));

        // Parse option value.
        std::string&& option_value{"nil"};
        std::string file_content{};

        // Skip if failed to open file descriptor.
        std::ifstream file_stream{dir_entry.path().c_str()};
        if (!file_stream.is_open()) {
            continue;
        }
        // auto&& file_content = utils::read_whole_file(dir_entry.path().c_str());
        // if (file_content.empty()) {
        if (!std::getline(file_stream, file_content)) {
            fmt::print(stderr, "Failed to read := '{}'\n", dir_entry.path().c_str());
            continue;
        }
        option_value = std::move(file_content);
        utils::replace_all(option_value, "\t", " ");

        // Option name is path, with path delimeters('/') replaced with '.'.
        std::string option_name{file_path};
        utils::replace_all(option_name, "/", ".");

        auto option_obj = SysctlOption{file_path, option_name.data(), std::move(option_value), std::move(doc_link)};
        options.emplace_back(std::move(option_obj));
    }

    return options;
}

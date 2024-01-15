// Copyright (C) 2022-2024 Vladislav Nepogodin
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

#ifndef SYSCTL_OPTION_HPP
#define SYSCTL_OPTION_HPP

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

#include <fmt/core.h>

class SysctlOption {
 public:
    consteval SysctlOption() = default;
    explicit SysctlOption(const std::string_view& raw, const std::string_view& name, std::string&& value, std::string&& doc_link) : m_raw(raw), m_name(name), m_value(std::move(value)), m_doc(std::move(doc_link)) { }

    static constexpr std::string_view PROC_PATH = "/proc/sys/";

    /* clang-format off */
    inline std::string_view get_raw() const noexcept
    { return m_raw.c_str(); }

    inline std::string_view get_name() const noexcept
    { return m_name.c_str(); }

    inline std::string_view get_value() const noexcept
    { return m_value.c_str(); }

    inline std::string_view get_doc() const noexcept
    { return m_doc.c_str(); }
    /* clang-format on */

    static std::vector<SysctlOption> get_options() noexcept;

 private:
    std::string m_raw{};
    std::string m_name{};
    std::string m_value{};
    std::string m_doc{};
};

#endif  // SYSCTL_OPTION_HPP

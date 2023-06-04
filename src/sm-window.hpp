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

#ifndef MAINWINDOW_HPP_
#define MAINWINDOW_HPP_

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wsign-conversion"
#pragma clang diagnostic ignored "-Wfloat-conversion"
#pragma clang diagnostic ignored "-Wimplicit-int-float-conversion"
#pragma clang diagnostic ignored "-Wdeprecated-enum-enum-conversion"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wuseless-cast"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wdeprecated-enum-enum-conversion"
#pragma GCC diagnostic ignored "-Wsuggest-attribute=pure"
#endif

#include <ui_sm-window.h>

#include "sysctl_option.hpp"
#include "utils.hpp"

#include <array>
#include <condition_variable>
#include <memory>
#include <thread>
#include <vector>

#include <QMainWindow>
#include <QThread>
#include <QTimer>

#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

class Work final : public QObject {
    Q_OBJECT

 public:
    using function_t = std::function<void()>;
    explicit Work(function_t func)
      : m_func(func) { }
    virtual ~Work() = default;

 public slots:
    void doHeavyCalculations();

 private:
    function_t m_func;
};

namespace TreeCol {
enum { Name,
    Value,
    Displayed,
    Immutable };
}

class MainWindow final : public QMainWindow {
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(MainWindow)
 public:
    explicit MainWindow(QWidget* parent = nullptr);
    virtual ~MainWindow();

 private slots:
    void on_cancel() noexcept;
    void on_execute() noexcept;

    void find_options() noexcept;

    void on_item_double_clicked(QTreeWidgetItem* item, int column) noexcept;
    void item_changed(QTreeWidgetItem* item, int column) noexcept;

 protected:
    void closeEvent(QCloseEvent* event) override;

 private:
    std::atomic_bool m_running{};
    std::atomic_bool m_thread_running{true};
    std::mutex m_mutex{};
    std::condition_variable m_cv{};

    QStringList m_change_list{};

    QThread* m_worker_th = new QThread(this);
    Work* m_worker{nullptr};

    std::unique_ptr<Ui::MainWindow> m_ui = std::make_unique<Ui::MainWindow>();
    std::vector<SysctlOption> m_options{};

    void buildChangeList(QTreeWidgetItem* item) noexcept;
};

#endif  // MAINWINDOW_HPP_

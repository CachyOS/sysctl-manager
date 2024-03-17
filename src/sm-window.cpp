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

#include "sm-window.hpp"
#include "sysctl_option.hpp"
#include "utils.hpp"

#include <thread>

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wold-style-cast"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnull-dereference"
#pragma GCC diagnostic ignored "-Wuseless-cast"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#endif

#include <range/v3/algorithm/find_if.hpp>

#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

#include <fmt/core.h>

#include <QDesktopServices>
#include <QLineEdit>
#include <QTemporaryFile>
#include <QTextStream>
#include <QTreeWidgetItem>
#include <QUrl>

namespace {

auto generate_script_from_options(QTreeWidget* tree_options, std::span<QString> change_list) noexcept -> std::string {
    std::string bash_script;

    // Iterate through changed options,
    // and generate cmd to apply changes.
    for (auto&& option_name : change_list) {
        const auto& items = tree_options->findItems(option_name, Qt::MatchExactly, TreeCol::Name);
        /* clang-format off */
        if (items.isEmpty()) { continue; }
        /* clang-format on */

        const auto& found_item = items.at(0);

        auto option_name_str = option_name.toStdString();
        utils::replace_all(option_name_str, ".", "/");
        option_name_str = fmt::format("{}/{}", SysctlOption::PROC_PATH, option_name_str);

        auto bash_line = fmt::format("echo \"{}\" > {} && ", found_item->text(TreeCol::Value).toStdString(), option_name_str);
        bash_script += bash_line;
    }

    // Remove last occurencies of ' && '
    if (bash_script.ends_with(" && ")) {
        bash_script.erase(bash_script.size() - 4);
    }
    return bash_script;
}

void init_options_tree_widget(QTreeWidget* tree_options, std::span<SysctlOption> options) noexcept {
    for (auto&& sysctl_option : options) {
        auto* widget_item = new QTreeWidgetItem(tree_options); // NOLINT
        widget_item->setText(TreeCol::Name, sysctl_option.get_name().data());
        widget_item->setText(TreeCol::Value, sysctl_option.get_value().data());
        widget_item->setText(TreeCol::Displayed, QStringLiteral("true"));
        widget_item->setFlags(widget_item->flags() | Qt::ItemIsEditable);
    }
}

}  // namespace

MainWindow::MainWindow(QWidget* parent)
  : QMainWindow(parent) {
    m_ui->setupUi(this);

    setAttribute(Qt::WA_NativeWindow);
    setWindowFlags(Qt::Window);  // for the close, min and max buttons

    m_ui->ok->setEnabled(false);

    // Create worker thread
    m_worker = new Work([&]() {
        while (m_thread_running.load(std::memory_order_consume)) {
            std::unique_lock<std::mutex> lock(m_mutex);
            fmt::print(stderr, "Waiting... \n");

            m_cv.wait(lock, [&] { return m_running.load(std::memory_order_consume); });

            if (m_running.load(std::memory_order_consume) && m_thread_running.load(std::memory_order_consume)) {
                m_ui->ok->setEnabled(false);

                const auto bash_script = generate_script_from_options(m_ui->treeOptions, std::span{m_change_list});
                utils::runCmdTerminal(bash_script.c_str(), true);

                // Convert to vector of std::string
                std::vector<std::string> change_list(static_cast<std::size_t>(m_change_list.size()));
                for (int i = 0; i < m_change_list.size(); ++i) {
                    change_list[static_cast<std::size_t>(i)] = m_change_list[i].toStdString();
                }

                // Fetch new changes
                m_options.clear();
                m_options = SysctlOption::get_options();

                // Go through change_list and remove changed ones
                auto* tree_options = m_ui->treeOptions;
                for (auto&& option_name : change_list) {
                    if (auto result = ranges::find_if(m_options, [&option_name](auto&& option) { return option_name == option.get_name(); }); result != m_options.end()) {
                        if (auto items = tree_options->findItems(QString{option_name.c_str()}, Qt::MatchExactly, TreeCol::Name); !items.isEmpty()) {
                            const auto& item_value = items.at(0)->text(TreeCol::Value).toStdString();
                            if (item_value == result->get_value()) {
                                m_change_list.removeOne(QString{option_name.c_str()});
                            }
                        }
                    }
                }

                // Reset state
                m_running.store(false, std::memory_order_relaxed);
                m_ui->ok->setEnabled(!m_change_list.isEmpty());
            }
        }
    });

    m_worker->moveToThread(m_worker_th);
    // name to appear in ps, task manager, etc.
    m_worker_th->setObjectName("WorkerThread");

    auto* tree_options = m_ui->treeOptions;
    QStringList column_names;
    column_names << "Name"
                 << "Value";
    tree_options->setHeaderLabels(column_names);
    tree_options->hideColumn(TreeCol::Displayed);  // Displayed status true/false
    tree_options->hideColumn(TreeCol::Immutable);  // Immutable status true/false
    tree_options->header()->setSectionResizeMode(QHeaderView::ResizeToContents);

    tree_options->setContextMenuPolicy(Qt::CustomContextMenu);

    tree_options->setEditTriggers(QTreeWidget::NoEditTriggers);
    tree_options->blockSignals(true);

    m_options = SysctlOption::get_options();

    // TODO(vnepogodin): parallelize it
    auto a2 = std::async(std::launch::deferred, [&] {
        const std::lock_guard<std::mutex> guard(m_mutex);
        init_options_tree_widget(tree_options, std::span{m_options});
    });

    // Connect buttons signal
    connect(m_ui->cancel, &QPushButton::clicked, this, &MainWindow::on_cancel);
    connect(m_ui->ok, &QPushButton::clicked, this, &MainWindow::on_execute);

    // Connect worker thread signals
    connect(m_worker_th, &QThread::finished, m_worker, &QObject::deleteLater);
    connect(m_worker_th, &QThread::started, m_worker, &Work::doHeavyCalculations, Qt::QueuedConnection);

    // connect search box
    connect(m_ui->search_option, &QLineEdit::textChanged, this, &MainWindow::find_options);

    // Connect tree widget
    connect(tree_options, &QTreeWidget::itemChanged, this, &MainWindow::item_changed);
    connect(tree_options, &QTreeWidget::itemDoubleClicked, this, &MainWindow::on_item_double_clicked);

    // Wait for async function to finish
    a2.wait();
    tree_options->blockSignals(false);
}

MainWindow::~MainWindow() {
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    if (m_worker_th != nullptr) {
        m_worker_th->exit();
    }
}

// Find package in view
void MainWindow::find_options() noexcept {
    const auto& word = m_ui->search_option->text();
    /* clang-format off */
    if (word.length() == 1) { return; }
    /* clang-format on */

    auto* tree_options = m_ui->treeOptions;
    auto found_items   = tree_options->findItems(word, Qt::MatchContains, TreeCol::Name);

    for (QTreeWidgetItemIterator it(tree_options); *it; ++it) {
        (*it)->setHidden((*it)->text(TreeCol::Displayed) != QLatin1String("true") || !found_items.contains(*it));
    }
    for (int i = 0; i < tree_options->columnCount(); ++i) {
        tree_options->resizeColumnToContents(i);
    }
}

// When double-clicking on value column
void MainWindow::on_item_double_clicked(QTreeWidgetItem* item, int column) noexcept {
    switch (column) {
    case TreeCol::Value:
        m_ui->treeOptions->editItem(item, column);
        break;
    case TreeCol::Name: {
        auto&& item_name = item->text(TreeCol::Name).toStdString();
        if (auto result = ranges::find_if(m_options, [item_name = std::move(item_name)](auto&& option) { return item_name == option.get_name(); }); result != m_options.end()) {
            QDesktopServices::openUrl(QUrl(result->get_doc().data()));
        }
        break;
    }
    default:
        break;
    }
}

// When selecting on item in the list
void MainWindow::item_changed(QTreeWidgetItem* item, int) noexcept {
    build_changelist(item);
}

// Build the change_list when selecting on item in the tree
void MainWindow::build_changelist(QTreeWidgetItem* item) noexcept {
    const auto& item_value = item->text(TreeCol::Value);
    const auto& item_name  = item->text(TreeCol::Name);

    for (auto&& sysctl_option : m_options) {
        /* clang-format off */
        if (item_name != sysctl_option.get_name().data()) { continue; }
        /* clang-format on */

        if (item_value != sysctl_option.get_value().data()) {
            m_ui->ok->setEnabled(true);
            m_change_list.append(item_name);
            return;
        }
        if (item_value == sysctl_option.get_value().data()) {
            m_change_list.removeOne(item_name);
            return;
        }
    }

    if (m_change_list.isEmpty()) {
        m_ui->ok->setEnabled(false);
    }
}

void MainWindow::closeEvent(QCloseEvent* event) {
    // Exit worker thread
    m_running.store(true, std::memory_order_relaxed);
    m_thread_running.store(false, std::memory_order_relaxed);
    m_cv.notify_all();

    // Execute parent function
    QWidget::closeEvent(event);
}

void MainWindow::on_cancel() noexcept {
    close();
}

void Work::doHeavyCalculations() {
    m_func();
}

void MainWindow::on_execute() noexcept {
    if (m_running.load(std::memory_order_consume)) {
        return;
    }
    m_running.store(true, std::memory_order_relaxed);
    m_thread_running.store(true, std::memory_order_relaxed);
    m_cv.notify_all();
    m_worker_th->start();
}

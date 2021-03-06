#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include "memory/data.h"
#include "task/file_crypt.h"

#include <QDir>
#include <QFile>
#include <QString>
#include <QPushButton>
#include <QToolButton>
#include <QFileDialog>
#include <QMessageBox>

#include <chrono>
#include <filesystem>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
  ui->setupUi(this);

  CreateConnect();
}

MainWindow::~MainWindow() {
  delete ui;
}

void MainWindow::CreateConnect() {
  connect(ui->src_tool_button, &QToolButton::clicked, this, &MainWindow::SrcDirSelectSlot);
  connect(ui->dst_tool_button, &QToolButton::clicked, this, &MainWindow::DstDirSelectSlot);
  connect(ui->encrypt_push_button, &QPushButton::clicked, this, &MainWindow::EncryptSlot);
  connect(ui->decrypt_push_botton, &QPushButton::clicked, this, &MainWindow::DecryptSlot);
}

void MainWindow::SrcDirSelectSlot() {
  auto src_dir = SelectDir(ui->src_line_edit->text());
  if (!src_dir.isEmpty()) {
    ui->src_line_edit->setText(src_dir);
    src_dir_ = src_dir.toStdString();
  }
}

void MainWindow::DstDirSelectSlot() {
  auto dst_dir = SelectDir(ui->dst_line_edit->text());

  if (!dst_dir.isEmpty()) {
    ui->dst_line_edit->setText(dst_dir);
    dst_dir_ = dst_dir.toStdString();
  }
}

void MainWindow::EncryptSlot() {
  UpdateSrcAndDstDir();
  if (!CheckDir()) {
    return;
  }

  auto q_secret = ui->secret_line_edit->text();
  if (q_secret.length() < 16) {
    QMessageBox::critical(this, tr("Critical!!!"), tr("The password must contain at least 16 characters"));
    return;
  }

  ui->decrypt_push_botton->setEnabled(false);
  ui->progressBar->setValue(0);

  auto secret = q_secret.toStdString();
  Crypto(src_dir_, dst_dir_, secret, true);

  auto megabytes_per_second = static_cast<double>(read_bytes_) / (static_cast<double>(usage_times_) / 1000);

  auto info = QString(
      "Encrypt Success\n\nUsage time: %1 ms\nRead bytes: %2 MB\nCrypt bytes: %3 MB\nWrite bytes: %4 MB\n\nSpeed: %5 MB/s, %6 mbps\n")
      .arg(usage_times_)
      .arg(UnitConvert(read_bytes_, Unit::MB))
      .arg(UnitConvert(crypt_bytes_, Unit::MB))
      .arg(UnitConvert(write_bytes_, Unit::MB))
      .arg(UnitConvert(megabytes_per_second, Unit::MB))
      .arg(UnitConvert(megabytes_per_second, Unit::MB) * 8);

  ui->decrypt_push_botton->setEnabled(true);

  QMessageBox::information(this, tr("Success!!!"), info);
}

void MainWindow::DecryptSlot() {
  UpdateSrcAndDstDir();
  if (!CheckDir()) {
    return;
  }

  auto q_secret = ui->secret_line_edit->text();
  if (q_secret.length() < 16) {
    QMessageBox::critical(this, tr("Critical!!!"), tr("The password must contain at least 16 characters"));
    return;
  }

  ui->encrypt_push_button->setEnabled(false);
  ui->progressBar->setValue(0);

  auto secret = q_secret.toStdString();
  Crypto(src_dir_, dst_dir_, secret, false);

  auto megabytes_per_second = static_cast<double>(read_bytes_) / (static_cast<double>(usage_times_) / 1000);

  auto info = QString(
      "Decrypt Success\n\nUsage time: %1 ms\nRead bytes: %2 MB\nCrypt bytes: %3 MB\nWrite bytes: %4 MB\n\nSpeed: %5 MB/s, %6 mbps\n")
      .arg(usage_times_)
      .arg(UnitConvert(read_bytes_, Unit::MB))
      .arg(UnitConvert(crypt_bytes_, Unit::MB))
      .arg(UnitConvert(write_bytes_, Unit::MB))
      .arg(UnitConvert(megabytes_per_second, Unit::MB))
      .arg(UnitConvert(megabytes_per_second, Unit::MB) * 8);

  ui->encrypt_push_button->setEnabled(true);

  QMessageBox::information(this, tr("Success!!!"), info);
}

QString MainWindow::SelectDir(const QString &_dir) {
  return QFileDialog::getExistingDirectory(this, tr("???????????????"), _dir, QFileDialog::Option::ShowDirsOnly);
}

bool MainWindow::CheckDir() {
  if (src_dir_.empty()) {
    QMessageBox::critical(this, tr("Critical!!!"), tr("Src dir should not be empty!"));
    return false;
  }
  if (dst_dir_.empty()) {
    QMessageBox::critical(this, tr("Critical!!!"), tr("Dst dir should not be empty!"));
    return false;
  }

  if (!QDir(src_dir_.c_str()).exists()) {
    QMessageBox::critical(this, tr("Critical!!!"), tr("Src dir no exist!"));
    return false;
  }

  if (!QDir(dst_dir_.c_str()).exists()) {
    QMessageBox::critical(this, tr("Critical!!!"), tr("Dst dir no exist!"));
    return false;
  }

  return true;
}

void MainWindow::UpdateSrcAndDstDir() {
  src_dir_ = ui->src_line_edit->text().toStdString();
  dst_dir_ = ui->dst_line_edit->text().toStdString();
}

void MainWindow::Crypto(const ::std::string &_src_dir,
                        const ::std::string &_dst_dir,
                        const ::std::string &_secret,
                        bool _is_encrypt) {
  // ???????????????????????????
  auto memory_resource = std::make_shared<std::pmr::synchronized_pool_resource>();

  // ????????????
  auto file_crypt_list = std::list<std::shared_ptr<FileCrypt>>();

  read_bytes_ = 0;
  crypt_bytes_ = 0;
  write_bytes_ = 0;
  usage_times_ = 0;

  // ????????????????????????
  auto start_time_point = std::chrono::system_clock::now();

  // ??????????????????
  for (auto &it: std::filesystem::directory_iterator(_src_dir)) {
    // ???????????????
    if (!it.is_regular_file()) {
      continue;
    }

    auto file_crypt = std::make_shared<FileCrypt>();

    auto ok = file_crypt->Start(it.path().string(),
                                _dst_dir + "/" + it.path().filename().string(),
                                _secret,
                                _is_encrypt,
                                memory_resource);
    if (ok) {
      file_crypt_list.push_back(file_crypt);
    }
  }

  // ????????????????????????
  int index = 0;
  ui->progressBar->setMaximum(static_cast<int>(file_crypt_list.size()));
  for (auto &file_crypt: file_crypt_list) {
    file_crypt->Wait();
    read_bytes_ += file_crypt->read_bytes_;
    crypt_bytes_ += file_crypt->crypt_bytes_;
    write_bytes_ += file_crypt->write_bytes_;

    // TODO: ????????????
    ui->progressBar->setValue(++index);
  }

  usage_times_ = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::system_clock::now() - start_time_point).count();
}

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include <string>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
 Q_OBJECT
 private:
  ::std::string src_dir_;
  ::std::string dst_dir_;
  size_t read_bytes_ = 0;
  size_t crypt_bytes_ = 0;
  size_t write_bytes_ = 0;
  long long usage_times_ = 0;

 public:
  explicit MainWindow(QWidget *parent = nullptr);
  ~MainWindow() override;

 public:
  void CreateConnect();
  QString SelectDir(const QString& _dir);
  bool CheckDir();
  void UpdateSrcAndDstDir();
  void Crypto(const ::std::string &_src_dir,
              const ::std::string &_dst_dir,
              const ::std::string &_secret,
              bool _is_encrypt);

 private:
  Ui::MainWindow *ui;

 private slots:
  void SrcDirSelectSlot();
  void DstDirSelectSlot();
  void EncryptSlot();
  void DecryptSlot();
};
#endif // MAINWINDOW_H

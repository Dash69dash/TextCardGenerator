#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QSpinBox>
#include <QCheckBox>
#include <QComboBox>
#include <QFontComboBox>
#include <QImage>
#include <QStringList>
#include <QColor>

class MainWindow : public QMainWindow {
  Q_OBJECT

 public:
  explicit MainWindow(QWidget *parent = nullptr);

 private slots:
  void OnBrowse();
  void OnGenerate();
  void OnPickTextColor();
  void OnPickBgColor();
  void OnUpdatePreview();
  void OnSelectAll();
  void OnSelectNone();

 private:
  QTextEdit *input_edit_;
  QLineEdit *dir_edit_;
  QFontComboBox *font_combo_;
  QSpinBox *font_spin_;     // font pixel size
  QSpinBox *width_spin_;    // image width in px
  QPushButton *text_color_btn_;
  QPushButton *bg_color_btn_;
  QCheckBox *chk_png_;
  QCheckBox *chk_pdf_;
  QCheckBox *chk_docx_;
  QComboBox *orient_combo_;
  QSpinBox *pad_spin_;      // image inner padding in px
  QSpinBox *margin_spin_;   // word page margin in mm
  QLabel *preview_label_;
  QLabel *status_label_;

  QColor text_color_;
  QColor bg_color_;

  QList<QImage> cards_;
  QStringList card_names_;

  QImage RenderCard(const QString &text);
  QStringList SplitText(const QString &text, int max_chars);
  QString SafeFileName(const QString &text);

  void GeneratePdf(const QString &path);
  void GenerateDocx(const QString &path);

  bool WriteZip(const QString &path, const QMap<QString, QByteArray> &files);
  QByteArray MakeLocalHeader(const QString &name, quint32 crc, quint32 size);
  QByteArray MakeCentralHeader(const QString &name, quint32 crc, quint32 size,
                               quint32 local_offset);
  QByteArray MakeEocd(quint32 entry_count, quint32 central_directory_size,
                      quint32 central_directory_offset);
  quint32 Crc32Of(const QByteArray &data);
};

#endif  // MAINWINDOW_H

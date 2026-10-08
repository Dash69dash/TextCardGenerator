#include "mainwindow.h"

#include <QCoreApplication>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFormLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QRegularExpression>
#include <QFont>
#include <QFontMetrics>
#include <QPainter>
#include <QPdfWriter>
#include <QPageLayout>
#include <QPageSize>
#include <QFile>
#include <QDataStream>
#include <QBuffer>
#include <QDir>
#include <QSet>
#include <QColorDialog>
#include <QScrollArea>
#include <QApplication>
#include <QPixmap>
#include <QDateTime>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
  setWindowTitle(QStringLiteral("文字卡片生成器"));
  resize(900, 1000);

  text_color_ = Qt::white;
  bg_color_ = Qt::black;

  QWidget *central = new QWidget(this);
  QVBoxLayout *root = new QVBoxLayout(central);

  // ---- input ----
  root->addWidget(new QLabel(QStringLiteral("输入文字（多个词条用逗号或换行分隔）：")));
  input_edit_ = new QTextEdit;
  input_edit_->setPlaceholderText(
      QStringLiteral("例如：\n角的初步认识，角的特征\n边，顶点\n猜想，验证，结论"));
  input_edit_->setMaximumHeight(120);
  root->addWidget(input_edit_);

  // ---- settings grid ----
  QGridLayout *grid = new QGridLayout;

  int row = 0;
  grid->addWidget(new QLabel(QStringLiteral("字体：")), row, 0);
  font_combo_ = new QFontComboBox;
  font_combo_->setCurrentFont(QFont(QStringLiteral("SimSun")));
  font_combo_->setFixedHeight(40);
  grid->addWidget(font_combo_, row, 1);

  row++;
  grid->addWidget(new QLabel(QStringLiteral("字体像素大小：")), row, 0);
  font_spin_ = new QSpinBox;
  font_spin_->setRange(50, 1200);
  font_spin_->setValue(600);
  font_spin_->setSuffix(QStringLiteral(" px"));
  font_spin_->setFixedHeight(40);
  grid->addWidget(font_spin_, row, 1);

  row++;
  grid->addWidget(new QLabel(QStringLiteral("图片宽度：")), row, 0);
  width_spin_ = new QSpinBox;
  width_spin_->setRange(400, 4000);
  width_spin_->setValue(2580);
  width_spin_->setSuffix(QStringLiteral(" px"));
  width_spin_->setFixedHeight(40);
  grid->addWidget(width_spin_, row, 1);

  row++;
  grid->addWidget(new QLabel(QStringLiteral("文字颜色：")), row, 0);
  text_color_btn_ = new QPushButton;
  text_color_btn_->setFixedWidth(120);
  text_color_btn_->setStyleSheet(QStringLiteral("background-color: white; color: black;"));
  connect(text_color_btn_, &QPushButton::clicked, this, &MainWindow::OnPickTextColor);
  grid->addWidget(text_color_btn_, row, 1);

  row++;
  grid->addWidget(new QLabel(QStringLiteral("背景颜色：")), row, 0);
  bg_color_btn_ = new QPushButton;
  bg_color_btn_->setFixedWidth(120);
  bg_color_btn_->setStyleSheet(QStringLiteral("background-color: black; color: white;"));
  connect(bg_color_btn_, &QPushButton::clicked, this, &MainWindow::OnPickBgColor);
  grid->addWidget(bg_color_btn_, row, 1);

  row++;
  grid->addWidget(new QLabel(QStringLiteral("输出格式：")), row, 0);
  QHBoxLayout *chk_row = new QHBoxLayout;
  chk_png_ = new QCheckBox(QStringLiteral("图片 PNG"));
  chk_pdf_ = new QCheckBox(QStringLiteral("PDF"));
  chk_docx_ = new QCheckBox(QStringLiteral("Word"));
  chk_png_->setChecked(true);
  chk_pdf_->setChecked(false);
  chk_docx_->setChecked(true);
  QPushButton *sel_all_btn = new QPushButton(QStringLiteral("全选"));
  QPushButton *sel_none_btn = new QPushButton(QStringLiteral("全不选"));
  chk_row->addWidget(chk_png_);
  chk_row->addWidget(chk_pdf_);
  chk_row->addWidget(chk_docx_);
  chk_row->addStretch();
  chk_row->addWidget(sel_all_btn);
  chk_row->addWidget(sel_none_btn);
  grid->addLayout(chk_row, row, 1);
  connect(sel_all_btn, &QPushButton::clicked, this, &MainWindow::OnSelectAll);
  connect(sel_none_btn, &QPushButton::clicked, this, &MainWindow::OnSelectNone);
  // pdf not success, disabled it temp.
  chk_pdf_->setEnabled(false);

  row++;
  grid->addWidget(new QLabel(QStringLiteral("Word方向：")), row, 0);
  orient_combo_ = new QComboBox;
  orient_combo_->addItem(QStringLiteral("横版"));
  orient_combo_->addItem(QStringLiteral("竖版"));
  orient_combo_->setCurrentIndex(0);
  orient_combo_->setFixedHeight(40);
  grid->addWidget(orient_combo_, row, 1);

  row++;
  grid->addWidget(new QLabel(QStringLiteral("图片内边距：")), row, 0);
  pad_spin_ = new QSpinBox;
  pad_spin_->setRange(0, 500);
  pad_spin_->setValue(5);
  pad_spin_->setSuffix(QStringLiteral(" px"));
  pad_spin_->setFixedHeight(40);
  grid->addWidget(pad_spin_, row, 1);

  row++;
  grid->addWidget(new QLabel(QStringLiteral("Word页边距：")), row, 0);
  margin_spin_ = new QSpinBox;
  margin_spin_->setRange(0, 100);
  margin_spin_->setValue(5);
  margin_spin_->setSuffix(QStringLiteral(" mm"));
  margin_spin_->setFixedHeight(40);
  grid->addWidget(margin_spin_, row, 1);

  root->addLayout(grid);

  // Unify text size inside spin boxes and font combo (propagates to their
  // internal QLineEdit automatically).
  QFont input_font = font_spin_->font();
  input_font.setPixelSize(20);
  font_combo_->setFont(input_font);
  font_spin_->setFont(input_font);
  width_spin_->setFont(input_font);
  orient_combo_->setFont(input_font);
  pad_spin_->setFont(input_font);
  margin_spin_->setFont(input_font);

  // ---- preview ----
  root->addWidget(new QLabel(QStringLiteral("预览：")));
  QScrollArea *scroll = new QScrollArea;
  scroll->setWidgetResizable(true);
  preview_label_ = new QLabel;
  preview_label_->setAlignment(Qt::AlignCenter);
  preview_label_->setMinimumHeight(160);
  preview_label_->setStyleSheet(QStringLiteral("background-color: #333;"));
  scroll->setWidget(preview_label_);
  root->addWidget(scroll, 1);

  // ---- output dir ----
  QHBoxLayout *dir_row = new QHBoxLayout;
  dir_edit_ = new QLineEdit(QCoreApplication::applicationDirPath() +
                            QStringLiteral("/generate_res"));
  dir_edit_->setFixedHeight(40);
  dir_edit_->setFont(input_font);
  QPushButton *browse_btn = new QPushButton(QStringLiteral("浏览..."));
  dir_row->addWidget(new QLabel(QStringLiteral("输出目录：")));
  dir_row->addWidget(dir_edit_, 1);
  dir_row->addWidget(browse_btn);
  root->addLayout(dir_row);

  // ---- generate ----
  QPushButton *gen_btn = new QPushButton(QStringLiteral("生成"));
  gen_btn->setMinimumHeight(40);
  root->addWidget(gen_btn);

  status_label_ = new QLabel(QStringLiteral("就绪"));
  root->addWidget(status_label_);

  setCentralWidget(central);

  connect(browse_btn, &QPushButton::clicked, this, &MainWindow::OnBrowse);
  connect(gen_btn, &QPushButton::clicked, this, &MainWindow::OnGenerate);

  // live preview
  connect(input_edit_, &QTextEdit::textChanged, this, &MainWindow::OnUpdatePreview);
  connect(font_spin_, QOverload<int>::of(&QSpinBox::valueChanged), this,
          &MainWindow::OnUpdatePreview);
  connect(width_spin_, QOverload<int>::of(&QSpinBox::valueChanged), this,
          &MainWindow::OnUpdatePreview);
  connect(font_combo_, &QFontComboBox::currentFontChanged, this,
          &MainWindow::OnUpdatePreview);
  connect(pad_spin_, QOverload<int>::of(&QSpinBox::valueChanged), this,
          &MainWindow::OnUpdatePreview);

  OnUpdatePreview();
}

void MainWindow::OnBrowse() {
  QString dir =
      QFileDialog::getExistingDirectory(this, QStringLiteral("选择输出目录"),
                                        dir_edit_->text());
  if (!dir.isEmpty())
    dir_edit_->setText(dir);
}

void MainWindow::OnPickTextColor() {
  QColor color =
      QColorDialog::getColor(text_color_, this, QStringLiteral("选择文字颜色"));
  if (color.isValid()) {
    text_color_ = color;
    text_color_btn_->setStyleSheet(
        QStringLiteral("background-color: %1; color: %2;")
            .arg(color.name(), color.lightness() > 128 ? QStringLiteral("black")
                                                        : QStringLiteral("white")));
    OnUpdatePreview();
  }
}

void MainWindow::OnPickBgColor() {
  QColor color =
      QColorDialog::getColor(bg_color_, this, QStringLiteral("选择背景颜色"));
  if (color.isValid()) {
    bg_color_ = color;
    bg_color_btn_->setStyleSheet(
        QStringLiteral("background-color: %1; color: %2;")
            .arg(color.name(), color.lightness() > 128 ? QStringLiteral("black")
                                                        : QStringLiteral("white")));
    OnUpdatePreview();
  }
}

void MainWindow::OnSelectAll() {
  chk_png_->setChecked(true);
  chk_pdf_->setChecked(true);
  chk_docx_->setChecked(true);
}

void MainWindow::OnSelectNone() {
  chk_png_->setChecked(false);
  chk_pdf_->setChecked(false);
  chk_docx_->setChecked(false);
}

QImage MainWindow::RenderCard(const QString &text) {
  QFont font = font_combo_->currentFont();
  font.setPixelSize(font_spin_->value());
  font.setBold(false);

  QFontMetrics font_metrics(font);
  int text_width = font_metrics.horizontalAdvance(text);
  int text_height = font_metrics.ascent() + font_metrics.descent();
  int pad_x = pad_spin_->value();
  int pad_y = pad_spin_->value();

  int image_width = text_width + 2 * pad_x;
  int image_height = text_height + 2 * pad_y;

  // Scale to requested width if larger.
  int target_width = width_spin_->value();
  if (image_width > target_width) {
    double scale = double(target_width) / image_width;
    image_width = target_width;
    image_height = int(image_height * scale);
    // Adjust font size proportionally for rendering.
    font.setPixelSize(int(font.pixelSize() * scale));
    font_metrics = QFontMetrics(font);
  }

  QImage image(image_width, image_height, QImage::Format_RGB32);
  image.fill(bg_color_);

  QPainter painter(&image);
  painter.setFont(font);
  painter.setPen(text_color_);
  int draw_x = (image_width - font_metrics.horizontalAdvance(text)) / 2;
  int draw_y = (image_height + font_metrics.ascent() - font_metrics.descent()) / 2;
  painter.drawText(draw_x, draw_y, text);
  painter.end();
  return image;
}

void MainWindow::OnUpdatePreview() {
  QString text = input_edit_->toPlainText().trimmed();
  if (text.isEmpty()) {
    preview_label_->setText(QStringLiteral("（预览）"));
    preview_label_->setPixmap(QPixmap());
    return;
  }
  // Preview the first entry.
  QRegularExpression sep(QStringLiteral("[,，;；\\n\\r]+"));
  QStringList entries = text.split(sep, Qt::SkipEmptyParts);
  QString first = entries.isEmpty() ? text : entries.first().trimmed();

  QImage image = RenderCard(first);

  // Scale preview to fit.
  QPixmap pixmap = QPixmap::fromImage(image);
  QLabel *label = preview_label_;
  pixmap = pixmap.scaled(qMin(label->width(), 1200), qMin(label->height(), 300),
                         Qt::KeepAspectRatio, Qt::SmoothTransformation);
  label->setPixmap(pixmap);
}

QStringList MainWindow::SplitText(const QString &text, int max_chars) {
  QStringList parts;
  int i = 0;
  while (i < text.length()) {
    int length = qMin(max_chars, text.length() - i);
    parts.append(text.mid(i, length));
    i += length;
  }
  return parts;
}

QString MainWindow::SafeFileName(const QString &text) {
  QString result = text;
  static const QSet<QChar> kIllegalChars = QSet<QChar>()
      << QLatin1Char('<') << QLatin1Char('>') << QLatin1Char(':')
      << QLatin1Char('"') << QLatin1Char('/') << QLatin1Char('\\')
      << QLatin1Char('|') << QLatin1Char('?') << QLatin1Char('*');
  for (int i = 0; i < result.length(); ++i)
    if (kIllegalChars.contains(result.at(i)))
      result[i] = QLatin1Char('_');
  return result;
}

void MainWindow::OnGenerate() {
  QString raw_text = input_edit_->toPlainText().trimmed();
  if (raw_text.isEmpty()) {
    QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("请输入文字"));
    return;
  }
  QString output_dir = dir_edit_->text().trimmed();
  if (output_dir.isEmpty() || !QDir().mkpath(output_dir)) {
    QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("输出目录无效"));
    return;
  }
  if (!chk_png_->isChecked() && !chk_pdf_->isChecked() &&
      !chk_docx_->isChecked()) {
    QMessageBox::warning(this, QStringLiteral("提示"),
                         QStringLiteral("请至少选择一种输出格式"));
    return;
  }

  QRegularExpression sep(QStringLiteral("[,，;；\\n\\r]+"));
  QStringList entries = raw_text.split(sep, Qt::SkipEmptyParts);

  cards_.clear();
  card_names_.clear();
  QSet<QString> used_names;

  int card_index = 0;
  for (const QString &entry : entries) {
    QString text = entry.trimmed();
    if (text.isEmpty())
      continue;
    QImage image = RenderCard(text);
    cards_.append(image);

    QString base = SafeFileName(text);
    QString name = base;
    int n = 2;
    while (used_names.contains(name)) {
      name = QStringLiteral("%1_%2").arg(base).arg(n++);
    }
    used_names.insert(name);
    card_names_.append(name);

    if (chk_png_->isChecked()) {
      QString png_path =
          QDir(output_dir).absoluteFilePath(name + QStringLiteral(".png"));
      image.save(png_path, "PNG");
    }
    card_index++;
  }

  if (cards_.isEmpty()) {
    QMessageBox::warning(this, QStringLiteral("提示"),
                         QStringLiteral("没有可生成的内容"));
    return;
  }

  QString timestamp =
      QDateTime::currentDateTime().toString(QStringLiteral("yyMMddHHmmss"));
  QStringList done;
  if (chk_png_->isChecked())
    done << QStringLiteral("PNG %1 张").arg(card_index);
  if (chk_pdf_->isChecked()) {
    QString pdf_path = QDir(output_dir)
                           .absoluteFilePath(QStringLiteral("卡片_A4_%1.pdf")
                                                 .arg(timestamp));
    GeneratePdf(pdf_path);
    done << QStringLiteral("PDF");
  }
  if (chk_docx_->isChecked()) {
    QString docx_path = QDir(output_dir)
                            .absoluteFilePath(QStringLiteral("卡片_A4_%1.docx")
                                                  .arg(timestamp));
    GenerateDocx(docx_path);
    done << QStringLiteral("Word");
  }

  status_label_->setText(QStringLiteral("完成：") + done.join(QStringLiteral("、")));
  QMessageBox box(this);
  box.setWindowTitle(QStringLiteral("完成"));
  box.setText(QStringLiteral("已生成：\n%1\t").arg(done.join(QStringLiteral("\n"))));
  box.setIcon(QMessageBox::Information);
  box.setStandardButtons(QMessageBox::Ok);
  box.resize(600, 300);
  box.exec();
}

void MainWindow::GeneratePdf(const QString &path) {
  // A4 at 300 DPI.
  const int kA4Width = 2480;
  const int kA4Height = 3508;
  const int kPageMargin = 142;
  const int kCardGap = 118;
  const int kCardHeight = 472;
  const int kAvailableWidth = kA4Width - 2 * kPageMargin;
  const int kAvailableHeight = kA4Height - 2 * kPageMargin;

  QList<QImage> pages;
  QImage page(kA4Width, kA4Height, QImage::Format_RGB32);
  page.fill(Qt::white);
  int y = kPageMargin;

  for (int i = 0; i < cards_.size(); ++i) {
    const QImage &image = cards_.at(i);
    int image_width =
        int(double(kCardHeight) * image.width() / image.height());
    if (image_width > kAvailableWidth)
      image_width = kAvailableWidth;

    if (y + kCardHeight > kAvailableHeight) {
      pages.append(page);
      page = QImage(kA4Width, kA4Height, QImage::Format_RGB32);
      page.fill(Qt::white);
      y = kPageMargin;
    }
    int x = kPageMargin + (kAvailableWidth - image_width) / 2;
    QPainter painter(&page);
    painter.drawImage(x, y,
                      image.scaled(image_width, kCardHeight,
                                   Qt::KeepAspectRatio, Qt::SmoothTransformation));
    painter.end();
    y += kCardHeight + kCardGap;
  }
  pages.append(page);

  QPdfWriter writer(path);
  writer.setPageSize(QPageSize(QPageSize::A4));
  QPainter painter(&writer);
  for (int i = 0; i < pages.size(); ++i) {
    if (i > 0)
      writer.newPage();
    painter.drawImage(0, 0, pages.at(i));
  }
  painter.end();
}

void MainWindow::GenerateDocx(const QString &path) {
  QMap<QString, QByteArray> files;

  bool landscape = (orient_combo_->currentIndex() == 0);
  int page_width = landscape ? 16838 : 11906;
  int page_height = landscape ? 11906 : 16838;
  // Page margin: mm -> twips (1 inch = 1440 twips = 25.4 mm).
  const int margin_twips = int(margin_spin_->value() * 1440.0 / 25.4);

  const qint64 kCardHeightEmu = 40LL * 36000;
  const qint64 kAvailableWidthEmu = 180LL * 36000;

  QStringList paragraphs;
  QStringList relationships;
  for (int i = 0; i < cards_.size(); ++i) {
    const QImage &image = cards_.at(i);
    qint64 width_emu = kCardHeightEmu * image.width() / image.height();
    if (width_emu > kAvailableWidthEmu)
      width_emu = kAvailableWidthEmu;

    QString relationship_id = QStringLiteral("rId%1").arg(i + 1);
    QString image_name = QStringLiteral("image%1.png").arg(i + 1);

    relationships.append(QStringLiteral(
        "<Relationship Id=\"%1\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/image\" Target=\"media/%2\"/>")
        .arg(relationship_id, image_name));

    QString paragraph_xml = QStringLiteral(
        "<w:p>"
          "<w:pPr><w:jc w:val=\"center\"/>"
          "<w:spacing w:before=\"200\" w:after=\"200\"/>"
          "</w:pPr>"
          "<w:r><w:drawing>"
            "<wp:inline xmlns:wp=\"http://schemas.openxmlformats.org/drawingml/2006/wordprocessingDrawing\" distT=\"0\" distB=\"0\" distL=\"0\" distR=\"0\">"
              "<wp:extent cx=\"%1\" cy=\"%2\"/>"
              "<wp:effectExtent l=\"0\" t=\"0\" r=\"0\" b=\"0\"/>"
              "<wp:docPr id=\"%3\" name=\"%4\"/>"
              "<a:graphic xmlns:a=\"http://schemas.openxmlformats.org/drawingml/2006/main\">"
                "<a:graphicData uri=\"http://schemas.openxmlformats.org/drawingml/2006/picture\">"
                  "<pic:pic xmlns:pic=\"http://schemas.openxmlformats.org/drawingml/2006/picture\">"
                    "<pic:nvPicPr>"
                      "<pic:cNvPr id=\"%3\" name=\"%4\"/>"
                      "<pic:cNvPicPr/>"
                    "</pic:nvPicPr>"
                    "<pic:blipFill>"
                      "<a:blip r:embed=\"%5\"/>"
                      "<a:stretch><a:fillRect/></a:stretch>"
                    "</pic:blipFill>"
                    "<pic:spPr>"
                      "<a:xfrm><a:off x=\"0\" y=\"0\"/><a:ext cx=\"%1\" cy=\"%2\"/></a:xfrm>"
                      "<a:prstGeom prst=\"rect\"><a:avLst/></a:prstGeom>"
                    "</pic:spPr>"
                  "</pic:pic>"
                "</a:graphicData>"
              "</a:graphic>"
            "</wp:inline>"
          "</w:drawing></w:r>"
        "</w:p>")
        .arg(width_emu).arg(kCardHeightEmu)
        .arg(i + 1).arg(card_names_.at(i)).arg(relationship_id);
    paragraphs.append(paragraph_xml);
  }

  QString document_xml =
      "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
      "<w:document xmlns:w=\"http://schemas.openxmlformats.org/wordprocessingml/2006/main\""
      " xmlns:r=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships\">"
      "<w:body>" +
      paragraphs.join(QString()) +
      QStringLiteral(
          "<w:sectPr>"
            "<w:pgSz w:w=\"%1\" w:h=\"%2\" w:orient=\"%3\"/>"
            "<w:pgMar w:top=\"%4\" w:right=\"%4\" w:bottom=\"%4\" w:left=\"%4\" w:header=\"0\" w:footer=\"0\" w:gutter=\"0\"/>"
          "</w:sectPr>"
      "</w:body></w:document>")
      .arg(page_width).arg(page_height)
      .arg(landscape ? "landscape" : "portrait").arg(margin_twips);

  QString relationships_xml =
      "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
      "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">" +
      relationships.join(QString()) +
      "</Relationships>";

  QString content_types =
      "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
      "<Types xmlns=\"http://schemas.openxmlformats.org/package/2006/content-types\">"
        "<Default Extension=\"rels\" ContentType=\"application/vnd.openxmlformats-package.relationships+xml\"/>"
        "<Default Extension=\"xml\" ContentType=\"application/xml\"/>"
        "<Default Extension=\"png\" ContentType=\"image/png\"/>"
        "<Override PartName=\"/word/document.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml\"/>"
      "</Types>";

  QString root_relationships =
      "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
      "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">"
        "<Relationship Id=\"rId1\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument\" Target=\"word/document.xml\"/>"
      "</Relationships>";

  files.insert(QStringLiteral("[Content_Types].xml"), content_types.toUtf8());
  files.insert(QStringLiteral("_rels/.rels"), root_relationships.toUtf8());
  files.insert(QStringLiteral("word/_rels/document.xml.rels"),
               relationships_xml.toUtf8());
  files.insert(QStringLiteral("word/document.xml"), document_xml.toUtf8());

  for (int i = 0; i < cards_.size(); ++i) {
    QByteArray image_bytes;
    QBuffer buffer(&image_bytes);
    buffer.open(QIODevice::WriteOnly);
    cards_.at(i).save(&buffer, "PNG");
    files.insert(QStringLiteral("word/media/image%1.png").arg(i + 1),
                 image_bytes);
  }

  WriteZip(path, files);
}

quint32 MainWindow::Crc32Of(const QByteArray &data) {
  static quint32 crc_table[256];
  static bool table_initialized = false;
  if (!table_initialized) {
    for (quint32 i = 0; i < 256; ++i) {
      quint32 crc_value = i;
      for (int k = 0; k < 8; ++k)
        crc_value = (crc_value & 1) ? 0xEDB88320u ^ (crc_value >> 1)
                                    : crc_value >> 1;
      crc_table[i] = crc_value;
    }
    table_initialized = true;
  }
  quint32 crc = 0xFFFFFFFFu;
  for (int i = 0; i < data.size(); ++i)
    crc = crc_table[(crc ^ (uchar(data.at(i)))) & 0xFF] ^ (crc >> 8);
  return crc ^ 0xFFFFFFFFu;
}

QByteArray MainWindow::MakeLocalHeader(const QString &name, quint32 crc,
                                       quint32 size) {
  QByteArray file_name = name.toUtf8();
  QByteArray header;
  QDataStream stream(&header, QIODevice::WriteOnly);
  stream.setByteOrder(QDataStream::LittleEndian);
  stream << quint32(0x04034b50);
  stream << quint16(20);
  stream << quint16(0);
  stream << quint16(0);
  stream << quint16(0);
  stream << quint16(0x21);
  stream << quint32(crc);
  stream << quint32(size);
  stream << quint32(size);
  stream << quint16(file_name.size());
  stream << quint16(0);
  header.append(file_name);
  return header;
}

QByteArray MainWindow::MakeCentralHeader(const QString &name, quint32 crc,
                                         quint32 size, quint32 local_offset) {
  QByteArray file_name = name.toUtf8();
  QByteArray header;
  QDataStream stream(&header, QIODevice::WriteOnly);
  stream.setByteOrder(QDataStream::LittleEndian);
  stream << quint32(0x02014b50);
  stream << quint16(20);
  stream << quint16(20);
  stream << quint16(0);
  stream << quint16(0);
  stream << quint16(0);
  stream << quint16(0x21);
  stream << quint32(crc);
  stream << quint32(size);
  stream << quint32(size);
  stream << quint16(file_name.size());
  stream << quint16(0);
  stream << quint16(0);
  stream << quint16(0);
  stream << quint16(0);
  stream << quint32(0);
  stream << quint32(local_offset);
  header.append(file_name);
  return header;
}

QByteArray MainWindow::MakeEocd(quint32 entry_count,
                                quint32 central_directory_size,
                                quint32 central_directory_offset) {
  QByteArray header;
  QDataStream stream(&header, QIODevice::WriteOnly);
  stream.setByteOrder(QDataStream::LittleEndian);
  stream << quint32(0x06054b50);
  stream << quint16(0);
  stream << quint16(0);
  stream << quint16(entry_count);
  stream << quint16(entry_count);
  stream << quint32(central_directory_size);
  stream << quint32(central_directory_offset);
  stream << quint16(0);
  return header;
}

bool MainWindow::WriteZip(const QString &path,
                          const QMap<QString, QByteArray> &files) {
  QFile file(path);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
    return false;

  QByteArray central_directory;
  quint32 offset = 0;

  for (auto it = files.begin(); it != files.end(); ++it) {
    const QString name = it.key();
    const QByteArray &data = it.value();
    quint32 crc = Crc32Of(data);

    QByteArray local_header = MakeLocalHeader(name, crc, data.size());
    file.write(local_header);
    file.write(data);

    central_directory.append(MakeCentralHeader(name, crc, data.size(), offset));
    offset += local_header.size() + data.size();
  }

  quint32 central_directory_start = offset;
  quint32 central_directory_size = central_directory.size();
  file.write(central_directory);
  file.write(MakeEocd(files.size(), central_directory_size,
                      central_directory_start));
  file.close();
  return true;
}

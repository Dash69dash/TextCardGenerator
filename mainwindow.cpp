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

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle(QStringLiteral("文字卡片生成器"));
    resize(900, 720);

    m_textColor = Qt::white;
    m_bgColor   = Qt::black;

    QWidget *central = new QWidget(this);
    QVBoxLayout *root = new QVBoxLayout(central);

    // ---- input ----
    root->addWidget(new QLabel(QStringLiteral("输入文字（多个词条用逗号或换行分隔）：")));
    m_inputEdit = new QTextEdit;
    m_inputEdit->setPlaceholderText(QStringLiteral("例如：\n角的初步认识，角的特征\n边，顶点\n猜想，验证，结论"));
    m_inputEdit->setMaximumHeight(120);
    root->addWidget(m_inputEdit);

    // ---- settings grid ----
    QGridLayout *grid = new QGridLayout;

    int row = 0;
    grid->addWidget(new QLabel(QStringLiteral("字体：")), row, 0);
    m_fontCombo = new QFontComboBox;
    m_fontCombo->setCurrentFont(QFont(QStringLiteral("SimSun")));
    grid->addWidget(m_fontCombo, row, 1);

    row++;
    grid->addWidget(new QLabel(QStringLiteral("字体像素大小：")), row, 0);
    m_fontSpin = new QSpinBox;
    m_fontSpin->setRange(50, 1200);
    m_fontSpin->setValue(600);
    m_fontSpin->setSuffix(QStringLiteral(" px"));
    grid->addWidget(m_fontSpin, row, 1);

    row++;
    grid->addWidget(new QLabel(QStringLiteral("图片宽度：")), row, 0);
    m_widthSpin = new QSpinBox;
    m_widthSpin->setRange(400, 4000);
    m_widthSpin->setValue(2580);
    m_widthSpin->setSuffix(QStringLiteral(" px"));
    grid->addWidget(m_widthSpin, row, 1);

    row++;
    grid->addWidget(new QLabel(QStringLiteral("文字颜色：")), row, 0);
    m_textColorBtn = new QPushButton;
    m_textColorBtn->setFixedWidth(120);
    m_textColorBtn->setStyleSheet(QStringLiteral("background-color: white; color: black;"));
    connect(m_textColorBtn, &QPushButton::clicked, this, &MainWindow::onPickTextColor);
    grid->addWidget(m_textColorBtn, row, 1);

    row++;
    grid->addWidget(new QLabel(QStringLiteral("背景颜色：")), row, 0);
    m_bgColorBtn = new QPushButton;
    m_bgColorBtn->setFixedWidth(120);
    m_bgColorBtn->setStyleSheet(QStringLiteral("background-color: black; color: white;"));
    connect(m_bgColorBtn, &QPushButton::clicked, this, &MainWindow::onPickBgColor);
    grid->addWidget(m_bgColorBtn, row, 1);

    row++;
    grid->addWidget(new QLabel(QStringLiteral("输出格式：")), row, 0);
    QHBoxLayout *chkRow = new QHBoxLayout;
    m_chkPng  = new QCheckBox(QStringLiteral("图片 PNG"));
    m_chkPdf  = new QCheckBox(QStringLiteral("PDF"));
    m_chkDocx = new QCheckBox(QStringLiteral("Word"));
    m_chkPng->setChecked(true);
    m_chkPdf->setChecked(false);
    m_chkDocx->setChecked(true);
    QPushButton *selAllBtn = new QPushButton(QStringLiteral("全选"));
    QPushButton *selNoneBtn = new QPushButton(QStringLiteral("全不选"));
    chkRow->addWidget(m_chkPng);
    chkRow->addWidget(m_chkPdf);
    chkRow->addWidget(m_chkDocx);
    chkRow->addStretch();
    chkRow->addWidget(selAllBtn);
    chkRow->addWidget(selNoneBtn);
    grid->addLayout(chkRow, row, 1);
    connect(selAllBtn, &QPushButton::clicked, this, &MainWindow::onSelectAll);
    connect(selNoneBtn, &QPushButton::clicked, this, &MainWindow::onSelectNone);
    // pdf not success, disabled it temp.
    m_chkPdf->setEnabled(false);

    row++;
    grid->addWidget(new QLabel(QStringLiteral("Word方向：")), row, 0);
    m_orientCombo = new QComboBox;
    m_orientCombo->addItem(QStringLiteral("横版"));
    m_orientCombo->addItem(QStringLiteral("竖版"));
    m_orientCombo->setCurrentIndex(0);
    grid->addWidget(m_orientCombo, row, 1);

    root->addLayout(grid);

    // ---- preview ----
    root->addWidget(new QLabel(QStringLiteral("预览：")));
    QScrollArea *scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    m_previewLabel = new QLabel;
    m_previewLabel->setAlignment(Qt::AlignCenter);
    m_previewLabel->setMinimumHeight(160);
    m_previewLabel->setStyleSheet(QStringLiteral("background-color: #333;"));
    scroll->setWidget(m_previewLabel);
    root->addWidget(scroll, 1);

    // ---- output dir ----
    QHBoxLayout *dirRow = new QHBoxLayout;
    m_dirEdit = new QLineEdit(QCoreApplication::applicationDirPath() + QStringLiteral("/generate_res"));
    QPushButton *browseBtn = new QPushButton(QStringLiteral("浏览..."));
    dirRow->addWidget(new QLabel(QStringLiteral("输出目录：")));
    dirRow->addWidget(m_dirEdit, 1);
    dirRow->addWidget(browseBtn);
    root->addLayout(dirRow);

    // ---- generate ----
    QPushButton *genBtn = new QPushButton(QStringLiteral("生成"));
    genBtn->setMinimumHeight(40);
    root->addWidget(genBtn);

    m_statusLabel = new QLabel(QStringLiteral("就绪"));
    root->addWidget(m_statusLabel);

    setCentralWidget(central);

    connect(browseBtn, &QPushButton::clicked, this, &MainWindow::onBrowse);
    connect(genBtn, &QPushButton::clicked, this, &MainWindow::onGenerate);

    // live preview
    connect(m_inputEdit, &QTextEdit::textChanged, this, &MainWindow::onUpdatePreview);
    connect(m_fontSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::onUpdatePreview);
    connect(m_widthSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::onUpdatePreview);
    connect(m_fontCombo, &QFontComboBox::currentFontChanged, this, &MainWindow::onUpdatePreview);

    onUpdatePreview();
}

void MainWindow::onBrowse()
{
    QString dir = QFileDialog::getExistingDirectory(this, QStringLiteral("选择输出目录"), m_dirEdit->text());
    if (!dir.isEmpty())
        m_dirEdit->setText(dir);
}

void MainWindow::onPickTextColor()
{
    QColor c = QColorDialog::getColor(m_textColor, this, QStringLiteral("选择文字颜色"));
    if (c.isValid()) {
        m_textColor = c;
        m_textColorBtn->setStyleSheet(QStringLiteral("background-color: %1; color: %2;")
            .arg(c.name(), c.lightness() > 128 ? QStringLiteral("black") : QStringLiteral("white")));
        onUpdatePreview();
    }
}

void MainWindow::onPickBgColor()
{
    QColor c = QColorDialog::getColor(m_bgColor, this, QStringLiteral("选择背景颜色"));
    if (c.isValid()) {
        m_bgColor = c;
        m_bgColorBtn->setStyleSheet(QStringLiteral("background-color: %1; color: %2;")
            .arg(c.name(), c.lightness() > 128 ? QStringLiteral("black") : QStringLiteral("white")));
        onUpdatePreview();
    }
}

void MainWindow::onSelectAll()
{
    m_chkPng->setChecked(true);
    m_chkPdf->setChecked(true);
    m_chkDocx->setChecked(true);
}

void MainWindow::onSelectNone()
{
    m_chkPng->setChecked(false);
    m_chkPdf->setChecked(false);
    m_chkDocx->setChecked(false);
}

QImage MainWindow::renderCard(const QString &text)
{
    QFont font = m_fontCombo->currentFont();
    font.setPixelSize(m_fontSpin->value());
    font.setBold(false);

    QFontMetrics fm(font);
    int textW = fm.horizontalAdvance(text);
    int textH = fm.ascent() + fm.descent();
    int padX = int(textW * 0.08) + 10;
    int padY = int(textH * 0.15) + 10;

    int imgW = textW + 2 * padX;
    int imgH = textH + 2 * padY;

    // scale to requested width if larger
    int targetW = m_widthSpin->value();
    if (imgW > targetW) {
        double scale = double(targetW) / imgW;
        imgW = targetW;
        imgH = int(imgH * scale);
        // adjust font size proportionally for rendering
        font.setPixelSize(int(font.pixelSize() * scale));
        fm = QFontMetrics(font);
    }

    QImage img(imgW, imgH, QImage::Format_RGB32);
    img.fill(m_bgColor);

    QPainter painter(&img);
    painter.setFont(font);
    painter.setPen(m_textColor);
    int drawX = (imgW - fm.horizontalAdvance(text)) / 2;
    int drawY = (imgH + fm.ascent() - fm.descent()) / 2;
    painter.drawText(drawX, drawY, text);
    painter.end();
    return img;
}

void MainWindow::onUpdatePreview()
{
    QString text = m_inputEdit->toPlainText().trimmed();
    if (text.isEmpty()) {
        m_previewLabel->setText(QStringLiteral("（预览）"));
        m_previewLabel->setPixmap(QPixmap());
        return;
    }
    // preview first entry
    QRegularExpression sep(QStringLiteral("[,，;；\\n\\r]+"));
    QStringList entries = text.split(sep, Qt::SkipEmptyParts);
    QString first = entries.isEmpty() ? text : entries.first().trimmed();

    QImage img = renderCard(first);

    // scale preview to fit
    QPixmap pm = QPixmap::fromImage(img);
    QLabel *lbl = m_previewLabel;
    pm = pm.scaled(qMin(lbl->width(), 1200), qMin(lbl->height(), 300),
                   Qt::KeepAspectRatio, Qt::SmoothTransformation);
    lbl->setPixmap(pm);
}

QStringList MainWindow::splitText(const QString &text, int maxChars)
{
    QStringList parts;
    int i = 0;
    while (i < text.length()) {
        int len = qMin(maxChars, text.length() - i);
        parts.append(text.mid(i, len));
        i += len;
    }
    return parts;
}

QString MainWindow::safeFileName(const QString &text)
{
    QString s = text;
    static const QSet<QChar> bad = QSet<QChar>()
        << QLatin1Char('<') << QLatin1Char('>') << QLatin1Char(':')
        << QLatin1Char('"') << QLatin1Char('/') << QLatin1Char('\\')
        << QLatin1Char('|') << QLatin1Char('?') << QLatin1Char('*');
    for (int i = 0; i < s.length(); ++i)
        if (bad.contains(s.at(i))) s[i] = QLatin1Char('_');
    return s;
}

void MainWindow::onGenerate()
{
    QString raw = m_inputEdit->toPlainText().trimmed();
    if (raw.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("请输入文字"));
        return;
    }
    QString outDir = m_dirEdit->text().trimmed();
    if (outDir.isEmpty() || !QDir().mkpath(outDir)) {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("输出目录无效"));
        return;
    }
    if (!m_chkPng->isChecked() && !m_chkPdf->isChecked() && !m_chkDocx->isChecked()) {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("请至少选择一种输出格式"));
        return;
    }

    QRegularExpression sep(QStringLiteral("[,，;；\\n\\r]+"));
    QStringList entries = raw.split(sep, Qt::SkipEmptyParts);

    m_cards.clear();
    m_cardNames.clear();
    QSet<QString> usedNames;

    int cardIndex = 0;
    for (const QString &entry : entries) {
        QString e = entry.trimmed();
        if (e.isEmpty()) continue;
        QImage img = renderCard(e);
        m_cards.append(img);

        QString base = safeFileName(e);
        QString name = base;
        int n = 2;
        while (usedNames.contains(name)) {
            name = QStringLiteral("%1_%2").arg(base).arg(n++);
        }
        usedNames.insert(name);
        m_cardNames.append(name);

        if (m_chkPng->isChecked()) {
            QString pngPath = QDir(outDir).absoluteFilePath(name + QStringLiteral(".png"));
            img.save(pngPath, "PNG");
        }
        cardIndex++;
    }

    if (m_cards.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("没有可生成的内容"));
        return;
    }

    QString ts = QDateTime::currentDateTime().toString(QStringLiteral("yyMMddHHmmss"));
    QStringList done;
    if (m_chkPng->isChecked())
        done << QStringLiteral("PNG %1 张").arg(cardIndex);
    if (m_chkPdf->isChecked()) {
        QString pdfPath = QDir(outDir).absoluteFilePath(QStringLiteral("卡片_A4_%1.pdf").arg(ts));
        generatePdf(pdfPath);
        done << QStringLiteral("PDF");
    }
    if (m_chkDocx->isChecked()) {
        QString docxPath = QDir(outDir).absoluteFilePath(QStringLiteral("卡片_A4_%1.docx").arg(ts));
        generateDocx(docxPath);
        done << QStringLiteral("Word");
    }

    m_statusLabel->setText(QStringLiteral("完成：") + done.join(QStringLiteral("、")));
    QMessageBox box(this);
    box.setWindowTitle(QStringLiteral("完成"));
    box.setText(QStringLiteral("已生成：\n%1\t").arg(done.join(QStringLiteral("\n"))));
    box.setIcon(QMessageBox::Information);
    box.setStandardButtons(QMessageBox::Ok);
    box.resize(600, 300);
    box.exec();
}

void MainWindow::generatePdf(const QString &path)
{
    // A4 at 300 DPI
    const int A4_W = 2480;
    const int A4_H = 3508;
    const int margin = 142;
    const int gap    = 118;
    const int cardH  = 472;
    const int availW = A4_W - 2 * margin;
    const int availH = A4_H - 2 * margin;

    QList<QImage> pages;
    QImage page(A4_W, A4_H, QImage::Format_RGB32);
    page.fill(Qt::white);
    int y = margin;

    for (int i = 0; i < m_cards.size(); ++i) {
        const QImage &img = m_cards.at(i);
        int imgW = int(double(cardH) * img.width() / img.height());
        if (imgW > availW) imgW = availW;

        if (y + cardH > availH) {
            pages.append(page);
            page = QImage(A4_W, A4_H, QImage::Format_RGB32);
            page.fill(Qt::white);
            y = margin;
        }
        int x = margin + (availW - imgW) / 2;
        QPainter p(&page);
        p.drawImage(x, y, img.scaled(imgW, cardH, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        p.end();
        y += cardH + gap;
    }
    pages.append(page);

    QPdfWriter writer(path);
    writer.setPageSize(QPageSize(QPageSize::A4));
    QPainter painter(&writer);
    for (int i = 0; i < pages.size(); ++i) {
        if (i > 0) writer.newPage();
        painter.drawImage(0, 0, pages.at(i));
    }
    painter.end();
}

void MainWindow::generateDocx(const QString &path)
{
    QMap<QString, QByteArray> files;

    bool landscape = (m_orientCombo->currentIndex() == 0);
    int pageW = landscape ? 16838 : 11906;
    int pageH = landscape ? 11906 : 16838;
    const int margin = 850;

    const qint64 cardHEmu = 40LL * 36000;
    const qint64 availWEmu = 180LL * 36000;

    QStringList paragraphs;
    QStringList rels;
    for (int i = 0; i < m_cards.size(); ++i) {
        const QImage &img = m_cards.at(i);
        qint64 wEmu = cardHEmu * img.width() / img.height();
        if (wEmu > availWEmu) wEmu = availWEmu;

        QString rid = QStringLiteral("rId%1").arg(i + 1);
        QString imgName = QStringLiteral("image%1.png").arg(i + 1);

        rels.append(QStringLiteral(
            "<Relationship Id=\"%1\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/image\" Target=\"media/%2\"/>")
            .arg(rid, imgName));

        QString p = QStringLiteral(
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
            .arg(wEmu).arg(cardHEmu)
            .arg(i + 1).arg(m_cardNames.at(i)).arg(rid);
        paragraphs.append(p);
    }

    QString documentXml =
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
        "<w:document xmlns:w=\"http://schemas.openxmlformats.org/wordprocessingml/2006/main\""
        " xmlns:r=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships\">"
        "<w:body>" +
            paragraphs.join(QString()) +
            QStringLiteral(
            "<w:sectPr>"
              "<w:pgSz w:w=\"%1\" w:h=\"%2\" w:orient=\"%3\"/>"
              "<w:pgMar w:top=\"%3\" w:right=\"%3\" w:bottom=\"%3\" w:left=\"%3\" w:header=\"0\" w:footer=\"0\" w:gutter=\"0\"/>"
            "</w:sectPr>"
        "</w:body></w:document>")
        .arg(pageW).arg(pageH).arg(landscape ? "landscape" : "portrait").arg(margin);

    QString relsXml =
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
        "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">" +
            rels.join(QString()) +
        "</Relationships>";

    QString contentTypes =
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
        "<Types xmlns=\"http://schemas.openxmlformats.org/package/2006/content-types\">"
          "<Default Extension=\"rels\" ContentType=\"application/vnd.openxmlformats-package.relationships+xml\"/>"
          "<Default Extension=\"xml\" ContentType=\"application/xml\"/>"
          "<Default Extension=\"png\" ContentType=\"image/png\"/>"
          "<Override PartName=\"/word/document.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml\"/>"
        "</Types>";

    QString rootRels =
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
        "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">"
          "<Relationship Id=\"rId1\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument\" Target=\"word/document.xml\"/>"
        "</Relationships>";

    files.insert(QStringLiteral("[Content_Types].xml"), contentTypes.toUtf8());
    files.insert(QStringLiteral("_rels/.rels"), rootRels.toUtf8());
    files.insert(QStringLiteral("word/_rels/document.xml.rels"), relsXml.toUtf8());
    files.insert(QStringLiteral("word/document.xml"), documentXml.toUtf8());

    for (int i = 0; i < m_cards.size(); ++i) {
        QByteArray png;
        QBuffer buf(&png);
        buf.open(QIODevice::WriteOnly);
        m_cards.at(i).save(&buf, "PNG");
        files.insert(QStringLiteral("word/media/image%1.png").arg(i + 1), png);
    }

    writeZip(path, files);
}

quint32 MainWindow::crc32Of(const QByteArray &data)
{
    static quint32 table[256];
    static bool init = false;
    if (!init) {
        for (quint32 i = 0; i < 256; ++i) {
            quint32 c = i;
            for (int k = 0; k < 8; ++k)
                c = (c & 1) ? 0xEDB88320u ^ (c >> 1) : c >> 1;
            table[i] = c;
        }
        init = true;
    }
    quint32 crc = 0xFFFFFFFFu;
    for (int i = 0; i < data.size(); ++i)
        crc = table[(crc ^ (uchar(data.at(i)))) & 0xFF] ^ (crc >> 8);
    return crc ^ 0xFFFFFFFFu;
}

QByteArray MainWindow::makeLocalHeader(const QString &name, quint32 crc, quint32 size)
{
    QByteArray fn = name.toUtf8();
    QByteArray h;
    QDataStream s(&h, QIODevice::WriteOnly);
    s.setByteOrder(QDataStream::LittleEndian);
    s << quint32(0x04034b50);
    s << quint16(20);
    s << quint16(0);
    s << quint16(0);
    s << quint16(0);
    s << quint16(0x21);
    s << quint32(crc);
    s << quint32(size);
    s << quint32(size);
    s << quint16(fn.size());
    s << quint16(0);
    h.append(fn);
    return h;
}

QByteArray MainWindow::makeCentralHeader(const QString &name, quint32 crc, quint32 size, quint32 localOffset)
{
    QByteArray fn = name.toUtf8();
    QByteArray h;
    QDataStream s(&h, QIODevice::WriteOnly);
    s.setByteOrder(QDataStream::LittleEndian);
    s << quint32(0x02014b50);
    s << quint16(20);
    s << quint16(20);
    s << quint16(0);
    s << quint16(0);
    s << quint16(0);
    s << quint16(0x21);
    s << quint32(crc);
    s << quint32(size);
    s << quint32(size);
    s << quint16(fn.size());
    s << quint16(0);
    s << quint16(0);
    s << quint16(0);
    s << quint16(0);
    s << quint32(0);
    s << quint32(localOffset);
    h.append(fn);
    return h;
}

QByteArray MainWindow::makeEocd(quint32 entryCount, quint32 cdSize, quint32 cdOffset)
{
    QByteArray h;
    QDataStream s(&h, QIODevice::WriteOnly);
    s.setByteOrder(QDataStream::LittleEndian);
    s << quint32(0x06054b50);
    s << quint16(0);
    s << quint16(0);
    s << quint16(entryCount);
    s << quint16(entryCount);
    s << quint32(cdSize);
    s << quint32(cdOffset);
    s << quint16(0);
    return h;
}

bool MainWindow::writeZip(const QString &path, const QMap<QString, QByteArray> &files)
{
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return false;

    QByteArray centralDir;
    quint32 offset = 0;

    for (auto it = files.begin(); it != files.end(); ++it) {
        const QString name = it.key();
        const QByteArray &data = it.value();
        quint32 crc = crc32Of(data);

        QByteArray lh = makeLocalHeader(name, crc, data.size());
        f.write(lh);
        f.write(data);

        centralDir.append(makeCentralHeader(name, crc, data.size(), offset));
        offset += lh.size() + data.size();
    }

    quint32 cdStart = offset;
    quint32 cdSize = centralDir.size();
    f.write(centralDir);
    f.write(makeEocd(files.size(), cdSize, cdStart));
    f.close();
    return true;
}

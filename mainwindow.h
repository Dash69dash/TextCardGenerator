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

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void onBrowse();
    void onGenerate();
    void onPickTextColor();
    void onPickBgColor();
    void onUpdatePreview();
    void onSelectAll();
    void onSelectNone();

private:
    QTextEdit     *m_inputEdit;
    QLineEdit     *m_dirEdit;
    QFontComboBox *m_fontCombo;
    QSpinBox      *m_fontSpin;       // font pixel size
    QSpinBox      *m_widthSpin;      // image width in px
    QPushButton   *m_textColorBtn;
    QPushButton   *m_bgColorBtn;
    QCheckBox     *m_chkPng;
    QCheckBox     *m_chkPdf;
    QCheckBox     *m_chkDocx;
    QComboBox    *m_orientCombo;
    QLabel        *m_previewLabel;
    QLabel        *m_statusLabel;

    QColor m_textColor;
    QColor m_bgColor;

    QList<QImage> m_cards;
    QStringList   m_cardNames;

    QImage renderCard(const QString &text);
    QStringList splitText(const QString &text, int maxChars);
    QString safeFileName(const QString &text);

    void generatePdf(const QString &path);
    void generateDocx(const QString &path);

    bool writeZip(const QString &path, const QMap<QString, QByteArray> &files);
    QByteArray makeLocalHeader(const QString &name, quint32 crc, quint32 size);
    QByteArray makeCentralHeader(const QString &name, quint32 crc, quint32 size, quint32 localOffset);
    QByteArray makeEocd(quint32 entryCount, quint32 cdSize, quint32 cdOffset);
    quint32 crc32Of(const QByteArray &data);
};

#endif // MAINWINDOW_H

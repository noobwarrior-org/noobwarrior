// === noobWarrior ===
// File: LoadingDialog.h
// Started by: Hattozo
// Started on: 8/10/2025
// Description: Mimics the Roblox startup dialog.
#include "LoadingDialog.h"

#include <QPixmap>

using namespace NoobWarrior;

LoadingDialog::LoadingDialog(QWidget *parent) : QDialog(parent),
    mImgLabel(nullptr),
    mTextLabel(nullptr),
    mProgressBar(nullptr)
{
    setWindowTitle("Loading...");
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    setFixedSize(QSize(518, 318));
    setWindowFlags(windowFlags() | Qt::FramelessWindowHint);
    setStyleSheet("QDialog { border: 1px solid rgb(37, 37, 37); }");

    InitWidgets();
}

void LoadingDialog::InitWidgets() {
    mImgLabel = new QLabel(this);
    mTextLabel = new QLabel(this);
    mProgressBar = new QProgressBar(this);
    mCancelButton = new QPushButton("Cancel", this);

    mImgLabel->move(212, 66);
    mTextLabel->move(29, 199);
    mProgressBar->move(29, 241);
    mCancelButton->move(194, 269);

    mImgLabel->resize(92, 92);
    mTextLabel->resize(460, 25);
    mProgressBar->resize(460, 20);
    mCancelButton->resize(130, 34);

    QImage image(":/shared/loading_icon.png");
    QPixmap iconPix = QPixmap::fromImage(image).scaled(92 * static_cast<int>(devicePixelRatio()), 92 * static_cast<int>(devicePixelRatio()), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    iconPix.setDevicePixelRatio(devicePixelRatio());

    mImgLabel->setPixmap(iconPix);

    mTextLabel->setText("Loading...");
    mTextLabel->setAlignment(Qt::AlignCenter);
    QFont font = mTextLabel->font();
    font.setPointSize(12);
    mTextLabel->setFont(font);

    mProgressBar->setTextVisible(false);
    mProgressBar->setMaximum(0);
    mProgressBar->setValue(0);

    QFont cancelFont = mCancelButton->font();
    cancelFont.setPointSize(12);
    mCancelButton->setFont(cancelFont);

    connect(mCancelButton, &QPushButton::clicked, [this]() {
        close();
    });
}

void LoadingDialog::SetText(const QString &str) {
    mTextLabel->setText(str);
}

void LoadingDialog::SetProgress(double progress) {
    if (progress < 0) {
        mProgressBar->setMaximum(0);
        mProgressBar->setValue(0);
        return;
    }
    mProgressBar->setMaximum(100);
    mProgressBar->setValue(progress * 100);
}
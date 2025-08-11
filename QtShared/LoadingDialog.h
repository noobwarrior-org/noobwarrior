// === noobWarrior ===
// File: LoadingDialog.h
// Started by: Hattozo
// Started on: 8/10/2025
// Description:
#pragma once
#include <QDialog>
#include <QWidget>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>

namespace NoobWarrior {
class LoadingDialog : public QDialog {
    Q_OBJECT
public:
    LoadingDialog(QWidget *parent = nullptr);
    void InitWidgets();

    void SetText(const QString &str);
    void SetProgress(double progress);
private:
    QLabel *mImgLabel;
    QLabel *mTextLabel;
    QProgressBar *mProgressBar;
    QPushButton *mCancelButton;
};
}

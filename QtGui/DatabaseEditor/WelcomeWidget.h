// === noobWarrior ===
// File: WelcomeWidget.h
// Started by: Hattozo
// Started on: 11/27/2025
// Description: Starting page for Database Editor
#pragma once
#include <QWidget>
#include <QGridLayout>
#include <QLabel>

namespace NoobWarrior {
class WelcomeWidget : public QWidget {
    Q_OBJECT
public:
    WelcomeWidget(QWidget *parent = nullptr);
    void InitWidgets();
private:
    QGridLayout* mLayout;
    QLabel* mHeader;

    QFrame* mStartFrame;
    QVBoxLayout* mStartLayout;
    QLabel* mStartLabel;

    QFrame* mRecentFrame;
    QVBoxLayout* mRecentLayout;
    QLabel* mRecentLabel;

    QFrame* mDatabasesFrame;
    QVBoxLayout* mDatabasesLayout;
    QLabel* mDatabasesLabel;
};
}
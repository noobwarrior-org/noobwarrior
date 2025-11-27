// === noobWarrior ===
// File: WelcomeWidget.cpp
// Started by: Hattozo
// Started on: 11/27/2025
// Description: Starting page for Database Editor
#include "WelcomeWidget.h"

#include <QApplication>
#include <qboxlayout.h>

using namespace NoobWarrior;

WelcomeWidget::WelcomeWidget(QWidget *parent) : QWidget(parent),
    mLayout(nullptr),
    mHeader(nullptr),
    mStartLabel(nullptr)
{
    InitWidgets();
}

void WelcomeWidget::InitWidgets() {
    mLayout = new QGridLayout(this);

    mLayout->setContentsMargins(width() * 0.4, height() * 0.4, width() * 0.4, height() * 0.4);

    mHeader = new QLabel("Welcome");
    mHeader->setFont(QFont(QApplication::font().family(), 20));
    mLayout->addWidget(mHeader, 0, 0);

    mStartFrame = new QFrame();
    mStartLayout = new QVBoxLayout(mStartFrame);
    mStartLayout->setContentsMargins(0, 0, 0, 0);
    mStartLabel = new QLabel("Start");
    mStartLabel->setFont(QFont(QApplication::font().family(), 14));
    mStartLayout->addWidget(mStartLabel);

    mRecentFrame = new QFrame();
    mRecentLayout = new QVBoxLayout(mRecentFrame);
    mRecentLayout->setContentsMargins(0, 0, 0, 0);
    mRecentLabel = new QLabel("Recent");
    mRecentLabel->setFont(QFont(QApplication::font().family(), 14));
    mStartLayout->addWidget(mRecentLabel);

    mDatabasesFrame = new QFrame();
    mDatabasesLayout = new QVBoxLayout(mDatabasesFrame);
    mDatabasesLayout->setContentsMargins(0, 0, 0, 0);
    mDatabasesLabel = new QLabel("Installed Databases");
    mDatabasesLabel->setFont(QFont(QApplication::font().family(), 14));
    mDatabasesLabel->setAlignment(Qt::AlignRight);
    mDatabasesLayout->addWidget(mDatabasesLabel);

    mLayout->addWidget(mStartFrame, 1, 0);
    mLayout->addWidget(mRecentFrame, 2, 0);
    mLayout->addWidget(mDatabasesFrame, 1, 1);
}
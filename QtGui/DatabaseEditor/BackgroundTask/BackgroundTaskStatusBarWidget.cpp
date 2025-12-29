// === noobWarrior ===
// File: BackgroundTaskStatusBarWidget.cpp
// Started by: Hattozo
// Started on: 12/28/2025
// Description: The little progress bar that you see in your status bar
#include "BackgroundTaskStatusBarWidget.h"
#include "BackgroundTaskPopupWidget.h"

#include <QApplication>
#include <QStyle>
#include <QMouseEvent>

using namespace NoobWarrior;

BackgroundTaskStatusBarWidget::BackgroundTaskStatusBarWidget(QWidget *parent) : QWidget(parent),
    mLayout(this),
    mPopupWidget(nullptr)
{
    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    mLayout.setSizeConstraint(QLayout::SetMaximumSize);
    
    mLayout.addWidget(&mLabel);
    mLayout.addWidget(&mProgressBar);

    auto bellLabel = new QLabel();
    QIcon bellIcon = QIcon(":/images/silk/bell.png");
    bellLabel->setPixmap(bellIcon.pixmap(QSize(16, 16)));
    mLayout.addWidget(bellLabel);

    mLabel.setVisible(false);
    mProgressBar.setVisible(false);
}

void BackgroundTaskStatusBarWidget::SetPopupWidget(BackgroundTaskPopupWidget *widget) {
    mPopupWidget = widget;
}

void BackgroundTaskStatusBarWidget::mouseReleaseEvent(QMouseEvent *event) {
    if (mPopupWidget != nullptr)
        mPopupWidget->isVisible() ? mPopupWidget->hide() : mPopupWidget->show();
}

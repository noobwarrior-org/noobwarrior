/*
 * Copyright (C) 2026 Hattozo
 *
 * This file is part of noobWarrior.
 *
 * noobWarrior is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * noobWarrior is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with noobWarrior; if not, see
 * <https://www.gnu.org/licenses/>.
 */
// === noobWarrior ===
// File: Notification.cpp
// Started by: Hattozo
// Started on: 6/17/2026
// Description: Corner toast notifications and the manager that stacks them.
#include "Notification.h"
#include "Application.h"

#include <QLabel>
#include <QToolButton>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTimer>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>
#include <QEnterEvent>
#include <QEvent>
#include <QIcon>

using namespace NoobWarrior;

static constexpr int kAutoDismissMs = 12000;
static constexpr int kFadeMs = 200;

NotificationWidget::NotificationWidget(const QString& title, const QString& message,
                                       const std::vector<NotificationAction>& actions, QWidget* parent)
    : QFrame(parent)
{
    setObjectName("NotificationToast");
    setFrameShape(QFrame::StyledPanel);
    setFixedWidth(340);
    setStyleSheet(
        "#NotificationToast { background-color: #3c3f41; border: 1px solid #5a5d5e; border-radius: 6px; }"
        "#NotificationToast QLabel { color: #e3e3e3; background: transparent; padding: 0px; margin: 0px; }"
        "#NotificationToast QLabel#NotifTitle { font-weight: bold; font-size: 13px; padding: 0px; margin: 0px; }"
        "#NotificationToast QToolButton { padding: 0px; margin: 0px; border: none; }"
        "#NotificationToast QPushButton { padding: 3px 10px; margin: 0px; }"
    );

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 6, 5, 7);
    layout->setSpacing(2);

    auto* titleRow = new QHBoxLayout();
    titleRow->setSpacing(4);

    auto* titleLabel = new QLabel(title, this);
    titleLabel->setObjectName("NotifTitle");
    titleLabel->setWordWrap(false);
    titleLabel->setMargin(0);
    titleLabel->setContentsMargins(0, 0, 0, 0);
    titleRow->addWidget(titleLabel, 0, Qt::AlignVCenter);
    titleRow->addStretch(1);

    auto* closeButton = new QToolButton(this);
    closeButton->setIcon(QIcon(":/images/silk/cross.png"));
    closeButton->setIconSize(QSize(12, 12));
    closeButton->setFixedSize(QSize(18, 18));
    closeButton->setAutoRaise(true);
    closeButton->setCursor(Qt::PointingHandCursor);
    closeButton->setToolTip("Dismiss");
    titleRow->addWidget(closeButton, 0, Qt::AlignTop);
    connect(closeButton, &QToolButton::clicked, this, &NotificationWidget::Dismiss);

    layout->addLayout(titleRow);

    auto* messageLabel = new QLabel(message, this);
    messageLabel->setWordWrap(true);
    messageLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    messageLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
    messageLabel->setOpenExternalLinks(true);
    layout->addWidget(messageLabel);
    mMessageLabel = messageLabel;

    if (!actions.empty()) {
        auto* actionRow = new QHBoxLayout();
        actionRow->setContentsMargins(0, 4, 0, 0);
        actionRow->addStretch(1);
        for (const NotificationAction& action : actions) {
            auto* button = new QPushButton(action.Label, this);
            button->setCursor(Qt::PointingHandCursor);
            std::function<void()> callback = action.OnTriggered;
            connect(button, &QPushButton::clicked, this, [this, callback]() {
                if (callback)
                    callback();
                Dismiss();
            });
            actionRow->addWidget(button);
        }
        layout->addLayout(actionRow);
    }
    
    mOpacity = new QGraphicsOpacityEffect(this);
    mOpacity->setOpacity(0.0);
    setGraphicsEffect(mOpacity);

    mFade = new QPropertyAnimation(mOpacity, "opacity", this);
    mFade->setDuration(kFadeMs);

    mDismissTimer = new QTimer(this);
    mDismissTimer->setSingleShot(true);
    mDismissTimer->setInterval(kAutoDismissMs);
    connect(mDismissTimer, &QTimer::timeout, this, &NotificationWidget::Dismiss);
}

void NotificationWidget::Appear() {
    show();
    if (mMessageLabel)
        mMessageLabel->setFixedHeight(mMessageLabel->heightForWidth(mMessageLabel->width()));
    adjustSize();
    raise();
    mFade->stop();
    mFade->setStartValue(mOpacity->opacity());
    mFade->setEndValue(1.0);
    mFade->start();
    mDismissTimer->start();
}

void NotificationWidget::Dismiss() {
    if (mClosing)
        return;
    mClosing = true;
    mDismissTimer->stop();

    mFade->stop();
    mFade->setStartValue(mOpacity->opacity());
    mFade->setEndValue(0.0);
    connect(mFade, &QPropertyAnimation::finished, this, [this]() {
        emit closed(this);
        deleteLater();
    });
    mFade->start();
}

void NotificationWidget::enterEvent(QEnterEvent* event) {
    mDismissTimer->stop();
    QFrame::enterEvent(event);
}

void NotificationWidget::leaveEvent(QEvent* event) {
    if (!mClosing)
        mDismissTimer->start();
    QFrame::leaveEvent(event);
}

NotificationManager::NotificationManager(QWidget* anchor) : QObject(anchor), mAnchor(anchor) {
    if (mAnchor)
        mAnchor->installEventFilter(this);
}

void NotificationManager::Notify(const QString& title, const QString& message,
                                 const std::vector<NotificationAction>& actions) {
    if (!mAnchor)
        return;

    auto* toast = new NotificationWidget(title, message, actions, mAnchor);
    connect(toast, &NotificationWidget::closed, this, &NotificationManager::Remove);
    mToasts.append(toast);
    toast->Appear();
    Reposition();

    if (!mAnchor->isActiveWindow() && gApp)
        gApp->ShowSystemNotification(title, message);
}

void NotificationManager::Reposition() {
    if (!mAnchor)
        return;

    const int margin = 12;
    const int spacing = 10;
    const int bottomInset = 44;

    int y = mAnchor->height() - bottomInset;
    for (int i = mToasts.size() - 1; i >= 0; --i) {
        NotificationWidget* toast = mToasts[i];
        y -= toast->height();
        toast->move(mAnchor->width() - toast->width() - margin, y);
        toast->raise();
        y -= spacing;
    }
}

void NotificationManager::Remove(NotificationWidget* toast) {
    mToasts.removeOne(toast);
    Reposition();
}

bool NotificationManager::eventFilter(QObject* watched, QEvent* event) {
    if (watched == mAnchor && event->type() == QEvent::Resize)
        Reposition();
    return QObject::eventFilter(watched, event);
}

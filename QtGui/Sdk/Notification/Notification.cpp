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
#include <QProgressBar>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTimer>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>
#include <QEnterEvent>
#include <QEvent>
#include <QIcon>
#include <QMenu>
#include <QWidgetAction>
#include <QTime>

#include <algorithm>

using namespace NoobWarrior;

static constexpr int kAutoDismissMs = 12000;
static constexpr int kFadeMs = 200;

Notification::Notification(const QString& title, const QString& message,
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

    mLayout = new QVBoxLayout(this);
    mLayout->setContentsMargins(8, 6, 5, 7);
    mLayout->setSpacing(2);

    auto* titleRow = new QHBoxLayout();
    titleRow->setSpacing(4);

    mTitleLabel = new QLabel(title, this);
    mTitleLabel->setObjectName("NotifTitle");
    mTitleLabel->setWordWrap(false);
    titleRow->addWidget(mTitleLabel, 0, Qt::AlignVCenter);
    titleRow->addStretch(1);

    auto* closeButton = new QToolButton(this);
    closeButton->setIcon(QIcon(":/images/silk/cross.png"));
    closeButton->setIconSize(QSize(12, 12));
    closeButton->setFixedSize(QSize(18, 18));
    closeButton->setAutoRaise(true);
    closeButton->setCursor(Qt::PointingHandCursor);
    closeButton->setToolTip("Dismiss");
    titleRow->addWidget(closeButton, 0, Qt::AlignTop);
    connect(closeButton, &QToolButton::clicked, this, &Notification::Dismiss);

    mLayout->addLayout(titleRow);

    mMessageLabel = new QLabel(message, this);
    mMessageLabel->setWordWrap(true);
    mMessageLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    mMessageLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
    mMessageLabel->setOpenExternalLinks(true);
    mLayout->addWidget(mMessageLabel);

    // Created up front but hidden; SetProgress reveals it so it always sits right under the message.
    mProgressBar = new QProgressBar(this);
    mProgressBar->setTextVisible(false);
    mProgressBar->setFixedHeight(8);
    mProgressBar->hide();
    mLayout->addWidget(mProgressBar);

    if (!actions.empty()) {
        mActionRow = new QWidget(this);
        auto* actionLayout = new QHBoxLayout(mActionRow);
        actionLayout->setContentsMargins(0, 4, 0, 0);
        actionLayout->addStretch(1);
        for (const NotificationAction& action : actions) {
            auto* button = new QPushButton(action.Label, mActionRow);
            button->setCursor(Qt::PointingHandCursor);
            std::function<void()> callback = action.OnTriggered;
            connect(button, &QPushButton::clicked, this, [this, callback]() {
                if (callback)
                    callback();
                Dismiss();
            });
            actionLayout->addWidget(button);
        }
        mLayout->addWidget(mActionRow);
    }

    mOpacity = new QGraphicsOpacityEffect(this);
    mOpacity->setOpacity(0.0);
    setGraphicsEffect(mOpacity);

    mFade = new QPropertyAnimation(mOpacity, "opacity", this);
    mFade->setDuration(kFadeMs);

    mDismissTimer = new QTimer(this);
    mDismissTimer->setSingleShot(true);
    mDismissTimer->setInterval(kAutoDismissMs);
    connect(mDismissTimer, &QTimer::timeout, this, &Notification::Dismiss);
}

void Notification::SetTitle(const QString& title) {
    mTitleLabel->setText(title);
}

void Notification::SetMessage(const QString& message) {
    mMessageLabel->setText(message);
    if (isVisible())
        mMessageLabel->setFixedHeight(mMessageLabel->heightForWidth(mMessageLabel->width()));
    adjustSize();
    emit resized();
}

void Notification::SetProgress(double progress) {
    mProgressBar->show();
    if (progress < 0.0) {
        mProgressBar->setRange(0, 0); // indeterminate "busy" bar
    } else {
        mProgressBar->setRange(0, 100);
        mProgressBar->setValue(static_cast<int>(std::min(progress, 1.0) * 100));
    }

    // While work is ongoing the toast must stay put; once it's done, let it fade like any other.
    mPersistent = progress < 1.0;
    if (mPersistent)
        mDismissTimer->stop();
    else
        ScheduleDismiss();

    adjustSize();
    emit resized();
}

void Notification::SetContent(QWidget* content) {
    if (!content)
        return;
    content->setParent(this);
    // Sit above the action row (if any) but below the message/progress.
    const int index = mActionRow ? mLayout->indexOf(mActionRow) : mLayout->count();
    mLayout->insertWidget(index, content);
    adjustSize();
    emit resized();
}

void Notification::Appear() {
    show();
    mMessageLabel->setFixedHeight(mMessageLabel->heightForWidth(mMessageLabel->width()));
    adjustSize();
    raise();
    mFade->stop();
    mFade->setStartValue(mOpacity->opacity());
    mFade->setEndValue(1.0);
    mFade->start();
    ScheduleDismiss();
}

void Notification::Dismiss() {
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

void Notification::ScheduleDismiss() {
    // Persistent toasts and toasts under the cursor stay until the user is done with them.
    if (mPersistent || mClosing || underMouse())
        return;
    mDismissTimer->start();
}

void Notification::enterEvent(QEnterEvent* event) {
    mDismissTimer->stop();
    QFrame::enterEvent(event);
}

void Notification::leaveEvent(QEvent* event) {
    ScheduleDismiss();
    QFrame::leaveEvent(event);
}

NotificationManager::NotificationManager(QWidget* anchor) : QObject(anchor), mAnchor(anchor) {
    if (mAnchor)
        mAnchor->installEventFilter(this);
}

Notification* NotificationManager::Notify(const QString& title, const QString& message,
                                          const std::vector<NotificationAction>& actions) {
    static constexpr int kMaxHistory = 50;
    mHistory.append(NotificationRecord{ title, message, QTime::currentTime().toString("HH:mm"), actions });
    while (mHistory.size() > kMaxHistory)
        mHistory.removeFirst();
    emit historyChanged();

    return Add(title, message, actions);
}

Notification* NotificationManager::StartTask(const QString& title, const QString& message) {
    Notification* toast = Add(title, message, {});
    if (toast)
        toast->SetProgress(-1.0); // indeterminate until the caller reports real progress
    return toast;
}

Notification* NotificationManager::Add(const QString& title, const QString& message,
                                       const std::vector<NotificationAction>& actions) {
    if (!mAnchor)
        return nullptr;

    auto* toast = new Notification(title, message, actions, mAnchor);
    connect(toast, &Notification::closed, this, &NotificationManager::Remove);
    connect(toast, &Notification::resized, this, &NotificationManager::Reposition);
    mToasts.append(toast);
    toast->Appear();
    Reposition();

    if (!mAnchor->isActiveWindow() && gApp)
        gApp->ShowSystemNotification(title, message);

    return toast;
}

void NotificationManager::Reposition() {
    if (!mAnchor)
        return;

    const int margin = 12;
    const int spacing = 10;
    const int bottomInset = 44;

    int y = mAnchor->height() - bottomInset;
    for (int i = mToasts.size() - 1; i >= 0; --i) {
        Notification* toast = mToasts[i];
        y -= toast->height();
        toast->move(mAnchor->width() - toast->width() - margin, y);
        toast->raise();
        y -= spacing;
    }
}

void NotificationManager::Remove(Notification* toast) {
    mToasts.removeOne(toast);
    Reposition();
}

bool NotificationManager::eventFilter(QObject* watched, QEvent* event) {
    if (watched == mAnchor && event->type() == QEvent::Resize)
        Reposition();
    return QObject::eventFilter(watched, event);
}

NotificationHistoryButton::NotificationHistoryButton(NotificationManager* manager, QWidget* parent)
    : QToolButton(parent), mManager(manager)
{
    setIcon(QIcon(":/images/silk/bell.png"));
    setIconSize(QSize(16, 16));
    setAutoRaise(true);
    setCursor(Qt::PointingHandCursor);
    setToolTip("Notifications");
    setPopupMode(QToolButton::InstantPopup);
    // Keep the button as small as the icon so it doesn't grow the status bar's minimum height.
    setStyleSheet("QToolButton { border: none; padding: 0px; margin: 0px; }"
                  "QToolButton::menu-indicator { image: none; width: 0px; }");
    setFixedSize(QSize(16, 16));

    mMenu = new QMenu(this);
    mMenu->setStyleSheet("QMenu { menu-scrollable: 1; }");
    setMenu(mMenu);

    if (mManager)
        connect(mManager, &NotificationManager::historyChanged, this, &NotificationHistoryButton::RebuildMenu);
    RebuildMenu();
}

void NotificationHistoryButton::RebuildMenu() {
    mMenu->clear();

    const QList<NotificationRecord>& history = mManager ? mManager->History() : QList<NotificationRecord>{};
    if (history.isEmpty()) {
        QAction* empty = mMenu->addAction("No notifications yet");
        empty->setEnabled(false);
        return;
    }

    // Newest first.
    for (int i = history.size() - 1; i >= 0; --i) {
        const NotificationRecord& record = history[i];

        auto* entry = new QWidget(mMenu);
        auto* layout = new QVBoxLayout(entry);
        layout->setContentsMargins(8, 4, 8, 4);
        layout->setSpacing(1);

        auto* titleLabel = new QLabel(QString("<b>%1</b>  <span style='color:#9a9a9a'>%2</span>")
                                          .arg(record.Title.toHtmlEscaped(), record.Time), entry);
        layout->addWidget(titleLabel);

        if (!record.Message.isEmpty()) {
            auto* messageLabel = new QLabel(record.Message, entry);
            messageLabel->setWordWrap(true);
            messageLabel->setFixedWidth(300);
            messageLabel->setStyleSheet("color: #c8c8c8;");
            layout->addWidget(messageLabel);
        }

        if (!record.Actions.empty()) {
            auto* buttonRow = new QHBoxLayout();
            buttonRow->setContentsMargins(0, 2, 0, 0);
            buttonRow->addStretch(1);
            for (const NotificationAction& act : record.Actions) {
                auto* button = new QPushButton(act.Label, entry);
                button->setCursor(Qt::PointingHandCursor);
                std::function<void()> callback = act.OnTriggered;
                connect(button, &QPushButton::clicked, mMenu, [this, callback]() {
                    if (callback)
                        callback();
                    mMenu->close();
                });
                buttonRow->addWidget(button);
            }
            layout->addLayout(buttonRow);
        }

        auto* action = new QWidgetAction(mMenu);
        action->setDefaultWidget(entry);
        mMenu->addAction(action);

        if (i > 0)
            mMenu->addSeparator();
    }
}

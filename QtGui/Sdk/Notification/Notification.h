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
// File: Notification.h
// Started by: Hattozo
// Started on: 6/17/2026
// Description: Corner toast notifications and the manager that stacks them. A toast can also carry
// a progress bar, which turns it into an ongoing-task notification you update while work runs.
#pragma once
#include <QFrame>
#include <QObject>
#include <QList>
#include <QString>
#include <QToolButton>

#include <functional>
#include <vector>

class QLabel;
class QMenu;
class QTimer;
class QProgressBar;
class QVBoxLayout;
class QPropertyAnimation;
class QGraphicsOpacityEffect;

namespace NoobWarrior {
struct NotificationAction {
    QString Label;
    std::function<void()> OnTriggered;
};

// A plain toast, remembered so the user can review it later from the status-bar history.
struct NotificationRecord {
    QString Title;
    QString Message;
    QString Time; // wall-clock time it was posted, formatted "HH:mm"
    std::vector<NotificationAction> Actions;
};

class Notification : public QFrame {
    Q_OBJECT
public:
    Notification(const QString& title, const QString& message,
                 const std::vector<NotificationAction>& actions, QWidget* parent);

    void SetTitle(const QString& title);
    void SetMessage(const QString& message);

    // A value in [0, 1] fills the bar; a negative value shows an indeterminate "busy" bar. While
    // progress is below 1 the toast will not auto-dismiss; reaching 1 lets it fade like any toast.
    void SetProgress(double progress);

    // Places an extra widget (e.g. a backup tree) beneath the message. Takes ownership.
    void SetContent(QWidget* content);

    void Appear();
    void Dismiss();
signals:
    void closed(Notification* self);
    void resized(); // height changed; the manager restacks the column in response
protected:
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;
private:
    void ScheduleDismiss();

    QVBoxLayout* mLayout { nullptr };
    QLabel* mTitleLabel { nullptr };
    QLabel* mMessageLabel { nullptr };
    QProgressBar* mProgressBar { nullptr };
    QWidget* mActionRow { nullptr };
    QGraphicsOpacityEffect* mOpacity { nullptr };
    QPropertyAnimation* mFade { nullptr };
    QTimer* mDismissTimer { nullptr };
    bool mPersistent { false };
    bool mClosing { false };
};

class NotificationManager : public QObject {
    Q_OBJECT
public:
    explicit NotificationManager(QWidget* anchor);

    // A plain toast that fades away on its own.
    Notification* Notify(const QString& title, const QString& message,
                         const std::vector<NotificationAction>& actions = {});

    // A persistent toast with an indeterminate progress bar, for ongoing work. Update it via
    // Notification::SetProgress / SetMessage; it dismisses itself once progress reaches 1.
    Notification* StartTask(const QString& title, const QString& message = QString());

    // Notifications posted via Notify, newest last. Toasts fade, so this is how the user catches up.
    const QList<NotificationRecord>& History() const { return mHistory; }
signals:
    void historyChanged();
protected:
    bool eventFilter(QObject* watched, QEvent* event) override;
private:
    Notification* Add(const QString& title, const QString& message,
                      const std::vector<NotificationAction>& actions);
    void Reposition();
    void Remove(Notification* toast);

    QWidget* mAnchor { nullptr };
    QList<Notification*> mToasts; // oldest first; newest sits lowest in the corner
    QList<NotificationRecord> mHistory;
};

// A small bell button for the status bar. Clicking it drops down the notification history.
class NotificationHistoryButton : public QToolButton {
    Q_OBJECT
public:
    NotificationHistoryButton(NotificationManager* manager, QWidget* parent = nullptr);
private:
    void RebuildMenu();

    NotificationManager* mManager { nullptr };
    QMenu* mMenu { nullptr };
};
}

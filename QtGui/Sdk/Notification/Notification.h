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
// Description: Corner toast notifications and the manager that stacks them.
#pragma once
#include <QFrame>
#include <QObject>
#include <QList>
#include <QString>
#include <QLabel>

#include <functional>
#include <vector>

class QLabel;
class QTimer;
class QPropertyAnimation;
class QGraphicsOpacityEffect;

namespace NoobWarrior {
struct NotificationAction {
    QString Label;
    std::function<void()> OnTriggered;
};

class NotificationWidget : public QFrame {
    Q_OBJECT
public:
    NotificationWidget(const QString& title, const QString& message,
                       const std::vector<NotificationAction>& actions, QWidget* parent);

    void Appear();
    void Dismiss();
signals:
    void closed(NotificationWidget* self);
protected:
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;
private:
    QLabel* mMessageLabel = nullptr;
    QGraphicsOpacityEffect* mOpacity { nullptr };
    QPropertyAnimation* mFade { nullptr };
    QTimer* mDismissTimer { nullptr };
    bool mClosing { false };
};

class NotificationManager : public QObject {
    Q_OBJECT
public:
    explicit NotificationManager(QWidget* anchor);

    void Notify(const QString& title, const QString& message,
                const std::vector<NotificationAction>& actions = {});
protected:
    bool eventFilter(QObject* watched, QEvent* event) override;
private:
    void Reposition();
    void Remove(NotificationWidget* toast);

    QWidget* mAnchor { nullptr };
    QList<NotificationWidget*> mToasts; // oldest first; newest sits lowest in the corner
};
}

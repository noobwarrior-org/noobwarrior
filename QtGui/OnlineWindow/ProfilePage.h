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
// File: ProfilePage.h
// Started by: Hattozo
// Started on: 8/23/2026
// Description: Your account on one master server: sign in, and what that master knows about you
#pragma once
#include <QWidget>

class QFormLayout;
class QLabel;
class QLineEdit;
class QListWidget;
class QPushButton;
class QStackedWidget;

namespace NoobWarrior {
// Two states over the same master: signed out shows a sign-in form, signed in shows the identity
// that master issues you. Which one is live is decided by the master keychain, so signing in from
// the sidebar (or from a server join) shows up here too once Reload() runs.
class ProfilePage : public QWidget {
    Q_OBJECT
public:
    explicit ProfilePage(QWidget *parent = nullptr);

    void SetMaster(const QString &masterUrl);
    // Re-reads the sign-in state and, when signed in, re-fetches the profile.
    void Reload();

    // Shows somebody else's public profile (identity is "name" or "name@domain") instead of your
    // own. Read-only: no sign-out, no bio of yours, and a Back button to return to your account.
    void ViewIdentity(const QString &identity);
    // Drops back to your own account view.
    void ViewSelf();

signals:
    // The signed-in identity changed, so the sidebar and other pages should catch up.
    void SessionChanged();
    void StatusChanged(const QString &status);

signals:
    // The Back button on a viewed profile; the window returns to whatever it was showing.
    void BackRequested();

private:
    void InitWidgets();
    QWidget *BuildSignInPage();
    QWidget *BuildDetailsPage();
    QLabel *AddRow(QFormLayout *form, QWidget *parent, const QString &label);

    void AttemptSignIn();
    void SignOut();
    void FetchProfile();
    // Fills the details page from a /v1/profile or /v1/users/:name payload; both share a shape.
    void ApplyProfileJson(const std::string &body);

    QString mMasterUrl;
    // Empty when showing your own account, else the identity being looked at.
    QString mViewedIdentity;

    QStackedWidget *mStack { nullptr };

    // Sign-in page
    QWidget *mSignInPage { nullptr };
    QLabel *mSignInHeading { nullptr };
    QLineEdit *mUsername { nullptr };
    QLineEdit *mPassword { nullptr };
    QPushButton *mSignInButton { nullptr };
    QLabel *mSignInStatus { nullptr };

    // Details page
    QWidget *mDetailsPage { nullptr };
    QLabel *mDisplayName { nullptr };
    QLabel *mIdentityLabel { nullptr };
    QLabel *mUsernameValue { nullptr };
    QLabel *mOnlineIdValue { nullptr };
    QLabel *mDomainValue { nullptr };
    QLabel *mJoinedValue { nullptr };
    QLabel *mSubmissionsValue { nullptr };
    QLabel *mBioValue { nullptr };
    QListWidget *mStatusList { nullptr };
    QLabel *mDetailsStatus { nullptr };
    // Hidden while viewing your own profile, and vice versa for the owner-only buttons.
    QPushButton *mBackButton { nullptr };
    QPushButton *mSignOutButton { nullptr };
    QPushButton *mRefreshButton { nullptr };
};
}

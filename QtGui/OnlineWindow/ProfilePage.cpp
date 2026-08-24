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
// File: ProfilePage.cpp
// Started by: Hattozo
// Started on: 8/23/2026
// Description: Your account on one master server: sign in, and what that master knows about you
#include "ProfilePage.h"
#include "MasterHttp.h"
#include "MasterServerStore.h"
#include "../Application.h"

#include <nlohmann/json.hpp>

#include <QDateTime>
#include <QDesktopServices>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QStackedWidget>
#include <QUrl>
#include <QVBoxLayout>

using namespace NoobWarrior;

ProfilePage::ProfilePage(QWidget *parent) : QWidget(parent) {
    InitWidgets();
}

QLabel *ProfilePage::AddRow(QFormLayout *form, QWidget *parent, const QString &label) {
    auto *value = new QLabel(parent);
    value->setWordWrap(true);
    value->setTextInteractionFlags(Qt::TextSelectableByMouse);
    form->addRow(label, value);
    return value;
}

void ProfilePage::InitWidgets() {
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    mStack = new QStackedWidget(this);
    mSignInPage = BuildSignInPage();
    mDetailsPage = BuildDetailsPage();
    mStack->addWidget(mSignInPage);
    mStack->addWidget(mDetailsPage);

    layout->addWidget(mStack);
}

QWidget *ProfilePage::BuildSignInPage() {
    auto *page = new QWidget(this);
    auto *layout = new QVBoxLayout(page);
    layout->setAlignment(Qt::AlignTop);

    mSignInHeading = new QLabel(page);
    QFont headingFont = mSignInHeading->font();
    headingFont.setBold(true);
    headingFont.setPointSize(headingFont.pointSize() + 3);
    mSignInHeading->setFont(headingFont);
    layout->addWidget(mSignInHeading);

    auto *blurb = new QLabel(
        "Signing in gives you an identity on this master server. It is the name you appear under\n"
        "when you join servers that trust it, and what you need to upload to its workshop.", page);
    blurb->setWordWrap(true);
    layout->addWidget(blurb);
    layout->addSpacing(12);

    auto *form = new QFormLayout();
    form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

    mUsername = new QLineEdit(page);
    form->addRow("Username", mUsername);

    mPassword = new QLineEdit(page);
    mPassword->setEchoMode(QLineEdit::Password);
    form->addRow("Password", mPassword);

    layout->addLayout(form);

    connect(mUsername, &QLineEdit::returnPressed, this, &ProfilePage::AttemptSignIn);
    connect(mPassword, &QLineEdit::returnPressed, this, &ProfilePage::AttemptSignIn);

    auto *buttons = new QHBoxLayout();
    mSignInButton = new QPushButton(QIcon(":/images/silk/door_in.png"), "Sign In", page);
    connect(mSignInButton, &QPushButton::clicked, this, &ProfilePage::AttemptSignIn);
    buttons->addWidget(mSignInButton);
    
    auto *registerButton = new QPushButton("Create an account...", page);
    connect(registerButton, &QPushButton::clicked, this, [this]() {
        if (mMasterUrl.isEmpty())
            return;
        QDesktopServices::openUrl(QUrl(MasterHttp::ResolveUrl(mMasterUrl, "/register")));
    });
    buttons->addWidget(registerButton);
    buttons->addStretch();
    layout->addLayout(buttons);

    mSignInStatus = new QLabel(page);
    mSignInStatus->setWordWrap(true);
    layout->addWidget(mSignInStatus);

    layout->addStretch();
    return page;
}

QWidget *ProfilePage::BuildDetailsPage() {
    auto *page = new QWidget(this);
    auto *layout = new QVBoxLayout(page);
    layout->setAlignment(Qt::AlignTop);

    mDisplayName = new QLabel(page);
    QFont nameFont = mDisplayName->font();
    nameFont.setBold(true);
    nameFont.setPointSize(nameFont.pointSize() + 5);
    mDisplayName->setFont(nameFont);
    layout->addWidget(mDisplayName);

    mIdentityLabel = new QLabel(page);
    mIdentityLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    layout->addWidget(mIdentityLabel);
    layout->addSpacing(12);

    auto *form = new QFormLayout();
    form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    mUsernameValue = AddRow(form, page, "Username");
    mOnlineIdValue = AddRow(form, page, "In-game user ID");
    mDomainValue = AddRow(form, page, "Home domain");
    mJoinedValue = AddRow(form, page, "Joined");
    mSubmissionsValue = AddRow(form, page, "Workshop uploads");
    mBioValue = AddRow(form, page, "Bio");
    layout->addLayout(form);

    layout->addSpacing(12);
    auto *statusesLabel = new QLabel("Recent statuses", page);
    QFont statusesFont = statusesLabel->font();
    statusesFont.setBold(true);
    statusesLabel->setFont(statusesFont);
    layout->addWidget(statusesLabel);

    mStatusList = new QListWidget(page);
    mStatusList->setAlternatingRowColors(true);
    mStatusList->setWordWrap(true);
    layout->addWidget(mStatusList, 1);

    auto *buttons = new QHBoxLayout();

    mBackButton = new QPushButton(QIcon(":/images/silk/arrow_left.png"), "Back", page);
    connect(mBackButton, &QPushButton::clicked, this, [this]() {
        ViewSelf();
        emit BackRequested();
    });
    buttons->addWidget(mBackButton);

    mRefreshButton = new QPushButton(QIcon(":/images/silk/arrow_refresh.png"), "Refresh", page);
    connect(mRefreshButton, &QPushButton::clicked, this, &ProfilePage::FetchProfile);
    buttons->addWidget(mRefreshButton);

    auto *openButton = new QPushButton("Open in Browser", page);
    connect(openButton, &QPushButton::clicked, this, [this]() {
        if (mMasterUrl.isEmpty())
            return;
        QString path = mViewedIdentity.isEmpty()
            ? QString("/profile")
            : QString("/@%1").arg(mViewedIdentity);
        QDesktopServices::openUrl(QUrl(MasterHttp::ResolveUrl(mMasterUrl, path)));
    });
    buttons->addWidget(openButton);

    mSignOutButton = new QPushButton(QIcon(":/images/silk/door_out.png"), "Sign Out", page);
    connect(mSignOutButton, &QPushButton::clicked, this, &ProfilePage::SignOut);
    buttons->addWidget(mSignOutButton);

    buttons->addStretch();
    layout->addLayout(buttons);

    mDetailsStatus = new QLabel(page);
    mDetailsStatus->setWordWrap(true);
    layout->addWidget(mDetailsStatus);

    return page;
}

void ProfilePage::SetMaster(const QString &masterUrl) {
    // Reload unconditionally: coming back to the same master should re-check the session, which may
    // have changed from the sidebar or a server join since we were last shown.
    if (mMasterUrl != masterUrl) {
        mMasterUrl = masterUrl;
        mUsername->clear();
        mPassword->clear();
        mSignInStatus->clear();
    }
    Reload();
}

void ProfilePage::ViewIdentity(const QString &identity) {
    if (identity.isEmpty())
        return;
    mViewedIdentity = identity;
    Reload();
}

void ProfilePage::ViewSelf() {
    if (mViewedIdentity.isEmpty())
        return;
    mViewedIdentity.clear();
    Reload();
}

void ProfilePage::Reload() {
    QString name = mMasterUrl.isEmpty() ? QString("this master server")
                                        : MasterServerStore::HostOf(mMasterUrl);
    mSignInHeading->setText("Sign in to " + name);

    // Somebody else's profile is public, so it shows without a session of our own; the owner-only
    // controls come off instead.
    bool viewingOther = !mViewedIdentity.isEmpty();
    mBackButton->setVisible(viewingOther);
    mSignOutButton->setVisible(!viewingOther);
    mRefreshButton->setVisible(true);

    if (viewingOther) {
        mStack->setCurrentWidget(mDetailsPage);
        FetchProfile();
        return;
    }

    if (mMasterUrl.isEmpty() || !MasterHttp::IsSignedIn(mMasterUrl)) {
        mStack->setCurrentWidget(mSignInPage);
        return;
    }

    mStack->setCurrentWidget(mDetailsPage);
    FetchProfile();
}

void ProfilePage::AttemptSignIn() {
    if (mMasterUrl.isEmpty())
        return;

    QString username = mUsername->text().trimmed();
    QString password = mPassword->text();
    if (username.isEmpty() || password.isEmpty()) {
        mSignInStatus->setText("Enter a username and password.");
        return;
    }

    mSignInButton->setEnabled(false);
    mSignInStatus->setText("Signing in...");

    MasterHttp::SignIn(this, mMasterUrl, username, password, [this](bool ok) {
        mSignInButton->setEnabled(true);
        if (!ok) {
            mSignInStatus->setText("That master server rejected those credentials.");
            return;
        }
        mPassword->clear();
        mSignInStatus->clear();
        emit SessionChanged();
        Reload();
    });
}

void ProfilePage::SignOut() {
    if (mMasterUrl.isEmpty())
        return;
    MasterHttp::SignOut(mMasterUrl);
    emit SessionChanged();
    Reload();
}

void ProfilePage::FetchProfile() {
    if (mMasterUrl.isEmpty())
        return;

    mDetailsStatus->setText("Loading profile...");
    emit StatusChanged("Loading profile...");

    // A viewed identity may be "name@domain"; the route takes the whole thing and resolves a
    // foreign domain through federation.
    QString path = mViewedIdentity.isEmpty()
        ? QString("/v1/profile")
        : QString("/v1/users/%1").arg(QString::fromUtf8(QUrl::toPercentEncoding(mViewedIdentity)));

    MasterHttp::Get(this, mMasterUrl, path, [this](const MasterResponse &response) {
        // A session the master no longer honours (expired, or the account was deleted) reads exactly
        // like a signed-out one, so drop the stale keychain entry instead of showing an error.
        // Only meaningful for our own profile; a public one never needs a session.
        if (response.Status == 401 && mViewedIdentity.isEmpty()) {
            MasterHttp::SignOut(mMasterUrl);
            mSignInStatus->setText("Your sign-in expired. Sign in again.");
            emit SessionChanged();
            Reload();
            return;
        }

        if (!response.Ok) {
            mDetailsStatus->setText("Couldn't load your profile: " + QString::fromStdString(response.Error));
            emit StatusChanged("Could not load profile.");
            return;
        }

        nlohmann::json profile = nlohmann::json::parse(response.Body, nullptr, false);
        if (profile.is_discarded() || !profile.is_object()) {
            mDetailsStatus->setText("The master server sent a profile we could not read.");
            return;
        }

        QString name = QString::fromStdString(profile.value("Name", std::string{}));
        QString displayName = QString::fromStdString(profile.value("DisplayName", std::string{}));
        QString identity = QString::fromStdString(profile.value("Identity", std::string{}));

        mDisplayName->setText(displayName.isEmpty() ? name : displayName);
        mIdentityLabel->setText(identity);
        mUsernameValue->setText("@" + name);
        mOnlineIdValue->setText(QString::number(profile.value("OnlineUserId", static_cast<int64_t>(0))));
        mDomainValue->setText(QString::fromStdString(profile.value("Domain", std::string{})));

        auto joinDate = profile.value("JoinDate", static_cast<int64_t>(0));
        mJoinedValue->setText(joinDate > 0
            ? QDateTime::fromSecsSinceEpoch(joinDate).toString("MMMM d, yyyy")
            : QString("Unknown"));

        mSubmissionsValue->setText(QString::number(profile.value("Submissions", 0)));

        QString bio = QString::fromStdString(profile.value("Bio", std::string{})).trimmed();
        mBioValue->setText(bio.isEmpty() ? "No bio set." : bio);

        mStatusList->clear();
        // An empty Lua table serialises as {}, not [], so anything that isn't an array means none.
        nlohmann::json statuses = profile.value("Statuses", nlohmann::json{});
        if (statuses.is_array()) {
            for (const auto &status : statuses) {
                QString body = QString::fromStdString(status.value("Body", std::string{}));
                auto created = status.value("Created", static_cast<int64_t>(0));
                QString when = created > 0
                    ? QDateTime::fromSecsSinceEpoch(created).toString("MMM d, yyyy h:mm ap")
                    : QString();
                QString prefix = status.value("IsReply", false) ? QString("Reply - ") : QString();
                mStatusList->addItem(QString("%1%2\n%3").arg(prefix, when, body));
            }
        }
        if (mStatusList->count() == 0)
            mStatusList->addItem("No statuses yet.");

        mDetailsStatus->clear();
        emit StatusChanged(mViewedIdentity.isEmpty() ? "Signed in as " + identity
                                                     : "Viewing " + identity);
    });
}

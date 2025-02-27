// === noobWarrior ===
// File: ArchiveEditor.h
// Started by: Hattozo
// Started on: 12/15/2024
// Description: Qt window that lets users view and edit a noobWarrior archive
#pragma once

#include <NoobWarrior/Archive.h>

#include "OutlineWidget.h"

#include <QMainWindow>
#include <QMenu>
#include <QObject>
#include <QToolBar>

namespace NoobWarrior {
    class ArchiveEditor : public QMainWindow {
        Q_OBJECT
        
    public:
        ArchiveEditor(QWidget *parent = nullptr);
        ~ArchiveEditor();

        void TryToCreateFile(const QString &path);
        void TryToOpenFile(const QString &path);
    protected:
        void closeEvent(QCloseEvent *event) override;
    private:
        void InitMenus();
        void InitWidgets();

        QMenu *mFileMenu;
        QToolBar *mFileToolBar;
        QToolBar *mViewToolBar;
        QTabWidget *mTabWidget;

        Archive *mCurrentArchive;
    };
}
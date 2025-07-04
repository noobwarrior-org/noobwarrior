// === noobWarrior ===
// File: OverviewWidget.cpp
// Started by: Hattozo
// Started on: 7/4/2025
// Description: The page that you see that details all the statistics when you open a database in the editor.
#include "OverviewWidget.h"

#include <QLabel>

using namespace NoobWarrior;

OverviewWidget::OverviewWidget(Database *db, QWidget *parent) : QWidget(parent),
    mDatabase(db),
    MainLayout(nullptr)
{
    InitWidgets();
}

void OverviewWidget::InitWidgets() {
    MainLayout = new QVBoxLayout(this);

    MainLayout->setSpacing(5);

    auto *metadataLayout = new QHBoxLayout();
    metadataLayout->setSpacing(5);

    QImage image;
    image.loadFromData(mDatabase->GetIcon());

    QPixmap pixmap = QPixmap::fromImage(image);

    auto *imageLabel = new QLabel();
    imageLabel->setAlignment(Qt::AlignLeft);
    imageLabel->setPixmap(pixmap);
    metadataLayout->addWidget(imageLabel);

    MainLayout->addLayout(metadataLayout);
}

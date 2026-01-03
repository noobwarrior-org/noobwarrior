// === noobWarrior ===
// File: ThumbnailPreviewWidget.h
// Started by: Hattozo
// Started on: 11/27/2025
// Description:
#pragma once
#include <QWidget>
#include <QMediaPlayer>

#include <filesystem>

namespace NoobWarrior {
class ThumbnailPreviewWidget : public QWidget {
    Q_OBJECT
public:
    ThumbnailPreviewWidget(QWidget *parent = nullptr);
    void AddMediaFromData(std::vector<unsigned char> &data);
    void AddMediaFromUrl(const std::string &url);
    void AddMediaFromFile(const std::filesystem::path &file);
protected:
    QMediaPlayer* Player;
};
}
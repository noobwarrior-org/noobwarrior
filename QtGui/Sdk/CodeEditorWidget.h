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
// File: CodeEditorWidget.h
// Started by: Hattozo
// Started on: 8/29/2026
// Description: A reusable plain code editor
#pragma once
#include <QPlainTextEdit>
#include <QRegularExpression>
#include <QSyntaxHighlighter>
#include <QTextCharFormat>

namespace NoobWarrior {

// Regex-based Luau/Lua highlighting: keywords, builtins, numbers, strings, comments.
class LuaSyntaxHighlighter : public QSyntaxHighlighter {
    Q_OBJECT
public:
    explicit LuaSyntaxHighlighter(QTextDocument* document);
protected:
    void highlightBlock(const QString& text) override;
private:
    struct Rule {
        QRegularExpression Pattern;
        QTextCharFormat Format;
    };
    QList<Rule> mRules;
    QTextCharFormat mCommentFormat;
};

class CodeEditorWidget : public QPlainTextEdit {
    Q_OBJECT
public:
    explicit CodeEditorWidget(QWidget* parent = nullptr);

    // Enables/disables the Lua highlighter
    void SetLuaHighlighting(bool enabled);

    // Internal: geometry/paint callbacks for the line-number gutter
    int LineNumberAreaWidth() const;
    void LineNumberAreaPaintEvent(QPaintEvent* event);
protected:
    void resizeEvent(QResizeEvent* event) override;
private:
    void UpdateLineNumberAreaWidth(int newBlockCount);
    void UpdateLineNumberArea(const QRect& rect, int dy);
    void HighlightCurrentLine();

    QWidget* mLineNumberArea;
    LuaSyntaxHighlighter* mHighlighter { nullptr };
    QFont mCodeFont;
};
}

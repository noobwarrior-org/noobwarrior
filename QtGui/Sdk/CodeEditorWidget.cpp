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
// File: CodeEditorWidget.cpp
// Started by: Hattozo
// Started on: 8/29/2026
// Description: See CodeEditorWidget.h.
#include "CodeEditorWidget.h"

#include <QFontDatabase>
#include <QPainter>
#include <QRegularExpression>
#include <QTextBlock>

using namespace NoobWarrior;

//////////////////////////////////////////////////////////////////////////////
// LuaSyntaxHighlighter
//////////////////////////////////////////////////////////////////////////////

LuaSyntaxHighlighter::LuaSyntaxHighlighter(QTextDocument* document) : QSyntaxHighlighter(document) {
    QTextCharFormat keywordFormat;
    keywordFormat.setForeground(QColor(86, 156, 214));
    keywordFormat.setFontWeight(QFont::Bold);
    static const char* kKeywords[] = {
        "and", "break", "do", "else", "elseif", "end", "false", "for", "function", "goto", "if",
        "in", "local", "nil", "not", "or", "repeat", "return", "then", "true", "until", "while",
        "continue", // Luau
    };
    for (const char* kw : kKeywords)
        mRules.append({QRegularExpression(QString("\\b%1\\b").arg(kw)), keywordFormat});

    QTextCharFormat builtinFormat;
    builtinFormat.setForeground(QColor(78, 201, 176));
    static const char* kBuiltins[] = {
        "game", "workspace", "script", "print", "warn", "error", "pcall", "xpcall", "select",
        "pairs", "ipairs", "next", "type", "typeof", "tostring", "tonumber", "require", "wait",
        "task", "math", "string", "table", "os", "coroutine", "Instance", "Vector3", "CFrame",
        "Color3", "UDim2", "Enum",
    };
    for (const char* b : kBuiltins)
        mRules.append({QRegularExpression(QString("\\b%1\\b").arg(b)), builtinFormat});

    QTextCharFormat numberFormat;
    numberFormat.setForeground(QColor(181, 206, 168));
    mRules.append({QRegularExpression("\\b(0[xX][0-9a-fA-F]+|\\d+\\.?\\d*([eE][+-]?\\d+)?)\\b"),
                   numberFormat});

    QTextCharFormat stringFormat;
    stringFormat.setForeground(QColor(206, 145, 120));
    mRules.append({QRegularExpression("\"[^\"\\n]*\""), stringFormat});
    mRules.append({QRegularExpression("'[^'\\n]*'"), stringFormat});

    mCommentFormat.setForeground(QColor(106, 153, 85));
}

void LuaSyntaxHighlighter::highlightBlock(const QString& text) {
    for (const Rule& rule : mRules) {
        QRegularExpressionMatchIterator it = rule.Pattern.globalMatch(text);
        while (it.hasNext()) {
            QRegularExpressionMatch match = it.next();
            setFormat(match.capturedStart(), match.capturedLength(), rule.Format);
        }
    }

    // Comments last so they override anything matched inside them.
    constexpr int kInComment = 1;
    setCurrentBlockState(0);

    int start = 0;
    if (previousBlockState() != kInComment) {
        int lineComment = -1;
        QRegularExpression blockStart("--\\[\\[");
        QRegularExpressionMatch bs = blockStart.match(text);
        int plain = text.indexOf("--");
        if (plain != -1 && (!bs.hasMatch() || plain < bs.capturedStart())) {
            // Could still BE the block start; only treat as line comment if it isn't.
            if (!bs.hasMatch() || plain != bs.capturedStart())
                lineComment = plain;
        }
        if (lineComment != -1 && (!bs.hasMatch() || lineComment < bs.capturedStart())) {
            setFormat(lineComment, text.length() - lineComment, mCommentFormat);
            return;
        }
        if (!bs.hasMatch())
            return;
        start = bs.capturedStart();
    }

    while (start >= 0) {
        int end = text.indexOf("]]", start);
        if (end == -1) {
            setCurrentBlockState(kInComment);
            setFormat(start, text.length() - start, mCommentFormat);
            return;
        }
        setFormat(start, end - start + 2, mCommentFormat);
        QRegularExpression blockStart("--\\[\\[");
        QRegularExpressionMatch next = blockStart.match(text, end + 2);
        start = next.hasMatch() ? next.capturedStart() : -1;
    }
}

//////////////////////////////////////////////////////////////////////////////
// CodeEditorWidget
//////////////////////////////////////////////////////////////////////////////

namespace {
class LineNumberArea : public QWidget {
public:
    explicit LineNumberArea(CodeEditorWidget* editor) : QWidget(editor), mEditor(editor) {}
    QSize sizeHint() const override { return QSize(mEditor->LineNumberAreaWidth(), 0); }
protected:
    void paintEvent(QPaintEvent* event) override { mEditor->LineNumberAreaPaintEvent(event); }
private:
    CodeEditorWidget* mEditor;
};
}

CodeEditorWidget::CodeEditorWidget(QWidget* parent) : QPlainTextEdit(parent) {
    mCodeFont = QFont("Fira Mono", 10);
    mCodeFont.setStyleHint(QFont::Monospace);
    setFont(mCodeFont);
    setLineWrapMode(QPlainTextEdit::NoWrap);
    setTabStopDistance(4 * QFontMetricsF(mCodeFont).horizontalAdvance(' '));

    setStyleSheet("QPlainTextEdit { background-color: #272727; color: #CCCCCC; border: none; "
                  "font-family: 'Fira Mono'; font-size: 10pt; }");

    mLineNumberArea = new LineNumberArea(this);
    connect(this, &QPlainTextEdit::blockCountChanged, this, &CodeEditorWidget::UpdateLineNumberAreaWidth);
    connect(this, &QPlainTextEdit::updateRequest, this, &CodeEditorWidget::UpdateLineNumberArea);
    connect(this, &QPlainTextEdit::cursorPositionChanged, this, &CodeEditorWidget::HighlightCurrentLine);
    UpdateLineNumberAreaWidth(0);
    HighlightCurrentLine();

    SetLuaHighlighting(true);
}

void CodeEditorWidget::SetLuaHighlighting(bool enabled) {
    if (enabled && mHighlighter == nullptr) {
        mHighlighter = new LuaSyntaxHighlighter(document());
    } else if (!enabled && mHighlighter != nullptr) {
        delete mHighlighter;
        mHighlighter = nullptr;
    }
}

int CodeEditorWidget::LineNumberAreaWidth() const {
    int digits = 1;
    int max = std::max(1, blockCount());
    while (max >= 10) {
        max /= 10;
        digits++;
    }
    return 10 + QFontMetrics(mCodeFont).horizontalAdvance(QLatin1Char('9')) * digits;
}

void CodeEditorWidget::UpdateLineNumberAreaWidth(int) {
    setViewportMargins(LineNumberAreaWidth(), 0, 0, 0);
}

void CodeEditorWidget::UpdateLineNumberArea(const QRect& rect, int dy) {
    if (dy != 0)
        mLineNumberArea->scroll(0, dy);
    else
        mLineNumberArea->update(0, rect.y(), mLineNumberArea->width(), rect.height());
    if (rect.contains(viewport()->rect()))
        UpdateLineNumberAreaWidth(0);
}

void CodeEditorWidget::resizeEvent(QResizeEvent* event) {
    QPlainTextEdit::resizeEvent(event);
    QRect cr = contentsRect();
    mLineNumberArea->setGeometry(QRect(cr.left(), cr.top(), LineNumberAreaWidth(), cr.height()));
}

void CodeEditorWidget::HighlightCurrentLine() {
    QList<QTextEdit::ExtraSelection> selections;
    if (!isReadOnly()) {
        QTextEdit::ExtraSelection selection;
        selection.format.setBackground(QColor(0x2f, 0x2f, 0x2f));
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);
        selection.cursor = textCursor();
        selection.cursor.clearSelection();
        selections.append(selection);
    }
    setExtraSelections(selections);
}

void CodeEditorWidget::LineNumberAreaPaintEvent(QPaintEvent* event) {
    QPainter painter(mLineNumberArea);
    painter.fillRect(event->rect(), QColor(0x23, 0x23, 0x23));
    painter.setPen(QColor(0x6e, 0x6e, 0x6e));
    painter.setFont(mCodeFont);

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom = top + qRound(blockBoundingRect(block).height());

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            painter.drawText(0, top, mLineNumberArea->width() - 4, QFontMetrics(mCodeFont).height(),
                             Qt::AlignRight, QString::number(blockNumber + 1));
        }
        block = block.next();
        top = bottom;
        bottom = top + qRound(blockBoundingRect(block).height());
        blockNumber++;
    }
}

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
// File: FluentStyle.cpp
// Started by: Hattozo
// Started on: 10/9/2026
// Description: A cool fluent theme
#include "FluentStyle.h"

#include <NoobWarrior/Macros.h>

#include <QAbstractItemView>
#include <QAbstractSpinBox>
#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QCursor>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDockWidget>
#include <QEvent>
#include <QHBoxLayout>
#include <QHoverEvent>
#include <QLabel>
#include <QListView>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPainter>
#include <QPainterPath>
#include <QStyleFactory>
#include <QStyleHints>
#include <QStyleOption>
#include <QTabWidget>
#include <QToolBar>
#include <QToolButton>
#include <QTreeView>

#include <vector>

#if defined(Q_OS_WIN)
#include <QAbstractNativeEventFilter>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <windowsx.h>
#include <dwmapi.h>
#ifndef DWMWA_BORDER_COLOR
#define DWMWA_BORDER_COLOR 34
#endif
#endif

using namespace NoobWarrior;

namespace {
const QColor kShell(0x1c, 0x1c, 0x1c);
const QColor kPane(0x28, 0x28, 0x28);
const QColor kMenu(0x2c, 0x2c, 0x2c);
const QColor kMenuEdge(0x14, 0x14, 0x14);
const QColor kMenuHighlight(0x3d, 0x3d, 0x3d);
const QColor kMenuBarHover(0x2d, 0x2d, 0x2d);
const QColor kControl(0x35, 0x35, 0x35);
const QColor kControlHover(0x3d, 0x3d, 0x3d);
const QColor kControlPressed(0x2e, 0x2e, 0x2e);
const QColor kControlDisabled(0x2b, 0x2b, 0x2b);
const QColor kControlBorder(0x42, 0x42, 0x42);
const QColor kControlStroke(0x4a, 0x4a, 0x4a);
const QColor kBorder(0x3d, 0x3d, 0x3d);
const QColor kSubtleBorder(0x33, 0x33, 0x33);
const QColor kRowHover(0x2f, 0x2f, 0x2f);
const QColor kTabHover(0x33, 0x33, 0x33);
const QColor kSelection(0x35, 0x35, 0x35);
const QColor kAccent(0xd7, 0xa0, 0x42);
const QColor kAccentHover(0xe2, 0xb2, 0x5e);
const QColor kAccentPressed(0xc0, 0x8b, 0x33);
const QColor kTextSelection(0x6b, 0x51, 0x22);
const QColor kToolBar(0x26, 0x26, 0x26);
const QColor kToolBarEdge(0x0e, 0x0e, 0x0e);
const QColor kCaptionHover(0x2d, 0x2d, 0x2d);
const QColor kCaptionPressed(0x29, 0x29, 0x29);
const QColor kCloseHover(0xc4, 0x2b, 0x1c);
const QColor kClosePressed(0xb2, 0x27, 0x1a);
const QColor kOnAccent(0x14, 0x14, 0x14);
const QColor kText(0xf0, 0xf0, 0xf0);
const QColor kTextSecondary(0xc5, 0xc5, 0xc5);
const QColor kTextDisabled(0x78, 0x78, 0x78);
const QColor kScrollThumb(0x6a, 0x6a, 0x6a);
const QColor kScrollThumbHover(0x9e, 0x9e, 0x9e);

constexpr int kShadowMargin = 12;
constexpr qreal kMenuRadius = 8;
constexpr qreal kControlRadius = 4;
constexpr qreal kCardRadius = 6;
constexpr int kMenuHMargin = 4;
constexpr int kMenuVMargin = 3;
constexpr const char *kMenuBarDecoratedProperty = "_nw_fluent_menubar";
constexpr const char *kMenuBarTitleProperty = "_nw_fluent_title";
constexpr const char *kMenuShiftedPosProperty = "_nw_fluent_shifted_pos";
constexpr const char *kFocusRingName = "_nw_fluent_focus_ring";
constexpr const char *kFramelessProperty = "_nw_fluent_frameless";
constexpr const char *kCaptionButtonsName = "_nw_fluent_caption";
constexpr int kTitleBarHeight = 29;
constexpr int kCaptionButtonWidth = 46;
constexpr int kDialogTitleHeight = 32;
constexpr const char *kDialogTitleName = "_nw_fluent_dialog_title";
constexpr const char *kDialogTitleIconName = "_nw_fluent_dialog_title_icon";
constexpr const char *kDialogTitleTextName = "_nw_fluent_dialog_title_text";

constexpr int kToolBarBand = 30;
constexpr int kToolBarButton = 24;
constexpr int kToolBarGap = 4;
constexpr int kToolBarGrip = 4;
constexpr int kTabFlare = 4;
constexpr int kToolBarTrailingGap = 6;
constexpr int kMenuBarIconGap = 7;
constexpr int kToolBarHandleExtent = 7;
constexpr int kToolBarHandleRight = 1 + kToolBarHandleExtent;
constexpr const char *kGripHotProperty = "_nw_fluent_grip_hot";
constexpr const char *kHoverTabProperty = "_nw_fluent_hover_tab";
constexpr const char *kToolBarConnectedProperty = "_nw_fluent_toolbar_connected";
const QColor kToolBarGripColor(0x13, 0x13, 0x13);

const QToolBar *DockedToolBarOf(const QWidget *w) {
    auto *toolBar = w != nullptr ? qobject_cast<const QToolBar*>(w->parentWidget()) : nullptr;
    return toolBar != nullptr && !toolBar->isWindow() ? toolBar : nullptr;
}

void ApplyToolBarOrientation(QToolBar *toolBar) {
    if (toolBar->isWindow())
        toolBar->setContentsMargins(0, 0, 0, 0);
    else if (toolBar->orientation() == Qt::Horizontal)
        toolBar->setContentsMargins(0, 0, kToolBarGap + 3, kToolBarTrailingGap);
    else
        toolBar->setContentsMargins(0, 0, kToolBarTrailingGap, kToolBarGap + 3);
    // QToolButton caches its size hint, and an orientation change doesn't reset it, so send a StyleChange to clear it.
    for (QToolButton *button : toolBar->findChildren<QToolButton*>(Qt::FindDirectChildrenOnly)) {
        QEvent styleChange(QEvent::StyleChange);
        QCoreApplication::sendEvent(button, &styleChange);
        button->updateGeometry();
    }
    if (toolBar->isWindow())
        toolBar->adjustSize();
}

bool IsFlushLeftTab(const QStyleOptionTab *tab) {
    return tab->rect.left() <= 0
        && (tab->position == QStyleOptionTab::Beginning || tab->position == QStyleOptionTab::OnlyOneTab);
}

QPainterPath PanePath(const QRectF &r, qreal radius) {
    QPainterPath pane;
    pane.moveTo(r.topLeft());
    pane.lineTo(r.right() - radius, r.top());
    pane.arcTo(QRectF(r.right() - radius * 2, r.top(), radius * 2, radius * 2), 90, -90);
    pane.lineTo(r.right(), r.bottom() - radius);
    pane.arcTo(QRectF(r.right() - radius * 2, r.bottom() - radius * 2, radius * 2, radius * 2), 0, -90);
    pane.lineTo(r.left() + radius, r.bottom());
    pane.arcTo(QRectF(r.left(), r.bottom() - radius * 2, radius * 2, radius * 2), 270, -90);
    pane.closeSubpath();
    return pane;
}

bool IsCentralWidget(const QWidget *w) {
    auto *window = w != nullptr ? qobject_cast<const QMainWindow*>(w->parentWidget()) : nullptr;
    return window != nullptr && window->centralWidget() == w;
}

QWidget *FocusCardOf(QWidget *w) {
    for (; w != nullptr; w = w->parentWidget()) {
        if (qobject_cast<QDockWidget*>(w) != nullptr || IsCentralWidget(w))
            return w;
    }
    return nullptr;
}

bool IsActiveCard(const QWidget *card) {
    return card != nullptr && FocusCardOf(QApplication::focusWidget()) == card;
}

void UpdateFocusRing(QWidget *card) {
    auto *ring = card->findChild<QWidget*>(kFocusRingName, Qt::FindDirectChildrenOnly);
    if (ring == nullptr)
        return;
    ring->setGeometry(card->rect());
    ring->setMask(QRegion(ring->rect()).subtracted(QRegion(ring->rect().adjusted(2, 2, -2, -2))));
    ring->show();
    ring->raise();
    ring->update();
}

void RefreshFocusVisuals(QWidget *w) {
    if (QWidget *card = FocusCardOf(w)) {
        UpdateFocusRing(card);
        if (auto *tabs = qobject_cast<QTabWidget*>(card)) {
            tabs->update();
            tabs->tabBar()->update();
        }
    }
}

void AddFocusRing(QWidget *card, QObject *filter) {
    if (card->focusPolicy() == Qt::NoFocus)
        card->setFocusPolicy(Qt::ClickFocus);
    else if (card->focusPolicy() == Qt::TabFocus)
        card->setFocusPolicy(Qt::StrongFocus);
    card->installEventFilter(filter);
    if (qobject_cast<QTabWidget*>(card) != nullptr || card->findChild<QWidget*>(kFocusRingName, Qt::FindDirectChildrenOnly) != nullptr)
        return;
    auto *ring = new QWidget(card);
    ring->setObjectName(kFocusRingName);
    ring->setAttribute(Qt::WA_TransparentForMouseEvents);
    ring->installEventFilter(filter);
    ring->hide();
}

bool HasShadow(const QWidget *w) {
    return qobject_cast<const QMenu*>(w) != nullptr && w->testAttribute(Qt::WA_TranslucentBackground);
}

bool IsShadowedComboPopup(const QWidget *w) {
    return w != nullptr && w->inherits("QComboBoxPrivateContainer") && w->testAttribute(Qt::WA_TranslucentBackground);
}

void DrawChevron(QPainter *p, const QRectF &box, Qt::ArrowType dir, const QColor &color, qreal size = 3.5) {
    const QPointF c = box.center();
    QPolygonF pts;
    switch (dir) {
    case Qt::UpArrow:    pts << c + QPointF(-size, size / 2) << c + QPointF(0, -size / 2) << c + QPointF(size, size / 2); break;
    case Qt::DownArrow:  pts << c + QPointF(-size, -size / 2) << c + QPointF(0, size / 2) << c + QPointF(size, -size / 2); break;
    case Qt::LeftArrow:  pts << c + QPointF(size / 2, -size) << c + QPointF(-size / 2, 0) << c + QPointF(size / 2, size); break;
    default:             pts << c + QPointF(-size / 2, -size) << c + QPointF(size / 2, 0) << c + QPointF(-size / 2, size); break;
    }
    p->save();
    p->setRenderHint(QPainter::Antialiasing);
    p->setPen(QPen(color, 1.3, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p->setBrush(Qt::NoBrush);
    p->drawPolyline(pts);
    p->restore();
}

void DrawTriangle(QPainter *p, const QRectF &box, Qt::ArrowType dir, const QColor &color, qreal size = 3) {
    const QPointF c = box.center();
    QPolygonF pts;
    switch (dir) {
    case Qt::UpArrow:    pts << c + QPointF(-size, size / 2) << c + QPointF(0, -size / 2) << c + QPointF(size, size / 2); break;
    case Qt::DownArrow:  pts << c + QPointF(-size, -size / 2) << c + QPointF(0, size / 2) << c + QPointF(size, -size / 2); break;
    case Qt::LeftArrow:  pts << c + QPointF(size / 2, -size) << c + QPointF(-size / 2, 0) << c + QPointF(size / 2, size); break;
    default:             pts << c + QPointF(-size / 2, -size) << c + QPointF(size / 2, 0) << c + QPointF(-size / 2, size); break;
    }
    p->save();
    p->setRenderHint(QPainter::Antialiasing);
    p->setPen(Qt::NoPen);
    p->setBrush(color);
    p->drawPolygon(pts);
    p->restore();
}

void DrawInputPanel(QPainter *p, const QRect &rect, const QColor &fill, bool focused) {
    p->save();
    p->setRenderHint(QPainter::Antialiasing);
    QPainterPath shape;
    shape.addRoundedRect(QRectF(rect), kControlRadius, kControlRadius);
    p->fillPath(shape, fill);
    const int stroke = focused ? 2 : 1;
    p->setClipRect(QRect(rect.left(), rect.bottom() + 1 - stroke, rect.width(), stroke));
    p->fillPath(shape, focused ? kAccent : kControlStroke);
    p->restore();
}

void DrawButtonPanel(QPainter *p, const QRect &rect, const QColor &fill, const QColor &border) {
    p->save();
    p->setRenderHint(QPainter::Antialiasing);
    p->setPen(QPen(border, 1));
    p->setBrush(fill);
    p->drawRoundedRect(QRectF(rect).adjusted(0.5, 0.5, -0.5, -0.5), kControlRadius, kControlRadius);
    p->restore();
}

void BlurAlpha(QImage &image, int radius) {
    const int width = image.width(), height = image.height();
    const int window = radius * 2 + 1;
    std::vector<int> line(static_cast<size_t>(qMax(width, height)));
    const auto blurLine = [&](int count, auto &&alphaAt) {
        for (int i = 0; i < count; i++)
            line[static_cast<size_t>(i)] = alphaAt(i);
        const auto at = [&](int i) { return i < 0 || i >= count ? 0 : line[static_cast<size_t>(i)]; };
        int sum = 0;
        for (int i = -radius; i <= radius; i++)
            sum += at(i);
        for (int i = 0; i < count; i++) {
            alphaAt(i) = static_cast<uchar>(sum / window);
            sum += at(i + radius + 1) - at(i - radius);
        }
    };
    for (int pass = 0; pass < 3; pass++) {
        for (int y = 0; y < height; y++) {
            uchar *row = image.scanLine(y);
            blurLine(width, [row](int x) -> uchar & { return row[x * 4 + 3]; });
        }
        for (int x = 0; x < width; x++)
            blurLine(height, [&image, x](int y) -> uchar & { return image.scanLine(y)[x * 4 + 3]; });
    }
}

void DrawMenuShadow(QPainter *p, const QRectF &panel, const QSize &size) {
    static QImage cached;
    const qreal dpr = p->device()->devicePixelRatioF();
    const QSize pixels = (QSizeF(size) * dpr).toSize();
    if (cached.size() != pixels || cached.devicePixelRatio() != dpr) {
        cached = QImage(pixels, QImage::Format_ARGB32_Premultiplied);
        cached.setDevicePixelRatio(dpr);
        cached.fill(Qt::transparent);
        QPainter painter(&cached);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(0, 0, 0, 64));
        painter.drawRoundedRect(panel.translated(0, 2), kMenuRadius, kMenuRadius);
        painter.end();
        BlurAlpha(cached, qRound(3 * dpr));
    }
    p->drawImage(QPointF(0, 0), cached);
}

bool IsAccentButton(const QStyleOption *opt, const QWidget *w) {
    const auto *btn = qstyleoption_cast<const QStyleOptionButton*>(opt);
    return btn != nullptr && (btn->features & QStyleOptionButton::DefaultButton) && (opt->state & QStyle::State_Enabled)
        && w != nullptr && qobject_cast<const QDialogButtonBox*>(w->parentWidget()) != nullptr;
}

QIcon GlyphIcon(QStyle::StandardPixmap sp) {
    QIcon icon;
    for (QIcon::Mode mode : { QIcon::Normal, QIcon::Active }) {
        for (int scale : { 1, 2 }) {
            QPixmap pix(16 * scale, 16 * scale);
            pix.setDevicePixelRatio(scale);
            pix.fill(Qt::transparent);
            QPainter p(&pix);
            p.setRenderHint(QPainter::Antialiasing);
            p.setPen(QPen(mode == QIcon::Active ? Qt::white : kTextSecondary, 1.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
            switch (sp) {
            case QStyle::SP_TitleBarMinButton:
                p.drawLine(QPointF(3.5, 8.5), QPointF(12.5, 8.5));
                break;
            case QStyle::SP_TitleBarMaxButton:
                p.drawRoundedRect(QRectF(3.5, 3.5, 9, 9), 1.5, 1.5);
                break;
            case QStyle::SP_TitleBarNormalButton:
                p.drawRoundedRect(QRectF(3.5, 5.5, 7, 7), 1.5, 1.5);
                p.drawPolyline(QPolygonF() << QPointF(5.5, 3.5) << QPointF(12.5, 3.5) << QPointF(12.5, 10.5));
                break;
            default:
                p.setRenderHint(QPainter::Antialiasing, false);
                p.drawLine(QPointF(3, 3), QPointF(12, 12));
                p.drawLine(QPointF(12, 3), QPointF(3, 12));
                break;
            }
            p.end();
            icon.addPixmap(pix, mode);
        }
    }
    return icon;
}

#if defined(Q_OS_WIN)
int FrameThickness(HWND hwnd) {
    const UINT dpi = GetDpiForWindow(hwnd);
    return GetSystemMetricsForDpi(SM_CYSIZEFRAME, dpi) + GetSystemMetricsForDpi(SM_CXPADDEDBORDER, dpi);
}

LRESULT TitleBarHitTest(QWidget *window, const QPoint &pos) {
    QWidget *child = window->childAt(pos);
    if (child == nullptr)
        return pos.y() < (qobject_cast<QDialog*>(window) != nullptr ? kDialogTitleHeight : kTitleBarHeight) ? HTCAPTION : HTCLIENT;
    if (child->objectName() == kDialogTitleIconName)
        return HTSYSMENU;
    if (child->objectName() == kDialogTitleName
        || (child->parentWidget() != nullptr && child->parentWidget()->objectName() == kDialogTitleName))
        return HTCAPTION;
    if (auto *menuBar = qobject_cast<QMenuBar*>(child)) {
        QAction *action = menuBar->actionAt(menuBar->mapFrom(window, pos));
        if (action != nullptr && !action->isEnabled() && action->text().isEmpty() && !action->icon().isNull())
            return HTSYSMENU;
        return action == nullptr || !action->isEnabled() ? HTCAPTION : HTCLIENT;
    }
    return HTCLIENT;
}

LRESULT ConstrainResizeHit(const QWidget *window, LRESULT hit) {
    const bool fixedWidth = window->minimumWidth() == window->maximumWidth();
    const bool fixedHeight = window->minimumHeight() == window->maximumHeight();
    const bool top = hit == HTTOPLEFT || hit == HTTOPRIGHT;
    const bool left = hit == HTTOPLEFT || hit == HTBOTTOMLEFT;
    switch (hit) {
    case HTLEFT:
    case HTRIGHT:
        return fixedWidth ? HTNOWHERE : hit;
    case HTTOP:
    case HTBOTTOM:
        return fixedHeight ? HTNOWHERE : hit;
    case HTTOPLEFT:
    case HTTOPRIGHT:
    case HTBOTTOMLEFT:
    case HTBOTTOMRIGHT:
        if (fixedWidth && fixedHeight)
            return HTNOWHERE;
        if (fixedWidth)
            return top ? HTTOP : HTBOTTOM;
        if (fixedHeight)
            return left ? HTLEFT : HTRIGHT;
        return hit;
    default:
        return hit;
    }
}

class FramelessFilter : public QAbstractNativeEventFilter {
public:
    bool nativeEventFilter(const QByteArray &type, void *message, qintptr *result) override {
        if (type != "windows_generic_MSG")
            return false;
        MSG *msg = static_cast<MSG*>(message);
        if (msg->message != WM_NCCALCSIZE && msg->message != WM_NCHITTEST)
            return false;
        QWidget *window = QWidget::find(reinterpret_cast<WId>(msg->hwnd));
        if (window == nullptr)
            return false;
        if (!window->isWindow()) {
            // Native child windows (Qt can create these, e.g. after a dock or toolbar floats) get the hit test first.
            // Pass it up wherever the window would answer caption, icon or resize edge.
            QWidget *top = window->window();
            if (msg->message != WM_NCHITTEST || !top->property(kFramelessProperty).toBool() || top->internalWinId() == 0)
                return false;
            const HWND topHwnd = reinterpret_cast<HWND>(top->internalWinId());
            POINT pt { GET_X_LPARAM(msg->lParam), GET_Y_LPARAM(msg->lParam) };
            ScreenToClient(topHwnd, &pt);
            const bool resizeEdge = !IsZoomed(topHwnd) && pt.y < FrameThickness(topHwnd)
                && (GetWindowLongW(topHwnd, GWL_STYLE) & WS_THICKFRAME) && top->minimumHeight() != top->maximumHeight();
            const qreal dpr = top->devicePixelRatioF();
            if (!resizeEdge && TitleBarHitTest(top, QPoint(qRound(pt.x / dpr), qRound(pt.y / dpr))) == HTCLIENT)
                return false;
            *result = HTTRANSPARENT;
            return true;
        }
        if (!window->property(kFramelessProperty).toBool())
            return false;

        if (msg->message == WM_NCCALCSIZE) {
            if (msg->wParam == FALSE)
                return false;
            // Windows keeps the side and bottom borders, so resizing and the DWM shadow still work. Only the
            // title bar becomes client area.
            auto *params = reinterpret_cast<NCCALCSIZE_PARAMS*>(msg->lParam);
            const LONG top = params->rgrc[0].top;
            DefWindowProcW(msg->hwnd, WM_NCCALCSIZE, msg->wParam, msg->lParam);
            params->rgrc[0].top = top + (IsZoomed(msg->hwnd) ? FrameThickness(msg->hwnd) : 0);
            *result = 0;
            return true;
        }

        const LRESULT native = DefWindowProcW(msg->hwnd, WM_NCHITTEST, msg->wParam, msg->lParam);
        if (native != HTCLIENT) {
            *result = ConstrainResizeHit(window, native);
            return true;
        }
        POINT pt { GET_X_LPARAM(msg->lParam), GET_Y_LPARAM(msg->lParam) };
        ScreenToClient(msg->hwnd, &pt);
        const int frame = FrameThickness(msg->hwnd);
        if (!IsZoomed(msg->hwnd) && pt.y < frame && (GetWindowLongW(msg->hwnd, GWL_STYLE) & WS_THICKFRAME)) {
            RECT rc;
            GetClientRect(msg->hwnd, &rc);
            const LRESULT edge = ConstrainResizeHit(window, pt.x < frame * 2 ? HTTOPLEFT : pt.x >= rc.right - frame * 2 ? HTTOPRIGHT : HTTOP);
            if (edge != HTNOWHERE) {
                *result = edge;
                return true;
            }
        }
        const qreal dpr = window->devicePixelRatioF();
        *result = TitleBarHitTest(window, QPoint(qRound(pt.x / dpr), qRound(pt.y / dpr)));
        return true;
    }
};

void UpdateWindowBorder(QWidget *window) {
    const QColor color = window->isActiveWindow() ? kAccent : kBorder;
    const COLORREF ref = RGB(static_cast<BYTE>(color.red()), static_cast<BYTE>(color.green()), static_cast<BYTE>(color.blue()));
    DwmSetWindowAttribute(reinterpret_cast<HWND>(window->winId()), DWMWA_BORDER_COLOR, &ref, sizeof(ref));
}

void ApplyFrameless(QWidget *window) {
    const HWND hwnd = reinterpret_cast<HWND>(window->winId());
    const DWORD corners = DWMWCP_ROUND;
    DwmSetWindowAttribute(hwnd, DWMWA_WINDOW_CORNER_PREFERENCE, &corners, sizeof(corners));
    SetWindowPos(hwnd, nullptr, 0, 0, 0, 0,
                 SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_NOACTIVATE);
    UpdateWindowBorder(window);
}
#endif

void UpdateDialogTitle(QWidget *window) {
    auto *title = window->findChild<QWidget*>(kDialogTitleName, Qt::FindDirectChildrenOnly);
    if (title == nullptr)
        return;
    if (auto *icon = title->findChild<QLabel*>(kDialogTitleIconName)) {
        QIcon windowIcon = window->windowIcon();
        if (windowIcon.isNull())
            windowIcon = QIcon(":/images/icon16_aa.png");
        icon->setPixmap(windowIcon.pixmap(16, 16));
    }
    if (auto *text = title->findChild<QLabel*>(kDialogTitleTextName))
        text->setText(window->windowTitle().isEmpty() ? QStringLiteral(NOOBWARRIOR_BRAND) : window->windowTitle());
}

void AddDialogTitle(QWidget *window) {
    auto *title = new QWidget(window);
    title->setObjectName(kDialogTitleName);
    auto *layout = new QHBoxLayout(title);
    layout->setContentsMargins(10, 0, 0, 0);
    layout->setSpacing(8);
    auto *icon = new QLabel(title);
    icon->setObjectName(kDialogTitleIconName);
    auto *text = new QLabel(title);
    text->setObjectName(kDialogTitleTextName);
    layout->addWidget(icon);
    layout->addWidget(text, 1);
    UpdateDialogTitle(window);
}

void LayoutCaptionButtons(QWidget *window) {
    auto *box = window->findChild<QWidget*>(kCaptionButtonsName, Qt::FindDirectChildrenOnly);
    if (box == nullptr)
        return;
    box->move(window->width() - box->width(), 0);
    box->raise();
    if (auto *max = box->findChild<QToolButton*>("max"))
        max->setIcon(GlyphIcon(window->isMaximized() ? QStyle::SP_TitleBarNormalButton : QStyle::SP_TitleBarMaxButton));
    if (auto *title = window->findChild<QWidget*>(kDialogTitleName, Qt::FindDirectChildrenOnly)) {
        title->setGeometry(0, 0, window->width() - box->width(), kDialogTitleHeight);
        title->raise();
    }
}

void AddCaptionButtons(QWidget *window, int height, bool minimize, bool maximize) {
    auto *box = new QWidget(window);
    box->setObjectName(kCaptionButtonsName);
    auto *layout = new QHBoxLayout(box);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    const std::pair<const char*, QStyle::StandardPixmap> buttons[] = {
        { "min", QStyle::SP_TitleBarMinButton },
        { "max", QStyle::SP_TitleBarMaxButton },
        { "close", QStyle::SP_TitleBarCloseButton },
    };
    for (const auto &[name, sp] : buttons) {
        if ((sp == QStyle::SP_TitleBarMinButton && !minimize) || (sp == QStyle::SP_TitleBarMaxButton && !maximize))
            continue;
        auto *button = new QToolButton(box);
        button->setObjectName(name);
        button->setAutoRaise(true);
        button->setFocusPolicy(Qt::NoFocus);
        button->setFixedSize(kCaptionButtonWidth, height);
        button->setIconSize(QSize(16, 16));
        button->setIcon(GlyphIcon(sp));
        layout->addWidget(button);
        QObject::connect(button, &QToolButton::clicked, window, [window, sp]() {
            if (sp == QStyle::SP_TitleBarMinButton)
                window->showMinimized();
            else if (sp == QStyle::SP_TitleBarMaxButton)
                window->isMaximized() ? window->showNormal() : window->showMaximized();
            else
                window->close();
        });
    }
    box->adjustSize();
    LayoutCaptionButtons(window);
}

bool IsTitleAction(const QStyleOptionMenuItem *opt, const QWidget *w) {
    auto *menuBar = qobject_cast<const QMenuBar*>(w);
    if (menuBar == nullptr)
        return false;
    QAction *action = menuBar->actionAt(opt->rect.center());
    return action != nullptr && action->property(kMenuBarTitleProperty).toBool();
}
}

FluentStyle::FluentStyle() : QProxyStyle(QStyleFactory::create("Fusion")) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
    QGuiApplication::styleHints()->setColorScheme(Qt::ColorScheme::Dark);
#endif
    connect(qApp, &QApplication::focusChanged, this, [](QWidget *old, QWidget *now) {
        RefreshFocusVisuals(old);
        RefreshFocusVisuals(now);
    });
#if defined(Q_OS_WIN)
    mFramelessFilter = std::make_unique<FramelessFilter>();
    qApp->installNativeEventFilter(mFramelessFilter.get());
#endif
}

FluentStyle::~FluentStyle() {
    if (mFramelessFilter != nullptr && qApp != nullptr)
        qApp->removeNativeEventFilter(mFramelessFilter.get());
}

QPalette FluentStyle::standardPalette() const {
    QPalette pal;
    for (auto group : { QPalette::Active, QPalette::Inactive, QPalette::Disabled }) {
        pal.setColor(group, QPalette::Window, kShell);
        pal.setColor(group, QPalette::WindowText, kText);
        pal.setColor(group, QPalette::Base, kPane);
        pal.setColor(group, QPalette::AlternateBase, QColor(0x2c, 0x2c, 0x2c));
        pal.setColor(group, QPalette::ToolTipBase, kMenu);
        pal.setColor(group, QPalette::ToolTipText, kText);
        pal.setColor(group, QPalette::PlaceholderText, QColor(0x8a, 0x8a, 0x8a));
        pal.setColor(group, QPalette::Text, kText);
        pal.setColor(group, QPalette::Button, kControl);
        pal.setColor(group, QPalette::ButtonText, kText);
        pal.setColor(group, QPalette::BrightText, Qt::white);
        pal.setColor(group, QPalette::Light, QColor(0x40, 0x40, 0x40));
        pal.setColor(group, QPalette::Midlight, QColor(0x38, 0x38, 0x38));
        pal.setColor(group, QPalette::Mid, kSubtleBorder);
        pal.setColor(group, QPalette::Dark, QColor(0x1a, 0x1a, 0x1a));
        pal.setColor(group, QPalette::Shadow, Qt::black);
        pal.setColor(group, QPalette::Highlight, kTextSelection);
        pal.setColor(group, QPalette::HighlightedText, Qt::white);
        pal.setColor(group, QPalette::Link, kAccent);
        pal.setColor(group, QPalette::LinkVisited, kAccentHover);
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
        pal.setColor(group, QPalette::Accent, kAccent);
#endif
    }
    pal.setColor(QPalette::Disabled, QPalette::WindowText, kTextDisabled);
    pal.setColor(QPalette::Disabled, QPalette::Text, kTextDisabled);
    pal.setColor(QPalette::Disabled, QPalette::ButtonText, kTextDisabled);
    pal.setColor(QPalette::Disabled, QPalette::Button, kControlDisabled);
    pal.setColor(QPalette::Disabled, QPalette::Highlight, kBorder);
    return pal;
}

void FluentStyle::polish(QPalette &pal) {
    pal = standardPalette();
}

void FluentStyle::polish(QWidget *widget) {
    QProxyStyle::polish(widget);

    QFont font = widget->font();
    font.setFamily("Source Sans Pro");
#if defined(_WIN32)
    font.setHintingPreference(QFont::PreferNoHinting);
    font.setStyleStrategy(QFont::PreferAntialias);
#endif
    widget->setFont(font);

    if (auto *menu = qobject_cast<QMenu*>(widget); menu != nullptr && menu->isWindow()) {
        menu->setWindowFlags(menu->windowFlags() | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
        menu->setAttribute(Qt::WA_TranslucentBackground);
        menu->installEventFilter(this);
    }

    if (auto *toolBar = qobject_cast<QToolBar*>(widget)) {
        ApplyToolBarOrientation(toolBar);
        toolBar->setAttribute(Qt::WA_Hover);
        toolBar->installEventFilter(this);
        if (!toolBar->property(kToolBarConnectedProperty).toBool()) {
            toolBar->setProperty(kToolBarConnectedProperty, true);
            const auto refresh = [toolBar]() {
                QMetaObject::invokeMethod(toolBar, [toolBar]() { ApplyToolBarOrientation(toolBar); }, Qt::QueuedConnection);
            };
            connect(toolBar, &QToolBar::orientationChanged, toolBar, refresh);
            connect(toolBar, &QToolBar::topLevelChanged, toolBar, refresh);
        }
    }

    if (qobject_cast<QTabBar*>(widget) != nullptr) {
        widget->setAttribute(Qt::WA_Hover);
        widget->installEventFilter(this);
    }

    if (auto *view = qobject_cast<QAbstractItemView*>(widget))
        view->viewport()->setAttribute(Qt::WA_Hover);

    if (widget->inherits("QComboBoxPrivateContainer") && widget->isWindow()) {
        widget->setWindowFlags(widget->windowFlags() | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
        widget->setAttribute(Qt::WA_TranslucentBackground);
        widget->setContentsMargins(kShadowMargin + kMenuHMargin, kShadowMargin + kMenuVMargin, kShadowMargin + kMenuHMargin, kShadowMargin + kMenuVMargin);
        widget->installEventFilter(this);
    }

#if !defined(Q_OS_MACOS)
    if (auto *menuBar = qobject_cast<QMenuBar*>(widget); menuBar != nullptr && !menuBar->property(kMenuBarDecoratedProperty).toBool()) {
        menuBar->setProperty(kMenuBarDecoratedProperty, true);

        auto *icon = new QAction(QIcon(":/images/icon16_aa.png"), "", menuBar);
        icon->setDisabled(true);
        icon->setMenuRole(QAction::NoRole);
        if (!menuBar->actions().isEmpty())
            menuBar->insertAction(menuBar->actions().at(0), icon);
        else menuBar->addAction(icon);

        auto *title = new QAction(menuBar->window()->windowTitle(), menuBar);
        title->setDisabled(true);
        title->setProperty(kMenuBarTitleProperty, true);
        menuBar->addAction(title);
    }
#endif

    if (auto *window = qobject_cast<QMainWindow*>(widget)) {
        if (window->contentsMargins().isNull())
            window->setContentsMargins(6, 0, 6, 6);
#if defined(Q_OS_WIN)
        if (window->isWindow() && window->menuWidget() != nullptr && !window->property(kFramelessProperty).toBool()) {
            window->setProperty(kFramelessProperty, true);
            window->installEventFilter(this);
            AddCaptionButtons(window, kTitleBarHeight, true, true);
            if (window->testAttribute(Qt::WA_WState_Created))
                ApplyFrameless(window);
        }
#endif
    }

#if defined(Q_OS_WIN)
    if (auto *dialog = qobject_cast<QDialog*>(widget); dialog != nullptr && dialog->isWindow()
        && !(dialog->windowFlags() & Qt::FramelessWindowHint) && !dialog->property(kFramelessProperty).toBool()) {
        dialog->setProperty(kFramelessProperty, true);
        dialog->installEventFilter(this);
        QMargins margins = dialog->contentsMargins();
        margins.setTop(margins.top() + kDialogTitleHeight);
        dialog->setContentsMargins(margins);
        AddDialogTitle(dialog);
        const Qt::WindowFlags flags = dialog->windowFlags();
        const bool minimize = (flags & Qt::WindowMinimizeButtonHint)
            || (dialog->parentWidget() == nullptr && qobject_cast<QMessageBox*>(dialog) == nullptr);
        AddCaptionButtons(dialog, kDialogTitleHeight, minimize, flags & Qt::WindowMaximizeButtonHint);
        if (dialog->testAttribute(Qt::WA_WState_Created))
            ApplyFrameless(dialog);
    }
#endif

    if (auto *dock = qobject_cast<QDockWidget*>(widget)) {
        QPalette pal = dock->palette();
        pal.setColor(QPalette::Window, kPane);
        dock->setPalette(pal);
        AddFocusRing(dock, this);
    } else if (IsCentralWidget(widget)) {
        AddFocusRing(widget, this);
    }

    if (qobject_cast<QTabWidget*>(widget->parentWidget()) != nullptr && qobject_cast<QTabBar*>(widget) == nullptr) {
        QPalette pal = widget->palette();
        pal.setColor(QPalette::Window, kPane);
        widget->setPalette(pal);
    }

    if (auto *frame = qobject_cast<QFrame*>(widget); frame != nullptr && qobject_cast<QAbstractScrollArea*>(widget) == nullptr) {
        const QFrame::Shape shape = frame->frameShape();
        if (shape == QFrame::StyledPanel || shape == QFrame::Panel || shape == QFrame::Box) {
            QPalette pal = frame->palette();
            pal.setColor(QPalette::Window, kPane);
            frame->setPalette(pal);
        }
    }
}

QIcon FluentStyle::standardIcon(StandardPixmap sp, const QStyleOption *opt, const QWidget *w) const {
    switch (sp) {
    case SP_TitleBarMinButton:
    case SP_TitleBarMaxButton:
    case SP_TitleBarNormalButton:
    case SP_TitleBarCloseButton:
    case SP_DockWidgetCloseButton:
        return GlyphIcon(sp);
    default:
        return QProxyStyle::standardIcon(sp, opt, w);
    }
}

void FluentStyle::unpolish(QWidget *widget) {
    widget->removeEventFilter(this);
    delete widget->findChild<QWidget*>(kFocusRingName, Qt::FindDirectChildrenOnly);
    QProxyStyle::unpolish(widget);
}

bool FluentStyle::eventFilter(QObject *obj, QEvent *event) {
    if (event->type() == QEvent::Show) {
        auto *menu = qobject_cast<QMenu*>(obj);
        if (menu != nullptr && HasShadow(menu) && menu->property(kMenuShiftedPosProperty).toPoint() != menu->pos()) {
            // Qt already places submenus beside their parent item, and PM_SubMenuOverlap accounts for the
            // shadow. Other menus get moved so the shadow margin sits outside the spot Qt picked.
            bool isSubmenu = false;
            for (QWidget *top : QApplication::topLevelWidgets()) {
                if (top != menu && top->isVisible() && qobject_cast<QMenu*>(top) != nullptr) {
                    isSubmenu = true;
                    break;
                }
            }
            if (!isSubmenu) {
                const QPoint cursor = QCursor::pos();
                const int dx = cursor.x() >= menu->x() + menu->width() - 2 ? kShadowMargin : -kShadowMargin;
                const int dy = cursor.y() >= menu->y() + menu->height() - 2 ? kShadowMargin : -kShadowMargin;
                menu->move(menu->pos() + QPoint(dx, dy));
            }
            menu->setProperty(kMenuShiftedPosProperty, menu->pos());
        }
    }

    if (auto *window = qobject_cast<QWidget*>(obj); window != nullptr && window->isWindow() && window->property(kFramelessProperty).toBool()) {
        switch (event->type()) {
#if defined(Q_OS_WIN)
        case QEvent::WinIdChange:
        case QEvent::Show:
            ApplyFrameless(window);
            break;
        case QEvent::ActivationChange:
            UpdateWindowBorder(window);
            break;
#endif
        case QEvent::Resize:
        case QEvent::WindowStateChange:
            LayoutCaptionButtons(window);
            break;
        case QEvent::WindowTitleChange:
        case QEvent::WindowIconChange:
            UpdateDialogTitle(window);
            break;
        default:
            break;
        }
    }

    if (event->type() == QEvent::Show && obj->inherits("QComboBoxPrivateContainer")) {
        // QComboBox overwrites the popup's palette with its own each time it opens.
        auto *popup = static_cast<QWidget*>(obj);
        QPalette pal = popup->palette();
        pal.setColor(QPalette::Base, kMenu);
        pal.setColor(QPalette::Window, kMenu);
        popup->setPalette(pal);
        if (auto *frame = qobject_cast<QFrame*>(popup))
            frame->setFrameShape(QFrame::NoFrame);
        if (auto *view = popup->findChild<QAbstractItemView*>())
            view->setFrameShape(QFrame::NoFrame);
        if (IsShadowedComboPopup(popup)) {
            QRect geometry = popup->geometry().adjusted(-kShadowMargin, 0, kShadowMargin, 0);
            if (QWidget *combo = popup->parentWidget(); combo != nullptr && combo->screen() != nullptr) {
                const QRect comboRect(combo->mapToGlobal(QPoint(0, 0)), combo->size());
                int top = comboRect.bottom() + 5 - kShadowMargin;
                if (top + geometry.height() - kShadowMargin > combo->screen()->availableGeometry().bottom())
                    top = comboRect.top() - 4 - geometry.height() + kShadowMargin;
                geometry.moveTop(top);
            }
            popup->setGeometry(geometry);
        }
    }

    if (auto *tabBar = qobject_cast<QTabBar*>(obj)) {
        const QEvent::Type type = event->type();
        if (type == QEvent::HoverEnter || type == QEvent::HoverMove || type == QEvent::HoverLeave) {
            const int index = type == QEvent::HoverLeave ? -1 : tabBar->tabAt(static_cast<QHoverEvent*>(event)->position().toPoint());
            const QVariant previous = tabBar->property(kHoverTabProperty);
            if (!previous.isValid() || previous.toInt() != index) {
                tabBar->setProperty(kHoverTabProperty, index);
                tabBar->update();
            }
        }
    }

    if (auto *toolBar = qobject_cast<QToolBar*>(obj)) {
        const QEvent::Type type = event->type();
        if (type == QEvent::HoverEnter || type == QEvent::HoverMove || type == QEvent::HoverLeave) {
            const bool hot = type != QEvent::HoverLeave && toolBar->isMovable() && !toolBar->isWindow()
                && (toolBar->orientation() == Qt::Horizontal ? static_cast<QHoverEvent*>(event)->position().x()
                                                              : static_cast<QHoverEvent*>(event)->position().y()) < kToolBarHandleRight;
            if (toolBar->property(kGripHotProperty).toBool() != hot) {
                toolBar->setProperty(kGripHotProperty, hot);
                toolBar->update();
            }
        }
    }

    if (event->type() == QEvent::Resize || event->type() == QEvent::ChildPolished) {
        if (auto *card = qobject_cast<QWidget*>(obj); card != nullptr && FocusCardOf(card) == card)
            UpdateFocusRing(card);
    } else if (event->type() == QEvent::Paint) {
        if (auto *ring = qobject_cast<QWidget*>(obj); ring != nullptr && ring->objectName() == kFocusRingName) {
            QPainter p(ring);
            p.setRenderHint(QPainter::Antialiasing);
            p.setPen(QPen(IsActiveCard(ring->parentWidget()) ? kAccent : kBorder, 1));
            p.setBrush(Qt::NoBrush);
            auto *dock = qobject_cast<QDockWidget*>(ring->parentWidget());
            const qreal radius = dock != nullptr && dock->isFloating() ? 0 : kCardRadius;
            p.drawRoundedRect(QRectF(ring->rect()).adjusted(0.5, 0.5, -0.5, -0.5), radius, radius);
            return true;
        }
        if (auto *dock = qobject_cast<QDockWidget*>(obj)) {
            QPainter p(dock);
            p.setRenderHint(QPainter::Antialiasing);
            p.setPen(Qt::NoPen);
            p.setBrush(kPane);
            const qreal radius = dock->isFloating() ? 0 : kCardRadius;
            p.drawRoundedRect(QRectF(dock->rect()), radius, radius);
        }
    }
    return QProxyStyle::eventFilter(obj, event);
}

void FluentStyle::drawPrimitive(PrimitiveElement pe, const QStyleOption *opt, QPainter *p, const QWidget *w) const {
    const bool enabled = opt->state & State_Enabled;
    const bool hover = enabled && (opt->state & State_MouseOver);
    const bool sunken = opt->state & State_Sunken;

    switch (pe) {
    case PE_PanelMenuBar:
    case PE_PanelToolBar:
    case PE_IndicatorToolBarHandle:
    case PE_FrameMenu:
    case PE_FrameFocusRect:
    case PE_FrameTabBarBase:
    case PE_FrameStatusBarItem:
    case PE_PanelScrollAreaCorner:
    case PE_IndicatorDockWidgetResizeHandle:
        return;

    case PE_PanelMenu: {
        p->save();
        p->setRenderHint(QPainter::Antialiasing);
        if (HasShadow(w) || IsShadowedComboPopup(w)) {
            const QRectF panel = QRectF(opt->rect).adjusted(kShadowMargin, kShadowMargin, -kShadowMargin, -kShadowMargin);
            DrawMenuShadow(p, panel, opt->rect.size());
            p->setPen(QPen(IsShadowedComboPopup(w) ? QColor(0, 0, 0, 80) : kMenuEdge, 1));
            p->setBrush(kMenu);
            p->drawRoundedRect(panel.adjusted(0.5, 0.5, -0.5, -0.5), kMenuRadius, kMenuRadius);
        } else {
            p->fillRect(opt->rect, kMenu);
            p->setPen(QPen(kMenuEdge, 1));
            p->setBrush(Qt::NoBrush);
            p->drawRect(QRectF(opt->rect).adjusted(0.5, 0.5, -0.5, -0.5));
        }
        p->restore();
        return;
    }

    case PE_PanelTipLabel:
        p->fillRect(opt->rect, kMenu);
        p->setPen(kBorder);
        p->drawRect(opt->rect.adjusted(0, 0, -1, -1));
        return;

    case PE_IndicatorToolBarSeparator: {
        const QRect r = opt->rect;
        if (opt->state & State_Horizontal)
            p->fillRect(QRect(r.center().x(), r.top() + (r.height() - 16) / 2, 1, 16), kBorder);
        else
            p->fillRect(QRect(r.left() + (r.width() - 16) / 2, r.center().y(), 16, 1), kBorder);
        return;
    }

    case PE_PanelButtonCommand: {
        const auto *btn = qstyleoption_cast<const QStyleOptionButton*>(opt);
        const bool isDefault = IsAccentButton(opt, w);
        const bool flat = btn != nullptr && (btn->features & QStyleOptionButton::Flat);
        const bool on = opt->state & State_On;
        if (flat) {
            if (hover || sunken || on)
                DrawButtonPanel(p, opt->rect, sunken ? kControlPressed : kRowHover, sunken ? kControlPressed : kRowHover);
            return;
        }
        if (isDefault) {
            DrawButtonPanel(p, opt->rect, sunken ? kAccentPressed : hover ? kAccentHover : kAccent, sunken ? kAccentPressed : kAccentHover);
        } else if (!enabled) {
            DrawButtonPanel(p, opt->rect, kControlDisabled, kControlDisabled);
        } else {
            const QColor fill = sunken ? kControlPressed : (hover || on) ? kControlHover : kControl;
            DrawButtonPanel(p, opt->rect, fill, kControlBorder);
        }
        return;
    }

    case PE_FrameDefaultButton:
        return;

    case PE_PanelButtonTool: {
        if (w != nullptr && w->parentWidget() != nullptr && w->parentWidget()->objectName() == kCaptionButtonsName) {
            const bool close = w->objectName() == "close";
            if (sunken || hover)
                p->fillRect(opt->rect, close ? (sunken ? kClosePressed : kCloseHover) : (sunken ? kCaptionPressed : kCaptionHover));
            return;
        }
        const bool on = opt->state & State_On;
        if (!(opt->state & State_AutoRaise)) {
            DrawButtonPanel(p, opt->rect, !enabled ? kControlDisabled : sunken ? kControlPressed : hover ? kControlHover : kControl,
                            enabled ? kControlBorder : kControlDisabled);
            return;
        }
        if (!hover && !sunken && !on)
            return;
        QRectF box = QRectF(opt->rect).adjusted(1, 1, -1, -1);
        if (const QToolBar *toolBar = DockedToolBarOf(w)) {
            if (toolBar->orientation() == Qt::Horizontal) {
                const qreal inset = (opt->rect.height() - kToolBarButton) / 2.0;
                box = QRectF(opt->rect).adjusted(0, inset, 0, -inset);
            } else {
                const qreal inset = (opt->rect.width() - kToolBarButton) / 2.0;
                box = QRectF(opt->rect).adjusted(inset, 0, -inset, 0);
            }
        }
        p->save();
        p->setRenderHint(QPainter::Antialiasing);
        p->setPen(Qt::NoPen);
        p->setBrush(sunken ? kControlPressed : hover ? kControlHover : kControl);
        p->drawRoundedRect(box, kControlRadius, kControlRadius);
        p->restore();
        return;
    }

    case PE_PanelLineEdit: {
        const auto *frame = qstyleoption_cast<const QStyleOptionFrame*>(opt);
        if (frame == nullptr || frame->lineWidth <= 0) {
            const QWidget *parent = w != nullptr ? w->parentWidget() : nullptr;
            if (qobject_cast<const QComboBox*>(parent) == nullptr && qobject_cast<const QAbstractSpinBox*>(parent) == nullptr)
                p->fillRect(opt->rect, opt->palette.base());
            return;
        }
        DrawInputPanel(p, opt->rect, enabled ? kControl : kControlDisabled, enabled && (opt->state & State_HasFocus));
        return;
    }

    case PE_FrameLineEdit:
        return;

    case PE_Frame: {
        if (w != nullptr && qobject_cast<const QAbstractScrollArea*>(w) == nullptr) {
            p->save();
            p->setRenderHint(QPainter::Antialiasing);
            QPainterPath card;
            card.addRoundedRect(QRectF(opt->rect).adjusted(0.5, 0.5, -0.5, -0.5), kCardRadius, kCardRadius);
            if (w->autoFillBackground()) {
                // Autofill paints a plain rectangle, so repaint the corners in the parent's colour.
                const QWidget *parent = w->parentWidget();
                QPainterPath corners;
                corners.addRect(QRectF(opt->rect));
                p->fillPath(corners.subtracted(card), parent != nullptr ? parent->palette().color(parent->backgroundRole()) : kShell);
            }
            p->setPen(QPen(kSubtleBorder, 1));
            p->setBrush(Qt::NoBrush);
            p->drawPath(card);
            p->restore();
            return;
        }
        p->setPen(kSubtleBorder);
        p->setBrush(Qt::NoBrush);
        p->drawRect(opt->rect.adjusted(0, 0, -1, -1));
        return;
    }

    case PE_FrameGroupBox: {
        p->save();
        p->setRenderHint(QPainter::Antialiasing);
        p->setPen(QPen(kBorder, 1));
        p->setBrush(Qt::NoBrush);
        p->drawRoundedRect(QRectF(opt->rect).adjusted(0.5, 0.5, -0.5, -0.5), kCardRadius, kCardRadius);
        p->restore();
        return;
    }

    case PE_FrameDockWidget:
        p->setPen(kBorder);
        p->setBrush(Qt::NoBrush);
        p->drawRect(opt->rect.adjusted(0, 0, -1, -1));
        return;

    case PE_FrameTabWidget: {
        p->save();
        p->setRenderHint(QPainter::Antialiasing);
        const auto *twf = qstyleoption_cast<const QStyleOptionTabWidgetFrame*>(opt);
        if (w != nullptr && twf != nullptr && (twf->shape == QTabBar::RoundedNorth || twf->shape == QTabBar::TriangularNorth)
            && opt->rect.top() > 0) {
            const QRectF strip(0, 0, w->width(), opt->rect.top() + 1);
            QPainterPath band;
            band.addRoundedRect(strip, kCardRadius, kCardRadius);
            QPainterPath bottom;
            bottom.addRect(strip.adjusted(0, kCardRadius, 0, 0));
            p->setPen(Qt::NoPen);
            p->setBrush(kToolBar);
            p->drawPath(band.united(bottom));
        }
        p->setPen(QPen(IsActiveCard(qobject_cast<const QTabWidget*>(w)) ? kAccent : kBorder, 1));
        p->setBrush(kPane);
        p->drawPath(PanePath(QRectF(opt->rect).adjusted(0.5, 0.5, -0.5, -0.5), kCardRadius));
        p->restore();
        return;
    }

    case PE_IndicatorCheckBox: {
        const bool on = opt->state & State_On;
        const bool partial = opt->state & State_NoChange;
        const QRectF r = QRectF(opt->rect).adjusted(0.5, 0.5, -0.5, -0.5);
        p->save();
        p->setRenderHint(QPainter::Antialiasing);
        const bool inMenu = qobject_cast<const QMenu*>(w) != nullptr;
        if (!inMenu) {
            if (on || partial) {
                p->setPen(Qt::NoPen);
                p->setBrush(!enabled ? kTextDisabled : sunken ? kAccentPressed : hover ? kAccentHover : kAccent);
            } else {
                p->setPen(QPen(!enabled ? kTextDisabled : hover ? kTextSecondary : QColor(0x8a, 0x8a, 0x8a), 1));
                p->setBrush(sunken ? kControlPressed : hover ? kControlHover : QColor(0x24, 0x24, 0x24));
            }
            p->drawRoundedRect(r, 3, 3);
        }
        const QColor mark = inMenu ? (enabled ? kText : kTextDisabled) : kOnAccent;
        p->setPen(QPen(mark, 1.6, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        p->setBrush(Qt::NoBrush);
        if (partial) {
            p->drawLine(QPointF(r.left() + r.width() * 0.28, r.center().y()), QPointF(r.right() - r.width() * 0.28, r.center().y()));
        } else if (on) {
            QPolygonF check;
            check << QPointF(r.left() + r.width() * 0.24, r.top() + r.height() * 0.52)
                  << QPointF(r.left() + r.width() * 0.43, r.top() + r.height() * 0.71)
                  << QPointF(r.left() + r.width() * 0.77, r.top() + r.height() * 0.32);
            p->drawPolyline(check);
        }
        p->restore();
        return;
    }

    case PE_IndicatorRadioButton: {
        const bool on = opt->state & State_On;
        const QRectF r = QRectF(opt->rect).adjusted(0.5, 0.5, -0.5, -0.5);
        p->save();
        p->setRenderHint(QPainter::Antialiasing);
        if (on) {
            p->setPen(Qt::NoPen);
            p->setBrush(!enabled ? kTextDisabled : hover ? kAccentHover : kAccent);
            p->drawEllipse(r);
            p->setBrush(kOnAccent);
            const qreal inset = r.width() * (sunken ? 0.34 : hover ? 0.26 : 0.3);
            p->drawEllipse(r.adjusted(inset, inset, -inset, -inset));
        } else {
            p->setPen(QPen(!enabled ? kTextDisabled : hover ? kTextSecondary : QColor(0x8a, 0x8a, 0x8a), 1));
            p->setBrush(sunken ? kControlPressed : hover ? kControlHover : QColor(0x24, 0x24, 0x24));
            p->drawEllipse(r);
        }
        p->restore();
        return;
    }

    case PE_IndicatorArrowUp:
    case PE_IndicatorArrowDown:
    case PE_IndicatorArrowLeft:
    case PE_IndicatorArrowRight: {
        const Qt::ArrowType dir = pe == PE_IndicatorArrowUp ? Qt::UpArrow
                                : pe == PE_IndicatorArrowDown ? Qt::DownArrow
                                : pe == PE_IndicatorArrowLeft ? Qt::LeftArrow : Qt::RightArrow;
        DrawChevron(p, opt->rect, dir, enabled ? kTextSecondary : kTextDisabled);
        return;
    }

    case PE_IndicatorTabClose: {
        const bool active = enabled && (opt->state & (State_Raised | State_MouseOver | State_Sunken));
        p->save();
        if (active) {
            p->setRenderHint(QPainter::Antialiasing);
            p->setPen(Qt::NoPen);
            p->setBrush(sunken ? kControlPressed : kControlHover);
            p->drawRoundedRect(QRectF(opt->rect), kControlRadius, kControlRadius);
        }
        p->setRenderHint(QPainter::Antialiasing, false);
        p->setPen(QPen(active ? kText : kTextSecondary, 1));
        const int size = 8;
        const int x = opt->rect.left() + (opt->rect.width() - size) / 2;
        const int y = opt->rect.top() + (opt->rect.height() - size) / 2;
        p->drawLine(x, y, x + size - 1, y + size - 1);
        p->drawLine(x + size - 1, y, x, y + size - 1);
        p->restore();
        return;
    }

    case PE_IndicatorBranch:
        if (opt->state & State_Children)
            DrawChevron(p, opt->rect, (opt->state & State_Open) ? Qt::DownArrow : Qt::RightArrow,
                        hover ? kText : kTextSecondary, 3);
        return;

    case PE_PanelItemViewRow:
    case PE_PanelItemViewItem: {
        const auto *vopt = qstyleoption_cast<const QStyleOptionViewItem*>(opt);
        const bool selected = opt->state & State_Selected;
        if (pe == PE_PanelItemViewRow && vopt != nullptr && (vopt->features & QStyleOptionViewItem::Alternate) && !selected)
            p->fillRect(opt->rect, opt->palette.alternateBase());
        if (pe == PE_PanelItemViewItem && vopt != nullptr && vopt->backgroundBrush.style() != Qt::NoBrush)
            p->fillRect(opt->rect, vopt->backgroundBrush);
        if (!selected && !(hover && pe == PE_PanelItemViewItem))
            return;

        const auto *list = qobject_cast<const QListView*>(w);
        if (list != nullptr && list->viewMode() == QListView::IconMode) {
            p->save();
            p->setRenderHint(QPainter::Antialiasing);
            p->setPen(Qt::NoPen);
            p->setBrush(selected ? kSelection : kRowHover);
            p->drawRoundedRect(QRectF(opt->rect), kControlRadius, kControlRadius);
            p->restore();
            return;
        }
        p->fillRect(opt->rect, selected ? kSelection : kRowHover);
        if (selected && qobject_cast<const QAbstractItemView*>(w) != nullptr && opt->rect.left() <= 0) {
            p->save();
            p->setRenderHint(QPainter::Antialiasing);
            p->setPen(Qt::NoPen);
            p->setBrush(kAccent);
            const QRectF r(opt->rect);
            p->drawRoundedRect(QRectF(r.left() + 1, r.top() + r.height() * 0.22, 3, r.height() * 0.56), 1.5, 1.5);
            p->restore();
        }
        return;
    }

    default:
        break;
    }
    QProxyStyle::drawPrimitive(pe, opt, p, w);
}

void FluentStyle::drawControl(ControlElement ce, const QStyleOption *opt, QPainter *p, const QWidget *w) const {
    const bool enabled = opt->state & State_Enabled;
    const bool hover = enabled && (opt->state & State_MouseOver);

    switch (ce) {
    case CE_ToolBar: {
        const auto *toolBar = qobject_cast<const QToolBar*>(w);
        const bool floating = w != nullptr && w->isWindow();
        const bool horizontal = toolBar == nullptr || toolBar->orientation() == Qt::Horizontal;
        QRectF band(opt->rect);
        if (!floating && toolBar != nullptr) {
            int contentEnd = 0;
            for (QObject *child : toolBar->children()) {
                if (auto *item = qobject_cast<QWidget*>(child); item != nullptr && !item->isWindow() && item->isVisible())
                    contentEnd = qMax(contentEnd, horizontal ? item->geometry().right() : item->geometry().bottom());
            }
            if (horizontal) {
                band.adjust(0, 0, -kToolBarGap, -kToolBarTrailingGap);
                const qreal inset = (band.height() - kToolBarBand) / 2.0;
                band.adjust(0, inset, 0, -inset);
                if (contentEnd > 0)
                    band.setRight(contentEnd + 4);
            } else {
                band.adjust(0, 0, -kToolBarTrailingGap, -kToolBarGap);
                const qreal inset = (band.width() - kToolBarBand) / 2.0;
                band.adjust(inset, 0, -inset, 0);
                if (contentEnd > 0)
                    band.setBottom(contentEnd + 4);
            }
        }
        const qreal radius = floating ? 0 : kControlRadius;
        QPainterPath shape;
        shape.addRoundedRect(band.adjusted(0.5, 0.5, -0.5, -0.5), radius, radius);
        p->save();
        p->setRenderHint(QPainter::Antialiasing);
        p->fillPath(shape, kToolBar);
        if (toolBar != nullptr && toolBar->isMovable() && !floating) {
            p->save();
            p->setClipPath(shape);
            const QRectF grip = horizontal ? QRectF(band.left(), band.top(), kToolBarGrip, band.height())
                                           : QRectF(band.left(), band.top(), band.width(), kToolBarGrip);
            p->fillRect(grip, toolBar->property(kGripHotProperty).toBool() ? kAccent : kToolBarGripColor);
            p->restore();
        }
        p->setPen(QPen(kToolBarEdge, 1));
        p->setBrush(Qt::NoBrush);
        p->drawPath(shape);
        p->restore();
        return;
    }

    case CE_MenuEmptyArea:
    case CE_Splitter:
        return;

    case CE_MenuBarEmptyArea:
        return;

#if !defined(Q_OS_MACOS)
    case CE_MenuBarItem: {
        const auto *mbi = qstyleoption_cast<const QStyleOptionMenuItem*>(opt);
        if (mbi == nullptr)
            break;
        const bool active = enabled && (mbi->state & (State_Selected | State_Sunken));
        const QRectF r = QRectF(mbi->rect).adjusted(1, 4, -1, -4);
        p->save();
        p->setRenderHint(QPainter::Antialiasing);
        if (IsTitleAction(mbi, w)) {
            p->setPen(kTextSecondary);
            p->drawText(mbi->rect, Qt::AlignCenter, mbi->text);
            p->restore();
            return;
        }
        if (active) {
            p->setPen(Qt::NoPen);
            p->setBrush(kMenuBarHover);
            p->drawRoundedRect(r, kControlRadius, kControlRadius);
        }
        if (!mbi->icon.isNull()) {
            drawItemPixmap(p, mbi->rect, Qt::AlignLeft | Qt::AlignVCenter, mbi->icon.pixmap(16, 16));
        } else {
            int flags = Qt::AlignCenter | Qt::TextShowMnemonic | Qt::TextDontClip | Qt::TextSingleLine;
            if (!proxy()->styleHint(SH_UnderlineShortcut, mbi, w))
                flags |= Qt::TextHideMnemonic;
            p->setPen(enabled ? kText : kTextDisabled);
            p->drawText(mbi->rect, flags, mbi->text);
        }
        p->restore();
        return;
    }
#endif

    case CE_MenuItem: {
        const auto *mi = qstyleoption_cast<const QStyleOptionMenuItem*>(opt);
        if (mi == nullptr)
            break;
        if (qobject_cast<const QComboBox*>(w) != nullptr)
            p->fillRect(mi->rect, kMenu);
        if (mi->menuItemType == QStyleOptionMenuItem::Separator && mi->text.isEmpty()) {
            p->fillRect(QRect(mi->rect.left() + 4, mi->rect.center().y(), mi->rect.width() - 8, 1), kBorder);
            return;
        }
        const bool selected = enabled && (mi->state & State_Selected);
        if (selected) {
            p->save();
            p->setRenderHint(QPainter::Antialiasing);
            p->setPen(Qt::NoPen);
            p->setBrush(kMenuHighlight);
            p->drawRoundedRect(QRectF(mi->rect).adjusted(0, 1, 0, -1), kControlRadius, kControlRadius);
            p->restore();
        }
        QStyleOptionMenuItem copy(*mi);
        copy.palette.setColor(QPalette::Highlight, Qt::transparent);
        copy.palette.setColor(QPalette::HighlightedText, kText);
        QProxyStyle::drawControl(ce, &copy, p, w);
        return;
    }

    case CE_TabBarTabShape: {
        const auto *tab = qstyleoption_cast<const QStyleOptionTab*>(opt);
        if (tab == nullptr)
            break;
        const bool selected = tab->state & State_Selected;
        const bool vertical = tab->shape == QTabBar::RoundedWest || tab->shape == QTabBar::RoundedEast
                           || tab->shape == QTabBar::TriangularWest || tab->shape == QTabBar::TriangularEast;
        const bool north = tab->shape == QTabBar::RoundedNorth || tab->shape == QTabBar::TriangularNorth;
        const QRectF r = vertical ? QRectF(tab->rect).adjusted(3, 2, -3, -2) : QRectF(tab->rect).adjusted(2, 3, -2, -3);
        p->save();
        p->setRenderHint(QPainter::Antialiasing);
        if (selected && north) {
            const QWidget *tabs = w != nullptr ? w->parentWidget() : nullptr;
            const QColor line = qobject_cast<const QTabWidget*>(tabs) != nullptr && IsActiveCard(tabs) ? kAccent : kBorder;
            const qreal flare = kTabFlare;
            const qreal radius = kCardRadius;
            const bool flushLeft = IsFlushLeftTab(tab);
            const qreal left = tab->rect.left() + 0.5, right = tab->rect.right() + 0.5;
            const qreal top = tab->rect.top() + 0.5, bottom = tab->rect.bottom() + 0.5;
            QPainterPath edge;
            if (flushLeft) {
                edge.moveTo(left, bottom);
            } else {
                edge.moveTo(left - flare, bottom);
                edge.arcTo(QRectF(left - flare * 2, bottom - flare * 2, flare * 2, flare * 2), 270, 90);
            }
            edge.lineTo(left, top + radius);
            edge.arcTo(QRectF(left, top, radius * 2, radius * 2), 180, -90);
            edge.lineTo(right - radius, top);
            edge.arcTo(QRectF(right - radius * 2, top, radius * 2, radius * 2), 90, -90);
            edge.lineTo(right, bottom - flare);
            edge.arcTo(QRectF(right, bottom - flare * 2, flare * 2, flare * 2), 180, 90);
            QPainterPath fill = edge;
            fill.lineTo(right + flare, bottom + 0.5);
            fill.lineTo(flushLeft ? left : left - flare, bottom + 0.5);
            fill.closeSubpath();
            p->setPen(Qt::NoPen);
            p->setBrush(kPane);
            p->drawPath(fill);
            p->setPen(QPen(line, 1));
            p->setBrush(Qt::NoBrush);
            p->drawPath(edge);
        } else if (selected) {
            p->setPen(QPen(kAccent, 1));
            p->setBrush(QColor(0x2e, 0x2e, 0x2e));
            p->drawRoundedRect(r.adjusted(0.5, 0.5, -0.5, -0.5), kControlRadius, kControlRadius);
        } else if (hover) {
            p->setPen(Qt::NoPen);
            p->setBrush(kRowHover);
            if (north) {
                const QRectF box = QRectF(tab->rect).adjusted(0, 0, 1, -1);
                QPainterPath shape;
                shape.addRoundedRect(box, kCardRadius, kCardRadius);
                QPainterPath bottom;
                bottom.addRect(box.adjusted(0, kCardRadius, 0, 0));
                p->setBrush(kTabHover);
                p->drawPath(shape.united(bottom));
            } else {
                p->drawRoundedRect(r, kControlRadius, kControlRadius);
            }
        }
        p->restore();
        return;
    }

    case CE_TabBarTabLabel: {
        const auto *tab = qstyleoption_cast<const QStyleOptionTab*>(opt);
        if (tab == nullptr)
            break;
        QStyleOptionTab copy(*tab);
        const QColor color = !enabled ? kTextDisabled : (tab->state & State_Selected) || hover ? kText : kTextSecondary;
        copy.palette.setColor(QPalette::WindowText, color);
        copy.palette.setColor(QPalette::ButtonText, color);
        p->save();
        if (tab->state & State_Selected) {
            QFont font = p->font();
            font.setBold(true);
            p->setFont(font);
        }
        QProxyStyle::drawControl(ce, &copy, p, w);
        p->restore();
        return;
    }

    case CE_DockWidgetTitle: {
        const auto *dw = qstyleoption_cast<const QStyleOptionDockWidget*>(opt);
        if (dw == nullptr || dw->verticalTitleBar)
            break;
        const QRect r = dw->rect.adjusted(8, 0, -4, 0);
        const QString text = dw->fontMetrics.elidedText(dw->title, Qt::ElideRight, r.width());
        p->setPen(enabled ? kText : kTextDisabled);
        p->drawText(r, Qt::AlignLeft | Qt::AlignVCenter | Qt::TextShowMnemonic | Qt::TextHideMnemonic, text);
        return;
    }

    case CE_HeaderSection: {
        const QRect r = opt->rect;
        p->fillRect(r, hover ? kRowHover : kPane);
        p->fillRect(QRect(r.left(), r.bottom(), r.width(), 1), kBorder);
        p->fillRect(QRect(r.right(), r.top() + 4, 1, r.height() - 8), kBorder);
        return;
    }

    case CE_HeaderEmptyArea:
        p->fillRect(opt->rect, kPane);
        p->fillRect(QRect(opt->rect.left(), opt->rect.bottom(), opt->rect.width(), 1), kBorder);
        return;

    case CE_ShapedFrame: {
        const auto *frame = qstyleoption_cast<const QStyleOptionFrame*>(opt);
        if (frame != nullptr && (frame->frameShape == QFrame::HLine || frame->frameShape == QFrame::VLine)) {
            const QRect r = frame->rect;
            if (frame->frameShape == QFrame::HLine)
                p->fillRect(QRect(r.left(), r.center().y(), r.width(), 1), kBorder);
            else
                p->fillRect(QRect(r.center().x(), r.top(), 1, r.height()), kBorder);
            return;
        }
        break;
    }

    case CE_ProgressBarGroove:
        p->save();
        p->setRenderHint(QPainter::Antialiasing);
        p->setPen(Qt::NoPen);
        p->setBrush(kControl);
        p->drawRoundedRect(QRectF(opt->rect), kControlRadius, kControlRadius);
        p->restore();
        return;

    case CE_ProgressBarContents: {
        const auto *bar = qstyleoption_cast<const QStyleOptionProgressBar*>(opt);
        if (bar == nullptr || bar->minimum >= bar->maximum)
            break;
        const qreal progress = qBound(0.0, qreal(bar->progress - bar->minimum) / (bar->maximum - bar->minimum), 1.0);
        if (progress <= 0)
            return;
        QRectF r = QRectF(bar->rect).adjusted(1, 1, -1, -1);
        if (bar->state & State_Horizontal) {
            const qreal width = r.width() * progress;
            if (bar->invertedAppearance)
                r.setLeft(r.right() - width);
            else
                r.setWidth(width);
        } else {
            const qreal height = r.height() * progress;
            if (bar->invertedAppearance)
                r.setHeight(height);
            else
                r.setTop(r.bottom() - height);
        }
        p->save();
        p->setRenderHint(QPainter::Antialiasing);
        p->setPen(Qt::NoPen);
        p->setBrush(enabled ? kAccent : kTextDisabled);
        p->drawRoundedRect(r, kControlRadius - 1, kControlRadius - 1);
        p->restore();
        return;
    }

    case CE_PushButtonLabel: {
        const auto *btn = qstyleoption_cast<const QStyleOptionButton*>(opt);
        if (btn != nullptr && IsAccentButton(opt, w)) {
            QStyleOptionButton copy(*btn);
            copy.palette.setColor(QPalette::ButtonText, kOnAccent);
            QProxyStyle::drawControl(ce, &copy, p, w);
            return;
        }
        break;
    }

    default:
        break;
    }
    QProxyStyle::drawControl(ce, opt, p, w);
}

void FluentStyle::drawComplexControl(ComplexControl cc, const QStyleOptionComplex *opt, QPainter *p, const QWidget *w) const {
    const bool enabled = opt->state & State_Enabled;
    const bool hover = enabled && (opt->state & State_MouseOver);

    switch (cc) {
    case CC_ComboBox: {
        const auto *cb = qstyleoption_cast<const QStyleOptionComboBox*>(opt);
        if (cb == nullptr)
            break;
        if (cb->frame) {
            const bool open = cb->state & State_On;
            QColor fill = !enabled ? kControlDisabled : (!cb->editable && (hover || open)) ? kControlHover : kControl;
            DrawInputPanel(p, cb->rect, fill, enabled && cb->editable && (cb->state & State_HasFocus));
        }
        if (cb->subControls & SC_ComboBoxArrow) {
            const QRect arrow = proxy()->subControlRect(CC_ComboBox, cb, SC_ComboBoxArrow, w);
            DrawChevron(p, arrow, Qt::DownArrow, enabled ? kTextSecondary : kTextDisabled);
        }
        return;
    }

    case CC_SpinBox: {
        const auto *sb = qstyleoption_cast<const QStyleOptionSpinBox*>(opt);
        if (sb == nullptr)
            break;
        if (sb->frame)
            DrawInputPanel(p, sb->rect, enabled ? kControl : kControlDisabled, enabled && (sb->state & State_HasFocus));
        if (sb->buttonSymbols == QAbstractSpinBox::NoButtons)
            return;
        for (SubControl sc : { SC_SpinBoxUp, SC_SpinBoxDown }) {
            const QRect r = proxy()->subControlRect(CC_SpinBox, sb, sc, w);
            const bool stepEnabled = enabled && (sb->stepEnabled & (sc == SC_SpinBoxUp ? QAbstractSpinBox::StepUpEnabled : QAbstractSpinBox::StepDownEnabled));
            const bool active = stepEnabled && (sb->activeSubControls & sc);
            if (active) {
                p->save();
                p->setRenderHint(QPainter::Antialiasing);
                p->setPen(Qt::NoPen);
                p->setBrush((sb->state & State_Sunken) ? kControlPressed : kControlHover);
                p->drawRoundedRect(QRectF(r).adjusted(1, 1, -1, -1), 3, 3);
                p->restore();
            }
            if (sb->buttonSymbols == QAbstractSpinBox::PlusMinus) {
                p->save();
                p->setPen(QPen(stepEnabled ? kTextSecondary : kTextDisabled, 1.3));
                const QPointF c = QRectF(r).center();
                p->drawLine(c - QPointF(3, 0), c + QPointF(3, 0));
                if (sc == SC_SpinBoxUp)
                    p->drawLine(c - QPointF(0, 3), c + QPointF(0, 3));
                p->restore();
            } else {
                DrawChevron(p, r, sc == SC_SpinBoxUp ? Qt::UpArrow : Qt::DownArrow, stepEnabled ? kTextSecondary : kTextDisabled, 3);
            }
        }
        return;
    }

    case CC_ScrollBar: {
        const auto *sb = qstyleoption_cast<const QStyleOptionSlider*>(opt);
        if (sb == nullptr)
            break;
        const bool horizontal = sb->orientation == Qt::Horizontal;
        const bool dragging = (sb->state & State_Sunken) && (sb->activeSubControls & SC_ScrollBarSlider);
        if (sb->subControls & SC_ScrollBarSlider) {
            const QRect slider = proxy()->subControlRect(CC_ScrollBar, sb, SC_ScrollBarSlider, w);
            if (slider.isValid() && sb->maximum > sb->minimum) {
                const qreal thickness = hover || dragging ? 8 : 4;
                QRectF r(slider);
                if (horizontal)
                    r = QRectF(r.left() + 1, r.center().y() - thickness / 2 + 0.5, r.width() - 2, thickness);
                else
                    r = QRectF(r.center().x() - thickness / 2 + 0.5, r.top() + 1, thickness, r.height() - 2);
                p->save();
                p->setRenderHint(QPainter::Antialiasing);
                p->setPen(Qt::NoPen);
                p->setBrush(dragging || hover ? kScrollThumbHover : kScrollThumb);
                p->drawRoundedRect(r, thickness / 2, thickness / 2);
                p->restore();
            }
        }
        const QColor arrowColor = !enabled ? kTextDisabled : hover ? kTextSecondary : kScrollThumb;
        if (sb->subControls & SC_ScrollBarSubLine)
            DrawTriangle(p, proxy()->subControlRect(CC_ScrollBar, sb, SC_ScrollBarSubLine, w), horizontal ? Qt::LeftArrow : Qt::UpArrow, arrowColor);
        if (sb->subControls & SC_ScrollBarAddLine)
            DrawTriangle(p, proxy()->subControlRect(CC_ScrollBar, sb, SC_ScrollBarAddLine, w), horizontal ? Qt::RightArrow : Qt::DownArrow, arrowColor);
        return;
    }

    default:
        break;
    }
    QProxyStyle::drawComplexControl(cc, opt, p, w);
}

QSize FluentStyle::sizeFromContents(ContentsType ct, const QStyleOption *opt, const QSize &size, const QWidget *w) const {
    QSize s = QProxyStyle::sizeFromContents(ct, opt, size, w);
    switch (ct) {
    case CT_MenuItem:
        if (const auto *mi = qstyleoption_cast<const QStyleOptionMenuItem*>(opt)) {
            if (mi->menuItemType == QStyleOptionMenuItem::Separator)
                s.setHeight(7);
            else
                s.setHeight(24);
        }
        break;
    case CT_MenuBarItem:
        if (const auto *mbi = qstyleoption_cast<const QStyleOptionMenuItem*>(opt); mbi != nullptr && mbi->text.isEmpty() && !mbi->icon.isNull())
            s = QSize(16 + kMenuBarIconGap, kTitleBarHeight);
        else
            s = QSize(s.width() + 8, kTitleBarHeight);
        break;
    case CT_ToolButton:
        if (const QToolBar *toolBar = DockedToolBarOf(w)) {
            if (toolBar->orientation() == Qt::Horizontal)
                s = QSize(qMax(s.width(), kToolBarButton), kToolBarBand);
            else
                s = QSize(kToolBarBand, qMax(s.height(), kToolBarButton));
        }
        break;
    case CT_TabBarTab:
        if (const auto *tab = qstyleoption_cast<const QStyleOptionTab*>(opt); tab != nullptr && w != nullptr) {
            QFont bold = w->font();
            bold.setBold(true);
            s.rwidth() += QFontMetrics(bold).horizontalAdvance(tab->text) - tab->fontMetrics.horizontalAdvance(tab->text);
        }
        break;
    default:
        break;
    }
    return s;
}

int FluentStyle::pixelMetric(PixelMetric metric, const QStyleOption *option, const QWidget *widget) const {
    switch (metric) {
    case PM_MenuPanelWidth: return HasShadow(widget) ? kShadowMargin + 1 : 1;
    case PM_MenuHMargin: return kMenuHMargin;
    case PM_MenuVMargin: return kMenuVMargin;
    case PM_SubMenuOverlap: return HasShadow(widget) ? kMenuHMargin + 2 - kShadowMargin : -1;
    case PM_MenuBarVMargin: return 0;
    case PM_MenuBarPanelWidth: return 0;
    case PM_MenuBarHMargin: return 0;
    case PM_MenuBarItemSpacing: return 0;
    case PM_MessageBoxIconSize: return 32;
    case PM_DockWidgetSeparatorExtent: return 6;
    case PM_DockWidgetTitleMargin: return 6;
    case PM_DockWidgetFrameWidth: return 0;
    case PM_SplitterWidth: return 6;
    case PM_ScrollBarExtent: return 14;
    case PM_ScrollBarSliderMin: return 24;
    case PM_TabBarTabHSpace: return 20;
    case PM_TabBarTabVSpace: return 10;
    case PM_TabBarBaseOverlap: return 1;
    case PM_TabBarTabShiftHorizontal:
    case PM_TabBarTabShiftVertical: return 0;
    case PM_ButtonShiftHorizontal:
    case PM_ButtonShiftVertical: return 0;
    case PM_IndicatorWidth:
    case PM_IndicatorHeight:
    case PM_ExclusiveIndicatorWidth:
    case PM_ExclusiveIndicatorHeight: return 16;
    case PM_ToolBarItemSpacing: return 1;
    case PM_ToolBarHandleExtent: return kToolBarHandleExtent;
    case PM_ToolBarSeparatorExtent: return 9;
    case PM_ToolBarIconSize: return 16;
    case PM_ToolBarFrameWidth: return 0;
    case PM_ToolBarItemMargin: return 1;
    default: return QProxyStyle::pixelMetric(metric, option, widget);
    }
}

int FluentStyle::styleHint(StyleHint hint, const QStyleOption *opt, const QWidget *w, QStyleHintReturn *ret) const {
    switch (hint) {
    case SH_Menu_Mask: return 0;
    case SH_ItemView_ShowDecorationSelected: return 1;
    case SH_DockWidget_ButtonsHaveFrame: return 1;
    case SH_UnderlineShortcut: return 0;
    case SH_MenuBar_MouseTracking: return 1;
    case SH_EtchDisabledText: return 0;
    case SH_ToolTipLabel_Opacity: return 255;
    default: return QProxyStyle::styleHint(hint, opt, w, ret);
    }
}

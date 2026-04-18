/*
    SPDX-FileCopyrightText: 2026 OpenAI

    SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "workspacesession.h"

#include "fitsviewer/fitsdata.h"
#include "fitsviewer/fitsview.h"

#include <QPointer>
#include <QScrollBar>

#include <algorithm>

namespace
{

void restoreViewport(FITSView *view, const Ekos::WorkspaceSession::ViewportState &viewport)
{
    if (view == nullptr || view->imageData().isNull())
        return;

    constexpr auto MAX_ZOOM_STEPS = 64;
    for (int step = 0; step < MAX_ZOOM_STEPS && view->getCurrentZoom() + 0.01 < viewport.zoom; ++step)
        view->ZoomIn();

    for (int step = 0; step < MAX_ZOOM_STEPS && view->getCurrentZoom() - 0.01 > viewport.zoom; ++step)
        view->ZoomOut();

    if (viewport.valid == false)
        return;

    view->horizontalScrollBar()->setValue(std::clamp(viewport.scrollPosition.x(), 0, view->horizontalScrollBar()->maximum()));
    view->verticalScrollBar()->setValue(std::clamp(viewport.scrollPosition.y(), 0, view->verticalScrollBar()->maximum()));
}

}

namespace Ekos
{

WorkspaceSession::WorkspaceSession(QObject *parent) : QObject(parent)
{
}

void WorkspaceSession::activateSource(Source source)
{
    if (m_activeSource == source)
        return;

    m_activeSource = source;
    Q_EMIT activeSourceChanged(source);
}

std::optional<WorkspaceSession::Source> WorkspaceSession::activeSource() const
{
    return m_activeSource;
}

void WorkspaceSession::publishFrame(Source source, const QSharedPointer<FITSData> &data)
{
    auto &state = m_sourceStates[sourceIndex(source)];
    if (state.frame == data)
        return;

    state.frame = data;
    Q_EMIT sourceUpdated(source);
}

void WorkspaceSession::publishView(Source source, const QSharedPointer<FITSView> &view)
{
    if (view.isNull())
        return;

    publishFrame(source, view->imageData());
    captureViewport(source, view.get());
}

void WorkspaceSession::captureViewport(Source source, const FITSView *view)
{
    if (view == nullptr)
        return;

    auto &state = m_sourceStates[sourceIndex(source)];
    const ViewportState viewport
    {
        view->getCurrentZoom(),
        QPoint(view->horizontalScrollBar()->value(), view->verticalScrollBar()->value()),
        view->imageData().isNull() == false,
    };

    if (state.viewport == viewport)
        return;

    state.viewport = viewport;
    Q_EMIT sourceUpdated(source);
}

void WorkspaceSession::applyTo(Source source, FITSView *view) const
{
    if (view == nullptr)
        return;

    const auto &state = m_sourceStates[sourceIndex(source)];
    if (state.frame.isNull())
        return;

    const auto viewport = state.viewport;
    auto restore = [guard = QPointer<FITSView>(view), viewport]()
    {
        if (guard == nullptr)
            return;

        restoreViewport(guard.get(), viewport);
    };

    if (view->imageData() != state.frame)
    {
        QObject::connect(view, &FITSView::loaded, view, restore, Qt::SingleShotConnection);
        view->loadData(state.frame);
        return;
    }

    restore();
}

const QSharedPointer<FITSData> &WorkspaceSession::frame(Source source) const
{
    return m_sourceStates[sourceIndex(source)].frame;
}

const WorkspaceSession::ViewportState &WorkspaceSession::viewport(Source source) const
{
    return m_sourceStates[sourceIndex(source)].viewport;
}

void WorkspaceSession::setOverlayVisible(const QString &key, bool visible)
{
    const auto existing = overlayVisible(key);
    if (existing == visible)
        return;

    m_overlayVisibility.insert(key, visible);
    Q_EMIT overlayVisibilityChanged(key, visible);
}

std::optional<bool> WorkspaceSession::overlayVisible(const QString &key) const
{
    const auto it = m_overlayVisibility.constFind(key);
    if (it == m_overlayVisibility.cend())
        return std::nullopt;

    return it.value();
}

}

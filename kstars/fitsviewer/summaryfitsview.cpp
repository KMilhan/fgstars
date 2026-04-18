/*
    SPDX-FileCopyrightText: 2021 Wolfgang Reissenberger <sterne-jaeger@openfuture.de>

    SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "summaryfitsview.h"
#include "fitsviewer.h"
#include "ekos/workspacesession.h"
#include "kstars.h"
#include "QGraphicsOpacityEffect"


SummaryFITSView::SummaryFITSView(QWidget *parent): FITSView(parent, FITS_NORMAL, FITS_NONE)
{
    processInfoWidget = new QWidget(this);
    processInfoWidget->setVisible(m_showProcessInfo);
    processInfoWidget->setGraphicsEffect(new QGraphicsOpacityEffect(this));

    processInfoWidget->raise();

    connect(this, &FITSView::loaded, this, &SummaryFITSView::syncWorkspaceSession);
    connect(this, &FITSView::updated, this, &SummaryFITSView::syncWorkspaceSession);
    connect(this, &FITSView::zoomRubberBand, this, [this](double)
    {
        syncWorkspaceSession();
    });
    connect(horizontalScrollBar(), &QScrollBar::valueChanged, this, [this]()
    {
        syncWorkspaceSession();
    });
    connect(verticalScrollBar(), &QScrollBar::valueChanged, this, [this]()
    {
        syncWorkspaceSession();
    });
}

void SummaryFITSView::setWorkspaceSession(Ekos::WorkspaceSession *session)
{
    m_workspaceSession = session;
    if (m_workspaceSession == nullptr)
        return;

    if (const auto processInfoVisible =
                m_workspaceSession->overlayVisible(QString::fromLatin1(Ekos::WorkspaceSession::CaptureProcessInfoOverlayKey));
            processInfoVisible.has_value())
        showProcessInfo(*processInfoVisible);
    else
        m_workspaceSession->setOverlayVisible(QString::fromLatin1(Ekos::WorkspaceSession::CaptureProcessInfoOverlayKey),
                                              m_showProcessInfo);

    syncWorkspaceSession();
}

void SummaryFITSView::createFloatingToolBar()
{
    FITSView::createFloatingToolBar();

    floatingToolBar->addSeparator();
    openDetachedViewerAction = floatingToolBar->addAction(QIcon::fromTheme("view-preview"),
                               i18n("Open In FITS Viewer"),
                               this, SLOT(openInDetachedViewer()));
    floatingToolBar->addSeparator();
    toggleProcessInfoAction = floatingToolBar->addAction(QIcon::fromTheme("document-properties"),
                              i18n("Show Capture Process Information"),
                              this, SLOT(toggleShowProcessInfo()));
    toggleProcessInfoAction->setCheckable(true);
}

void SummaryFITSView::openInDetachedViewer()
{
    if (imageData().isNull())
        return;

    if (m_detachedViewer.isNull())
    {
        const auto &viewer = KStars::Instance()->createFITSViewer(true);
        m_detachedViewer = viewer.get();
        m_detachedViewerTabID = -1;
        connect(m_detachedViewer, &FITSViewer::terminated, this, [this]()
        {
            m_detachedViewer = nullptr;
            m_detachedViewerTabID = -1;
        });
    }

    auto * const viewer = m_detachedViewer.data();
    if (viewer == nullptr)
        return;

    const auto imageURL = QUrl::fromLocalFile(imageData()->filename().isEmpty()
                          ? QStringLiteral("summary-preview.fits")
                          : imageData()->filename());

    if (m_detachedViewerTabID >= 0)
    {
        if (viewer->updateData(imageData(), imageURL, m_detachedViewerTabID, &m_detachedViewerTabID) == false)
            viewer->loadData(imageData(), imageURL, &m_detachedViewerTabID);
    }
    else
    {
        viewer->loadData(imageData(), imageURL, &m_detachedViewerTabID);
    }

    viewer->show();
    viewer->raise();
    viewer->activateWindow();
}

void SummaryFITSView::showProcessInfo(bool show)
{
    m_showProcessInfo = show;
    processInfoWidget->setVisible(show);
    if(toggleProcessInfoAction != nullptr)
        toggleProcessInfoAction->setChecked(show);
    if (m_workspaceSession != nullptr)
        m_workspaceSession->setOverlayVisible(QString::fromLatin1(Ekos::WorkspaceSession::CaptureProcessInfoOverlayKey), show);
    updateFrame();
}

void SummaryFITSView::resizeEvent(QResizeEvent *event)
{
    FITSView::resizeEvent(event);
    // forward the viewport geometry to the overlay
    processInfoWidget->setGeometry(this->viewport()->geometry());
}

void SummaryFITSView::syncWorkspaceSession()
{
    if (m_workspaceSession == nullptr)
        return;

    const auto activeSource = m_workspaceSession->activeSource();
    if (activeSource.has_value() == false)
        return;

    if (m_workspaceSession->frame(*activeSource) != imageData())
        return;

    m_workspaceSession->captureViewport(*activeSource, this);
}

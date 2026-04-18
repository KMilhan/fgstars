/*
    SPDX-FileCopyrightText: 2021 Wolfgang Reissenberger <sterne-jaeger@openfuture.de>

    SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "summaryfitsview.h"
#include "fitsviewer.h"
#include "kstars.h"
#include "QGraphicsOpacityEffect"


SummaryFITSView::SummaryFITSView(QWidget *parent): FITSView(parent, FITS_NORMAL, FITS_NONE)
{
    processInfoWidget = new QWidget(this);
    processInfoWidget->setVisible(m_showProcessInfo);
    processInfoWidget->setGraphicsEffect(new QGraphicsOpacityEffect(this));

    processInfoWidget->raise();
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
    updateFrame();
}

void SummaryFITSView::resizeEvent(QResizeEvent *event)
{
    FITSView::resizeEvent(event);
    // forward the viewport geometry to the overlay
    processInfoWidget->setGeometry(this->viewport()->geometry());
}

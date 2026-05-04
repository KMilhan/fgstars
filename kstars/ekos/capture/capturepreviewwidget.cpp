/*
    SPDX-FileCopyrightText: 2012 Jasem Mutlaq <mutlaqja@ikarustech.com>
    SPDX-FileCopyrightText: 2021 Wolfgang Reissenberger <sterne-jaeger@openfuture.de>

    SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "capturepreviewwidget.h"
#include "sequencejob.h"
#include <ekos_capture_debug.h>
#include "ksutils.h"
#include "ksmessagebox.h"
#include "ekos/mount/mount.h"
#include "Options.h"
#include "capture.h"
#include "sequencejob.h"
#include "fitsviewer/fitsdata.h"
#include "fitsviewer/summaryfitsview.h"
#include "ekos/workspacesession.h"
#include "ekos/scheduler/schedulerjob.h"
#include "ekos/scheduler/schedulermodulestate.h"

#include <QCheckBox>
#include <QComboBox>
#include <QFrame>
#include <QGridLayout>
#include <QHash>
#include <QLabel>
#include <QSpinBox>

#include <cmath>

using Ekos::SequenceJob;

CapturePreviewWidget::CapturePreviewWidget(QWidget *parent) : QWidget(parent)
{
    setupUi(this);
    m_overlay = QSharedPointer<CaptureProcessOverlay>(new CaptureProcessOverlay);
    m_overlay->setVisible(false);
    // capture device selection
    connect(trainSelectionCB, &QComboBox::currentTextChanged, this, &CapturePreviewWidget::selectedTrainChanged);
    // history navigation
    connect(m_overlay->historyBackwardButton, &QPushButton::clicked, this, &CapturePreviewWidget::showPreviousFrame);
    connect(m_overlay->historyForwardButton, &QPushButton::clicked, this, &CapturePreviewWidget::showNextFrame);
    // deleting of captured frames
    connect(m_overlay->deleteCurrentFrameButton, &QPushButton::clicked, this, &CapturePreviewWidget::deleteCurrentFrame);

    connect(centerTargetB, &QPushButton::clicked, this, &CapturePreviewWidget::centerTargetRequested);
    connect(centerTargetSettingsB, &QPushButton::clicked, this, &CapturePreviewWidget::centerTargetSettingsRequested);
    centerTargetB->setAccessibleName(i18n("Center Target"));
    centerTargetB->setAccessibleDescription(i18n("Capture, plate solve, and correct the mount for the selected target."));
    centerTargetSettingsB->setAccessibleName(i18n("Center Target Settings"));
    centerTargetSettingsB->setAccessibleDescription(
        i18n("Open advanced astrometry settings for centering the selected target."));
    setCenterTargetAvailable(false);

    // make invisible until we have at least two cameras active
    trainSelectionCB->setVisible(false);

    initializeEssentialSimulator();
    initializeSummaryFITSView();
}

void CapturePreviewWidget::shareCaptureModule(Ekos::Capture *module)
{
    if (m_captureModule != nullptr && m_captureModule != module)
        disconnect(m_captureModule, nullptr, this, nullptr);

    m_captureModule = module;
    captureCountsWidget->shareCaptureProcess(module);
    m_trainNames.clear();
    trainSelectionCB->clear();
    // make invisible until we have at least two cameras active
    trainSelectionCB->setVisible(false);
    captureLabel->setVisible(true);

    if (m_captureModule != nullptr)
    {
        connect(m_captureModule, &Ekos::Capture::newDownloadProgress, this, &CapturePreviewWidget::updateDownloadProgress,
                Qt::UniqueConnection);
        connect(m_captureModule, &Ekos::Capture::newExposureProgress, this, &CapturePreviewWidget::updateExposureProgress,
                Qt::UniqueConnection);
        connect(m_captureModule, &Ekos::Capture::captureTarget, this, &CapturePreviewWidget::setTargetName,
                Qt::UniqueConnection);
    }
}

void CapturePreviewWidget::shareSchedulerModuleState(QSharedPointer<Ekos::SchedulerModuleState> state)
{
    m_schedulerModuleState = state;
    captureCountsWidget->shareSchedulerState(state);
}

void CapturePreviewWidget::shareMountModule(Ekos::Mount *module)
{
    if (m_mountModule != nullptr && m_mountModule != module)
        disconnect(m_mountModule, nullptr, this, nullptr);

    m_mountModule = module;
    if (m_mountModule != nullptr)
    {
        connect(m_mountModule, &Ekos::Mount::newTargetName, this, &CapturePreviewWidget::setTargetName,
                Qt::UniqueConnection);
    }
}

void CapturePreviewWidget::shareWorkspaceSession(Ekos::WorkspaceSession *session)
{
    m_workspaceSession = session;
    if (m_fitsPreview != nullptr)
        m_fitsPreview->setWorkspaceSession(session);
}

void CapturePreviewWidget::updateJobProgress(const QSharedPointer<Ekos::SequenceJob> &job,
        const QSharedPointer<FITSData> &data, const QString &trainname)
{
    // ensure that we have all camera device names in the selection
    if (!m_trainNames.contains(trainname))
    {
        m_trainNames.append(trainname);
        trainSelectionCB->addItem(trainname);

        trainSelectionCB->setVisible(m_trainNames.count() >= 2);
        captureLabel->setVisible(m_trainNames.count() < 2);
    }

    if (job != nullptr)
    {
        // cache frame meta data
        m_currentFrame[trainname].frameType = job->getFrameType();
        if (job->getFrameType() == FRAME_LIGHT)
        {
            if (m_schedulerModuleState != nullptr && m_schedulerModuleState->activeJob() != nullptr)
                m_currentFrame[trainname].target = m_schedulerModuleState->activeJob()->getName();
            else
                m_currentFrame[trainname].target = m_mountTarget;
        }
        else
        {
            m_currentFrame[trainname].target = "";
        }

        m_currentFrame[trainname].filterName  = job->getCoreProperty(SequenceJob::SJ_Filter).toString();
        m_currentFrame[trainname].exptime     = job->getCoreProperty(SequenceJob::SJ_Exposure).toDouble();
        m_currentFrame[trainname].targetdrift = -1.0; // will be updated later
        m_currentFrame[trainname].binning     = job->getCoreProperty(SequenceJob::SJ_Binning).toPoint();
        m_currentFrame[trainname].gain        = job->getCoreProperty(SequenceJob::SJ_Gain).toDouble();
        m_currentFrame[trainname].offset      = job->getCoreProperty(SequenceJob::SJ_Offset).toDouble();
        m_currentFrame[trainname].jobType     = job->jobType();
        m_currentFrame[trainname].frameType   = job->getFrameType();
        m_currentFrame[trainname].count       = job->getCoreProperty(SequenceJob::SJ_Count).toInt();
        m_currentFrame[trainname].completed   = job->getCompleted();

        if (data != nullptr)
        {
            m_currentFrame[trainname].filename    = data->filename();
            m_currentFrame[trainname].width       = data->width();
            m_currentFrame[trainname].height      = data->height();
        }

        const auto ISOIndex = job->getCoreProperty(SequenceJob::SJ_Offset).toInt();
        auto *const captureISOs = m_captureModule != nullptr && m_captureModule->mainCamera() != nullptr
                                  ? m_captureModule->mainCamera()->captureISOS
                                  : nullptr;
        if (captureISOs != nullptr && ISOIndex >= 0 && ISOIndex < captureISOs->count())
            m_currentFrame[trainname].iso = captureISOs->itemText(ISOIndex);
        else
            m_currentFrame[trainname].iso = "";
    }

    // forward first to the counting widget
    captureCountsWidget->updateJobProgress(m_currentFrame[trainname], trainname);

    // add it to the overlay if data is present
    if (!data.isNull())
    {
        m_overlay->addFrameData(m_currentFrame[trainname], trainname);
        m_overlay->setVisible(true);
    }

    // load frame
    if (m_fitsPreview != nullptr && Options::useSummaryPreview() && trainSelectionCB->currentText() == trainname)
        m_fitsPreview->loadData(data);
}

void CapturePreviewWidget::updateJobPreview(const QString &filePath)
{
    // without FITS filePath, we do nothing
    if (filePath == "")
        return;

    // load frame
    if (m_fitsPreview != nullptr && Options::useSummaryPreview())
        m_fitsPreview->loadFile(filePath);
}

void CapturePreviewWidget::showNextFrame()
{
    m_overlay->setEnabled(false);
    if (m_overlay->showNextFrame())
        m_fitsPreview->loadFile(m_overlay->currentFrame().filename);
    // Hint: since the FITSView loads in the background, we have to wait for FITSView::load() to enable the layer
    else
        m_overlay->setEnabled(true);
}

void CapturePreviewWidget::showPreviousFrame()
{
    m_overlay->setEnabled(false);
    if (m_overlay->showPreviousFrame())
        m_fitsPreview->loadFile(m_overlay->currentFrame().filename);
    // Hint: since the FITSView loads in the background, we have to wait for FITSView::load() to enable the layer
    else
        m_overlay->setEnabled(true);
}

void CapturePreviewWidget::deleteCurrentFrame()
{
    m_overlay->setEnabled(false);
    if (m_overlay->hasFrames() == false)
        // nothing to delete
        return;

    // make sure that the history does not change inbetween
    int pos = m_overlay->currentPosition();
    CaptureHistory::FrameData current = m_overlay->getFrame(pos);
    QFile *file = new QFile(current.filename);

    // prepare a warning dialog
    // move to trash or delete permanently
    QCheckBox *permanentlyDeleteCB = new QCheckBox(i18n("Delete directly, do not move to trash."));
    permanentlyDeleteCB->setChecked(m_permanentlyDelete);
    KSMessageBox::Instance()->setCheckBox(permanentlyDeleteCB);
    connect(permanentlyDeleteCB, &QCheckBox::toggled, this, [this](bool checked)
    {
        this->m_permanentlyDelete = checked;
    });
    // Delete
    connect(KSMessageBox::Instance(), &KSMessageBox::accepted, this, [this, pos, file]()
    {
        KSMessageBox::Instance()->disconnect(this);
        bool success = false;
        if (this->m_permanentlyDelete == false && (success = file->moveToTrash()))
        {
            qCInfo(KSTARS_EKOS_CAPTURE) << m_overlay->currentFrame().filename << "moved to Trash.";
        }
        else if (this->m_permanentlyDelete && (success = file->remove()))
        {
            qCInfo(KSTARS_EKOS_CAPTURE) << m_overlay->currentFrame().filename << "deleted.";
        }

        if (success)
        {
            // delete it from the history and update the FITS view
            if (m_overlay->deleteFrame(pos) && m_overlay->hasFrames())
            {
                m_fitsPreview->loadFile(m_overlay->currentFrame().filename);
                // Hint: since the FITSView loads in the background, we have to wait for FITSView::load() to enable the layer
            }
            else
            {
                m_fitsPreview->clearData();
                m_overlay->setEnabled(true);
            }
        }
        else
        {
            qCWarning(KSTARS_EKOS_CAPTURE) << "Deleting" << m_overlay->currentFrame().filename <<
                                              "failed!";
            // give up
            m_overlay->setEnabled(true);
        }
        // clear the check box
        KSMessageBox::Instance()->setCheckBox(nullptr);
    });

    // Cancel
    connect(KSMessageBox::Instance(), &KSMessageBox::rejected, this, [this]()
    {
        KSMessageBox::Instance()->disconnect(this);
        // clear the check box
        KSMessageBox::Instance()->setCheckBox(nullptr);
        //do nothing
        m_overlay->setEnabled(true);
    });

    // open the message box
    QFileInfo fileinfo(current.filename);
    KSMessageBox::Instance()->warningContinueCancel(i18n("Do you really want to delete %1 from the file system?",
            fileinfo.fileName()),
            i18n("Delete %1", fileinfo.fileName()), 0, false, i18n("Delete"));

}

void CapturePreviewWidget::initializeEssentialSimulator()
{
    if (m_essentialSimulatorPanel != nullptr)
        return;

    auto *panel = new QFrame(this);
    panel->setObjectName(QStringLiteral("essentialSimulatorPanel"));
    panel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    panel->setAccessibleName(i18n("Essential Simulator"));
    panel->setAccessibleDescription(i18n("Simulates camera, focus, mount, optical train, GPSD, and PHD2 guiding input."));

    auto *layout = new QGridLayout(panel);
    layout->setContentsMargins(0, 2, 0, 2);
    layout->setHorizontalSpacing(8);
    layout->setVerticalSpacing(2);

    auto *title = new QLabel(i18n("Essential Simulator"), panel);
    title->setObjectName(QStringLiteral("essentialSimulatorTitleL"));
    title->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    m_simulatorGainSB = new QSpinBox(panel);
    m_simulatorGainSB->setObjectName(QStringLiteral("essentialSimulatorGainSB"));
    m_simulatorGainSB->setRange(0, 30);
    m_simulatorGainSB->setSuffix(i18n(" gain"));
    m_simulatorGainSB->setAccessibleName(i18n("Camera Gain"));

    m_simulatorFocusSB = new QSpinBox(panel);
    m_simulatorFocusSB->setObjectName(QStringLiteral("essentialSimulatorFocusSB"));
    m_simulatorFocusSB->setRange(0, 100);
    m_simulatorFocusSB->setValue(50);
    m_simulatorFocusSB->setSuffix(i18n(" focus"));
    m_simulatorFocusSB->setAccessibleName(i18n("Focuser Position"));

    m_simulatorCameraStatusL = new QLabel(panel);
    m_simulatorCameraStatusL->setObjectName(QStringLiteral("essentialSimulatorCameraStatusL"));
    m_simulatorCameraStatusL->setMinimumWidth(210);

    m_simulatorTrackingCB = new QCheckBox(i18n("Tracking"), panel);
    m_simulatorTrackingCB->setObjectName(QStringLiteral("essentialSimulatorTrackingCB"));
    m_simulatorTrackingCB->setChecked(true);

    m_simulatorMountModeCB = new QComboBox(panel);
    m_simulatorMountModeCB->setObjectName(QStringLiteral("essentialSimulatorMountModeCB"));
    m_simulatorMountModeCB->addItems(QStringList { i18n("Sidereal"), i18n("Move"), i18n("Slew"), i18n("Parked") });

    m_simulatorMountStatusL = new QLabel(panel);
    m_simulatorMountStatusL->setObjectName(QStringLiteral("essentialSimulatorMountStatusL"));
    m_simulatorMountStatusL->setMinimumWidth(150);

    m_simulatorTrainCB = new QComboBox(panel);
    m_simulatorTrainCB->setObjectName(QStringLiteral("essentialSimulatorTrainCB"));
    m_simulatorTrainCB->addItems(QStringList
    {
        i18n("Mount"),
        i18n("Camera"),
        i18n("Rotator"),
        i18n("GuideVia"),
        i18n("DustCap"),
        i18n("Scope"),
        i18n("FilterWheel"),
        i18n("Focuser"),
        i18n("Reducer"),
        i18n("LightBox"),
        i18n("Dome"),
        i18n("Weather"),
        i18n("GPS"),
        i18n("PAC"),
    });
    m_simulatorTrainCB->setCurrentText(i18n("Camera"));

    m_simulatorTrainStatusL = new QLabel(panel);
    m_simulatorTrainStatusL->setObjectName(QStringLiteral("essentialSimulatorTrainStatusL"));
    m_simulatorTrainStatusL->setMinimumWidth(240);

    m_simulatorGpsdCB = new QCheckBox(i18n("GPSD"), panel);
    m_simulatorGpsdCB->setObjectName(QStringLiteral("essentialSimulatorGpsdCB"));
    m_simulatorGpsdCB->setChecked(true);

    m_simulatorGpsdStatusL = new QLabel(panel);
    m_simulatorGpsdStatusL->setObjectName(QStringLiteral("essentialSimulatorGpsdStatusL"));
    m_simulatorGpsdStatusL->setMinimumWidth(150);

    m_simulatorGuideCB = new QCheckBox(i18n("PHD2"), panel);
    m_simulatorGuideCB->setObjectName(QStringLiteral("essentialSimulatorGuideCB"));
    m_simulatorGuideCB->setChecked(true);

    m_simulatorGuideModeCB = new QComboBox(panel);
    m_simulatorGuideModeCB->setObjectName(QStringLiteral("essentialSimulatorGuideModeCB"));
    m_simulatorGuideModeCB->addItems(QStringList { i18n("Looping"), i18n("Guiding"), i18n("Dither") });
    m_simulatorGuideModeCB->setCurrentText(i18n("Guiding"));

    m_simulatorGuideStatusL = new QLabel(panel);
    m_simulatorGuideStatusL->setObjectName(QStringLiteral("essentialSimulatorGuideStatusL"));
    m_simulatorGuideStatusL->setMinimumWidth(140);

    layout->addWidget(title, 0, 0);
    layout->addWidget(m_simulatorGainSB, 0, 1);
    layout->addWidget(m_simulatorFocusSB, 0, 2);
    layout->addWidget(m_simulatorCameraStatusL, 0, 3, 1, 2);
    layout->addWidget(m_simulatorTrackingCB, 1, 0);
    layout->addWidget(m_simulatorMountModeCB, 1, 1);
    layout->addWidget(m_simulatorMountStatusL, 1, 2);
    layout->addWidget(m_simulatorTrainCB, 1, 3);
    layout->addWidget(m_simulatorTrainStatusL, 1, 4);
    layout->addWidget(m_simulatorGpsdCB, 2, 0);
    layout->addWidget(m_simulatorGpsdStatusL, 2, 1, 1, 2);
    layout->addWidget(m_simulatorGuideCB, 2, 3);
    layout->addWidget(m_simulatorGuideModeCB, 2, 4);
    layout->addWidget(m_simulatorGuideStatusL, 2, 5);
    layout->setColumnStretch(6, 1);

    verticalLayout->insertWidget(2, panel);
    m_essentialSimulatorPanel = panel;

    const auto refresh = [this]
    {
        updateEssentialSimulator();
    };
    connect(m_simulatorGainSB, qOverload<int>(&QSpinBox::valueChanged), this, refresh);
    connect(m_simulatorFocusSB, qOverload<int>(&QSpinBox::valueChanged), this, refresh);
    connect(m_simulatorTrackingCB, &QCheckBox::toggled, this, refresh);
    connect(m_simulatorMountModeCB, &QComboBox::currentTextChanged, this, refresh);
    connect(m_simulatorTrainCB, &QComboBox::currentTextChanged, this, refresh);
    connect(m_simulatorGpsdCB, &QCheckBox::toggled, this, refresh);
    connect(m_simulatorGuideCB, &QCheckBox::toggled, this, refresh);
    connect(m_simulatorGuideModeCB, &QComboBox::currentTextChanged, this, refresh);
    updateEssentialSimulator();
}

void CapturePreviewWidget::updateEssentialSimulator()
{
    const double cameraNoise = 1.2 + m_simulatorGainSB->value() * 0.12;
    const double focusBlur = std::abs(m_simulatorFocusSB->value() - 50) * 0.04;
    m_simulatorCameraStatusL->setText(i18n("Noise %1 e- - Focus blur %2 px",
                                           QString::number(cameraNoise, 'f', 1),
                                           QString::number(focusBlur, 'f', 1)));

    const bool tracking = m_simulatorTrackingCB->isChecked();
    const QString mountMode = m_simulatorMountModeCB->currentText();
    const double drift = tracking && mountMode == i18n("Sidereal") ? 0.2 : 8.0;
    m_simulatorMountStatusL->setText(i18n("%1 - drift %2",
                                          tracking ? mountMode : i18n("Tracking off"),
                                          QString::number(drift, 'f', 1)));

    const QHash<QString, QString> trainSummary
    {
        { i18n("Mount"), i18n("Mount: telescope simulator") },
        { i18n("Camera"), i18n("Camera: CCD, gain, noise") },
        { i18n("Rotator"), i18n("Rotator: angle simulator") },
        { i18n("GuideVia"), i18n("GuideVia: pulse source") },
        { i18n("DustCap"), i18n("DustCap: cover state") },
        { i18n("Scope"), i18n("Scope: aperture, focal length") },
        { i18n("FilterWheel"), i18n("FilterWheel: filter slot") },
        { i18n("Focuser"), i18n("Focuser: focus position") },
        { i18n("Reducer"), i18n("Reducer: focal ratio") },
        { i18n("LightBox"), i18n("LightBox: flat panel") },
        { i18n("Dome"), i18n("Dome: parked or slaved") },
        { i18n("Weather"), i18n("Weather: safe or alert") },
        { i18n("GPS"), i18n("GPS: site and time source") },
        { i18n("PAC"), i18n("PAC: alignment correction") },
    };
    m_simulatorTrainStatusL->setText(trainSummary.value(m_simulatorTrainCB->currentText()));

    m_simulatorGpsdStatusL->setText(m_simulatorGpsdCB->isChecked()
                                    ? i18n("GPSD fixed: 35.7N 139.7E")
                                    : i18n("GPSD disconnected"));

    if (!m_simulatorGuideCB->isChecked())
    {
        m_simulatorGuideStatusL->setText(i18n("PHD2 idle"));
        return;
    }

    const QString guideMode = m_simulatorGuideModeCB->currentText();
    const double rms = guideMode == i18n("Guiding") ? 0.38 : guideMode == i18n("Dither") ? 0.72 : 1.10;
    m_simulatorGuideStatusL->setText(i18n("PHD2 %1 RMS %2 px", guideMode, QString::number(rms, 'f', 2)));
}

void CapturePreviewWidget::initializeSummaryFITSView()
{
    if (m_fitsPreview != nullptr)
        return;

    // The capture preview widget owns the embedded workspace and its overlay host.
    m_fitsPreview.reset(new SummaryFITSView(previewWidget));
    m_fitsPreview->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_fitsPreview->createFloatingToolBar();
    m_fitsPreview->setCursorMode(FITSView::dragCursor);
    m_fitsPreview->showProcessInfo(false);
    m_fitsPreview->setWorkspaceSession(m_workspaceSession);

    auto *previewLayout = new QVBoxLayout();
    previewLayout->setContentsMargins(0, 0, 0, 0);
    previewLayout->addWidget(m_fitsPreview.get());
    previewWidget->setLayout(previewLayout);
    previewWidget->setContentsMargins(0, 0, 0, 0);

    auto *layout = new QVBoxLayout(m_fitsPreview->processInfoWidget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_overlay.get(), 0);

    m_fitsPreview->processInfoWidget->setLayout(layout);
    connect(m_fitsPreview.get(), &FITSView::loaded, this, [this]()
    {
        m_overlay->setEnabled(true);
    });
    connect(m_fitsPreview.get(), &FITSView::failed, this, [this]()
    {
        m_overlay->setEnabled(true);
    });
}

void CapturePreviewWidget::setEnabled(bool enabled)
{
    // forward to sub widget
    captureCountsWidget->setEnabled(enabled);
    QWidget::setEnabled(enabled);
}

void CapturePreviewWidget::reset()
{
    shareCaptureModule(nullptr);
    shareMountModule(nullptr);
    shareSchedulerModuleState(nullptr);

    m_currentFrame.clear();
    m_trainNames.clear();
    trainSelectionCB->clear();
    trainSelectionCB->setVisible(false);
    captureLabel->setVisible(true);
    targetLabel->setVisible(false);
    mountTarget->setVisible(false);
    m_mountTarget.clear();

    if (m_overlay->hasFrames())
        m_overlay->captureHistory().reset();
    m_overlay->setVisible(false);
    // forward to sub widget
    captureCountsWidget->reset();
}

void CapturePreviewWidget::updateCaptureStatus(Ekos::CaptureState status, bool isPreview, const QString &trainname)
{
    // forward to sub widgets
    captureStatusWidget->setCaptureState(status);
    captureCountsWidget->updateCaptureStatus(status, isPreview, trainname);
}

void CapturePreviewWidget::updateTargetDistance(double targetDiff)
{
    // forward it to the overlay
    m_overlay->updateTargetDistance(targetDiff);
}

void CapturePreviewWidget::updateCaptureCountDown(int delta)
{
    // forward to sub widget
    captureCountsWidget->updateCaptureCountDown(delta);
}

void CapturePreviewWidget::selectedTrainChanged(QString newName)
{
    m_overlay->setCurrentTrainName(newName);
    captureCountsWidget->setCurrentTrainName(newName);

    m_overlay->setEnabled(false);
    if (m_overlay->hasFrames())
    {
        // Hint: since the FITSView loads in the background, we have to wait for FITSView::load() to enable the layer
        m_fitsPreview->loadFile(m_overlay->currentFrame().filename);
    }
    else
    {
        m_fitsPreview->clearData();
        m_overlay->setEnabled(true);
    }
    // update counters
    updateCaptureCountDown(0);
    captureCountsWidget->refreshCaptureCounters(newName);
}

void CapturePreviewWidget::updateExposureProgress(const QSharedPointer<Ekos::SequenceJob> &job, const QString &trainname)
{
    if (trainname == trainSelectionCB->currentText())
        captureCountsWidget->updateExposureProgress(job, trainname);
}

void CapturePreviewWidget::updateDownloadProgress(double downloadTimeLeft, const QString &trainname)
{
    if (trainname == trainSelectionCB->currentText())
        captureCountsWidget->updateDownloadProgress(downloadTimeLeft, trainname);
}

void CapturePreviewWidget::setTargetName(QString name)
{
    targetLabel->setVisible(!name.isEmpty());
    mountTarget->setVisible(!name.isEmpty());
    mountTarget->setText(name);
    m_mountTarget = name;
    m_currentFrame[trainSelectionCB->currentText()].target = name;
}

void CapturePreviewWidget::setCenterTargetAvailable(bool available)
{
    centerTargetB->setEnabled(available);
}

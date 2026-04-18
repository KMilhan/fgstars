/*
    SPDX-FileCopyrightText: 2026 OpenAI

    SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <QObject>
#include <QHash>
#include <QPoint>
#include <QRect>
#include <QSharedPointer>

#include <array>
#include <optional>
#include <utility>

class FITSData;
class FITSView;

namespace Ekos
{

class WorkspaceSession : public QObject
{
        Q_OBJECT

    public:
        enum class Source
        {
            Capture,
            Focus,
            Align,
            Guide,
        };
        Q_ENUM(Source)

        struct ViewportState
        {
            double zoom {100.0};
            QPoint scrollPosition {};
            bool valid {false};

            bool operator==(const ViewportState &) const = default;
        };

        struct FocusOverlayState
        {
            bool trackingBoxEnabled {false};
            QRect trackingBox {};
            bool starsEnabled {false};
            bool starsHfrEnabled {false};

            bool operator==(const FocusOverlayState &) const = default;
        };

        struct AlignOverlayState
        {
            bool crosshairEnabled {false};
            bool eqGridEnabled {false};
            bool objectsEnabled {false};

            bool operator==(const AlignOverlayState &) const = default;
        };

        struct GuideOverlayState
        {
            bool trackingBoxEnabled {false};
            QRect trackingBox {};

            bool operator==(const GuideOverlayState &) const = default;
        };

        static constexpr auto CaptureProcessInfoOverlayKey = "capture.process-info";

        explicit WorkspaceSession(QObject *parent = nullptr);

        void activateSource(Source source);
        std::optional<Source> activeSource() const;

        void publishFrame(Source source, const QSharedPointer<FITSData> &data);
        void publishView(Source source, const QSharedPointer<FITSView> &view);
        void captureViewport(Source source, const FITSView *view);
        void applyTo(Source source, FITSView *view) const;

        const QSharedPointer<FITSData> &frame(Source source) const;
        const ViewportState &viewport(Source source) const;
        void setFocusOverlay(const FocusOverlayState &state);
        std::optional<FocusOverlayState> focusOverlay() const;
        void setAlignOverlay(const AlignOverlayState &state);
        std::optional<AlignOverlayState> alignOverlay() const;
        void setGuideOverlay(const GuideOverlayState &state);
        std::optional<GuideOverlayState> guideOverlay() const;

        void setOverlayVisible(const QString &key, bool visible);
        std::optional<bool> overlayVisible(const QString &key) const;

    Q_SIGNALS:
        void sourceUpdated(Source source);
        void activeSourceChanged(Source source);
        void overlayVisibilityChanged(const QString &key, bool visible);

    private:
        struct SourceState
        {
            QSharedPointer<FITSData> frame;
            ViewportState viewport;
        };

        static constexpr auto SOURCE_COUNT = std::size_t {std::to_underlying(Source::Guide) + 1};

        static constexpr std::size_t sourceIndex(Source source)
        {
            return static_cast<std::size_t>(std::to_underlying(source));
        }

        std::array<SourceState, SOURCE_COUNT> m_sourceStates {};
        std::optional<Source> m_activeSource {Source::Capture};
        std::optional<FocusOverlayState> m_focusOverlay;
        std::optional<AlignOverlayState> m_alignOverlay;
        std::optional<GuideOverlayState> m_guideOverlay;
        QHash<QString, bool> m_overlayVisibility;
};

}

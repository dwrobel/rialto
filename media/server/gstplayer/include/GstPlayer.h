/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2022 Sky UK
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef FIREBOLT_RIALTO_SERVER_GST_PLAYER_H_
#define FIREBOLT_RIALTO_SERVER_GST_PLAYER_H_

#include "IGlibWrapper.h"
#include "IGstDispatcherThread.h"
#include "IGstPlayer.h"
#include "IGstPlayerPrivate.h"
#include "IGstSrc.h"
#include "IGstWrapper.h"
#include "ITimer.h"
#include "IWorkerThread.h"
#include "PlayerContext.h"
#include "tasks/IPlayerTask.h"
#include "tasks/IPlayerTaskFactory.h"
#include <IMediaPipeline.h>
#include <memory>
#include <string>

namespace firebolt::rialto::server
{
constexpr uint32_t kMinPrimaryVideoWidth{1920};
constexpr uint32_t kMinPrimaryVideoHeight{1080};

/**
 * @brief IGstPlayer factory class definition.
 */
class GstPlayerFactory : public IGstPlayerFactory
{
public:
    /**
     * @brief Weak pointer to the singleton factory object.
     */
    static std::weak_ptr<IGstPlayerFactory> m_factory;

    std::unique_ptr<IGstPlayer> createGstPlayer(IGstPlayerClient *client, IDecryptionService &decryptionService,
                                                MediaType type, const VideoRequirements &videoRequirements) override;
};

/**
 * @brief The definition of the GstPlayer.
 */
class GstPlayer : public IGstPlayer, public IGstPlayerPrivate
{
public:
    /**
     * @brief The constructor.
     *
     * @param[in] client                       : The gstreamer player client.
     * @param[in] decryptionService            : The decryption service
     * @param[in] type                         : The media type the gstreamer player shall support.
     * @param[in] videoRequirements            : The video requirements for the playback.
     * @param[in] gstWrapper                   : The gstreamer wrapper.
     * @param[in] glibWrapper                  : The glib wrapper.
     * @param[in] gstSrcFactory                : The gstreamer rialto src factory.
     * @param[in] timerFactory                 : The Timer factory
     * @param[in] taskFactory                  : The task factory
     * @param[in] workerThreadFactory          : The worker thread factory
     * @param[in] gstDispatcherThreadFactory   : The gst dispatcher thread factory
     */
    GstPlayer(IGstPlayerClient *client, IDecryptionService &decryptionService, MediaType type,
              const VideoRequirements &videoRequirements, const std::shared_ptr<IGstWrapper> &gstWrapper,
              const std::shared_ptr<IGlibWrapper> &glibWrapper, const std::shared_ptr<IGstSrcFactory> &gstSrcFactory,
              std::shared_ptr<common::ITimerFactory> timerFactory, std::unique_ptr<IPlayerTaskFactory> taskFactory,
              std::unique_ptr<IWorkerThreadFactory> workerThreadFactory,
              std::unique_ptr<IGstDispatcherThreadFactory> gstDispatcherThreadFactory);

    /**
     * @brief Virtual destructor.
     */
    virtual ~GstPlayer();

    void attachSource(const IMediaPipeline::MediaSource &mediaSource) override;
    void play() override;
    void pause() override;
    void stop() override;
    void attachSamples(const IMediaPipeline::MediaSegmentVector &mediaSegments) override;
    void attachSamples(const std::shared_ptr<IDataReader> &dataReader) override;
    void setPosition(std::int64_t position) override;
    void setVideoGeometry(int x, int y, int width, int height) override;
    void setEos(const firebolt::rialto::MediaSourceType &type) override;
    void setPlaybackRate(double rate) override;
    bool getPosition(std::int64_t &position) override;

private:
    void scheduleSourceSetupFinish() override;
    void scheduleNeedMediaData(GstAppSrc *src) override;
    void scheduleEnoughData(GstAppSrc *src) override;
    void scheduleAudioUnderflow() override;
    void scheduleVideoUnderflow() override;
    bool setWesterossinkRectangle() override;
    bool setWesterossinkSecondaryVideo() override;
    void notifyNeedMediaData(bool audioNotificationNeeded, bool videoNotificationNeeded) override;
    GstBuffer *createBuffer(const IMediaPipeline::MediaSegment &mediaSegment) const override;
    void attachAudioData() override;
    void attachVideoData() override;
    void updateAudioCaps(int32_t rate, int32_t channels) override;
    void updateVideoCaps(int32_t width, int32_t height) override;
    bool changePipelineState(GstState newState) override;
    void startPositionReportingAndCheckAudioUnderflowTimer() override;
    void stopPositionReportingAndCheckAudioUnderflowTimer() override;
    void stopWorkerThread() override;
    void cancelUnderflow(bool &underflowFlag) override;
    void setPendingPlaybackRate() override;
    void renderFrame() override;

private:
    /**
     * @brief Initialises the player pipeline for MSE playback.
     */
    void initMsePipeline();

    /**
     * @brief Gets the flag from gstreamer.
     *
     * @param[in] nick : The name of the flag in gstreamer.
     *
     * @retval Value of the flag or 0 on error.
     */
    unsigned getGstPlayFlag(const char *nick);

    /**
     * @brief Callback on source-setup. Called by the Gstreamer thread
     *
     * @param[in] pipeline  : The pipeline the signal was fired from.
     * @param[in] source    : The source to setup.
     * @param[in] self      : Reference to the calling object.
     */
    static void setupSource(GstElement *pipeline, GstElement *source, GstPlayer *self);

    /**
     * @brief Callback on element-setup. Called by the Gstreamer thread
     *
     * @param[in] pipeline  : The pipeline the signal was fired from.
     * @param[in] element   : an element that was added to the playbin hierarchy
     * @param[in] self      : Reference to the calling object.
     */
    static void setupElement(GstElement *pipeline, GstElement *element, GstPlayer *self);

private:
    /**
     * @brief The player context.
     */
    PlayerContext m_context;

    /**
     * @brief The gstreamer player client.
     */
    IGstPlayerClient *m_gstPlayerClient = nullptr;

    /**
     * @brief The gstreamer wrapper object.
     */
    std::shared_ptr<IGstWrapper> m_gstWrapper;

    /**
     * @brief The glib wrapper object.
     */
    std::shared_ptr<IGlibWrapper> m_glibWrapper;

    /**
     * @brief Thread for handling player tasks.
     */
    std::unique_ptr<IWorkerThread> m_workerThread;

    /**
     * @brief Thread for handling gst bus callbacks
     */
    std::unique_ptr<IGstDispatcherThread> m_gstDispatcherThread;

    /**
     * @brief Factory creating timers
     *
     */
    std::shared_ptr<common::ITimerFactory> m_timerFactory;

    /**
     * @brief Timer to trigger FinishSourceSetup
     */
    std::unique_ptr<firebolt::rialto::common::ITimer> m_finishSourceSetupTimer{nullptr};

    /**
     * @brief Timer reporting playback position and check audio underflow
     *
     * Variable can be used only in worker thread
     */
    std::unique_ptr<firebolt::rialto::common::ITimer> m_positionReportingAndCheckAudioUnderflowTimer{nullptr};

    /**
     * @brief The GstPlayer task factory
     */
    std::unique_ptr<IPlayerTaskFactory> m_taskFactory;
};
} // namespace firebolt::rialto::server

#endif // FIREBOLT_RIALTO_SERVER_GST_PLAYER_H_

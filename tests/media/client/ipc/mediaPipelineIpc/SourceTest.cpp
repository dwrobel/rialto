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

#include "MediaPipelineIpcTestBase.h"

MATCHER_P8(attachSourceRequestMatcher2, sessionId, mimeType, numberOfChannels, sampleRate, codecSpecificConfig,
           alignment, streamFormat, codecData, "")
{
    const ::firebolt::rialto::AttachSourceRequest *request =
        dynamic_cast<const ::firebolt::rialto::AttachSourceRequest *>(arg);
    std::vector<uint8_t> codecDataFromReq(request->codec_data().begin(), request->codec_data().end());
    return ((request->session_id() == sessionId) &&
            (static_cast<const unsigned int>(request->media_type()) ==
             static_cast<const unsigned int>(MediaSourceType::AUDIO)) &&
            (request->mime_type() == mimeType) && (request->has_audio_config()) &&
            (request->audio_config().number_of_channels() == numberOfChannels) &&
            (request->audio_config().sample_rate() == sampleRate) &&
            (request->audio_config().codec_specific_config() == codecSpecificConfig) &&
            (request->segment_alignment() == alignment) && (request->stream_format() == streamFormat) &&
            (codecDataFromReq == codecData));
}

MATCHER_P3(attachSourceRequestMatcher, sessionId, mediaType, mimeType, "")
{
    const ::firebolt::rialto::AttachSourceRequest *request =
        dynamic_cast<const ::firebolt::rialto::AttachSourceRequest *>(arg);
    return ((request->session_id() == sessionId) &&
            (static_cast<const unsigned int>(request->media_type()) == mediaType) && (request->mime_type() == mimeType));
}

MATCHER_P2(removeSourceRequestMatcher, sessionId, sourceId, "")
{
    const ::firebolt::rialto::RemoveSourceRequest *request =
        dynamic_cast<const ::firebolt::rialto::RemoveSourceRequest *>(arg);
    return ((request->session_id() == sessionId) && (request->source_id() == sourceId));
}

class RialtoClientMediaPipelineIpcSourceTest : public MediaPipelineIpcTestBase
{
protected:
    int32_t m_id = 456;
    MediaSourceType m_type = MediaSourceType::AUDIO;
    const char *m_kMimeType = "video/mpeg";

    virtual void SetUp()
    {
        MediaPipelineIpcTestBase::SetUp();

        createMediaPipelineIpc();
    }

    virtual void TearDown()
    {
        destroyMediaPipelineIpc();

        MediaPipelineIpcTestBase::TearDown();
    }

public:
    void setAttachSourceResponse(google::protobuf::Message *response)
    {
        firebolt::rialto::AttachSourceResponse *attachSourceResponse =
            dynamic_cast<firebolt::rialto::AttachSourceResponse *>(response);
        attachSourceResponse->set_source_id(m_id);
    }
};

/**
 * Test that Load can be called successfully.
 */
TEST_F(RialtoClientMediaPipelineIpcSourceTest, AttachSourceSuccess)
{
    expectIpcApiCallSuccess();

    EXPECT_CALL(*m_channelMock,
                CallMethod(methodMatcher("attachSource"), m_controllerMock.get(),
                           attachSourceRequestMatcher(m_sessionId, static_cast<uint32_t>(m_type), m_kMimeType), _,
                           m_blockingClosureMock.get()))
        .WillOnce(WithArgs<3>(Invoke(this, &RialtoClientMediaPipelineIpcSourceTest::setAttachSourceResponse)));

    IMediaPipeline::MediaSource mediaSource(m_id, m_type, m_kMimeType);

    EXPECT_EQ(m_mediaPipelineIpc->attachSource(mediaSource, m_id), true);
}

/**
 * Test attach audio source with codec specific config.
 */
TEST_F(RialtoClientMediaPipelineIpcSourceTest, AttachAudioSourceWithAdditionaldataSuccess)
{
    expectIpcApiCallSuccess();

    uint32_t numberOfChannels = 6;
    uint32_t sampleRate = 48000;
    std::string codecSpecificConfigStr("1243567");
    firebolt::rialto::SegmentAlignment alignment = firebolt::rialto::SegmentAlignment::UNDEFINED;
    std::vector<uint8_t> codecData{'T', 'E', 'S', 'T'};
    firebolt::rialto::StreamFormat streamFormat = firebolt::rialto::StreamFormat::RAW;
    EXPECT_CALL(*m_channelMock,
                CallMethod(methodMatcher("attachSource"), m_controllerMock.get(),
                           attachSourceRequestMatcher2(m_sessionId, m_kMimeType, numberOfChannels, sampleRate,
                                                       codecSpecificConfigStr,
                                                       firebolt::rialto::AttachSourceRequest_SegmentAlignment_ALIGNMENT_UNDEFINED,
                                                       firebolt::rialto::AttachSourceRequest_StreamFormat_STREAM_FORMAT_RAW,
                                                       codecData),
                           _, m_blockingClosureMock.get()))
        .WillOnce(WithArgs<3>(Invoke(this, &RialtoClientMediaPipelineIpcSourceTest::setAttachSourceResponse)));

    std::vector<uint8_t> codecSpecificConfig;
    codecSpecificConfig.assign(codecSpecificConfigStr.begin(), codecSpecificConfigStr.end());
    AudioConfig audioConfig{6, 48000, codecSpecificConfig};

    IMediaPipeline::MediaSource mediaSource(m_id, m_kMimeType, audioConfig, alignment, streamFormat, codecData);

    EXPECT_EQ(m_mediaPipelineIpc->attachSource(mediaSource, m_id), true);
}

/**
 * Test that Load fails when ipc fails.
 */
TEST_F(RialtoClientMediaPipelineIpcSourceTest, AttachSourceFailure)
{
    expectIpcApiCallFailure();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("attachSource"), _, _, _, _));
    IMediaPipeline::MediaSource mediaSource(m_id, m_type, m_kMimeType);

    EXPECT_EQ(m_mediaPipelineIpc->attachSource(mediaSource, m_id), false);
}

/**
 * Test that AttachSource fails if the ipc channel disconnected.
 */
TEST_F(RialtoClientMediaPipelineIpcSourceTest, AttachSourceChannelDisconnected)
{
    expectIpcApiCallDisconnected();
    expectUnsubscribeEvents();

    IMediaPipeline::MediaSource mediaSource(m_id, m_type, m_kMimeType);
    EXPECT_EQ(m_mediaPipelineIpc->attachSource(mediaSource, m_id), false);

    // Reattach channel on destroySession
    EXPECT_CALL(*m_ipcClientMock, getChannel()).WillOnce(Return(m_channelMock)).RetiresOnSaturation();
    expectSubscribeEvents();
}

/**
 * Test that AttachSource fails if the ipc channel disconnected and succeeds if the channel is reconnected.
 */
TEST_F(RialtoClientMediaPipelineIpcSourceTest, AttachSourceReconnectChannel)
{
    expectIpcApiCallReconnected();
    expectUnsubscribeEvents();
    expectSubscribeEvents();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("attachSource"), _, _, _, _));

    IMediaPipeline::MediaSource mediaSource(m_id, m_type, m_kMimeType);
    EXPECT_EQ(m_mediaPipelineIpc->attachSource(mediaSource, m_id), true);
}

/**
 * Test that Load can be called successfully.
 */
TEST_F(RialtoClientMediaPipelineIpcSourceTest, RemoveSourceSuccess)
{
    expectIpcApiCallSuccess();

    EXPECT_CALL(*m_channelMock,
                CallMethod(methodMatcher("removeSource"), m_controllerMock.get(),
                           removeSourceRequestMatcher(m_sessionId, m_id), _, m_blockingClosureMock.get()));

    EXPECT_EQ(m_mediaPipelineIpc->removeSource(m_id), true);
}

/**
 * Test that Load fails when ipc fails.
 */
TEST_F(RialtoClientMediaPipelineIpcSourceTest, RemoveSourceFailure)
{
    expectIpcApiCallFailure();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("removeSource"), _, _, _, _));

    EXPECT_EQ(m_mediaPipelineIpc->removeSource(m_id), false);
}

/**
 * Test that RemoveSource fails if the ipc channel disconnected.
 */
TEST_F(RialtoClientMediaPipelineIpcSourceTest, RemoveSourceChannelDisconnected)
{
    expectIpcApiCallDisconnected();
    expectUnsubscribeEvents();

    EXPECT_EQ(m_mediaPipelineIpc->removeSource(m_id), false);

    // Reattach channel on destroySession
    EXPECT_CALL(*m_ipcClientMock, getChannel()).WillOnce(Return(m_channelMock)).RetiresOnSaturation();
    expectSubscribeEvents();
}

/**
 * Test that RemoveSource fails if the ipc channel disconnected and succeeds if the channel is reconnected.
 */
TEST_F(RialtoClientMediaPipelineIpcSourceTest, RemoveSourceReconnectChannel)
{
    expectIpcApiCallReconnected();
    expectUnsubscribeEvents();
    expectSubscribeEvents();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("removeSource"), _, _, _, _));

    EXPECT_EQ(m_mediaPipelineIpc->removeSource(m_id), true);
}

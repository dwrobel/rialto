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

#include "MediaPipelineModuleServiceTestsFixture.h"
#include "MediaPipelineModuleService.h"
#include "MediaSourceUtil.h"
#include <fcntl.h>
#include <string>
#include <sys/stat.h>
#include <vector>

using testing::_;
using testing::DoAll;
using testing::Invoke;
using testing::Return;
using testing::SaveArg;
using testing::SetArgReferee;

namespace
{
constexpr firebolt::rialto::MediaSourceType mediaSourceType{firebolt::rialto::MediaSourceType::AUDIO};
constexpr std::uint32_t width{1920};
constexpr std::uint32_t height{1080};
constexpr int hardcodedSessionId{2};
const firebolt::rialto::MediaType mediaType{firebolt::rialto::MediaType::MSE};
const std::string mimeType{"exampleMimeType"};
constexpr uint32_t numberOfChannels{6};
constexpr uint32_t sampleRate{48000};
const std::string codecSpecificConfigStr("1243567");
std::vector<uint8_t> codecData{'T', 'E', 'S', 'T'};
const std::string url{"https://example.url.com"};
constexpr int64_t position{2000000000};
constexpr std::uint32_t requestId{2};
const firebolt::rialto::MediaSourceStatus mediaSourceStatus{firebolt::rialto::MediaSourceStatus::CODEC_CHANGED};
constexpr std::uint32_t numFrames{1};
constexpr int x{30};
constexpr int y{40};
constexpr std::uint32_t size{456};
constexpr std::uint64_t sourceId{12};
constexpr size_t frameCount{5};
constexpr std::uint32_t maxBytes{2};
constexpr std::uint32_t needDataRequestId{32};
constexpr firebolt::rialto::ShmInfo shmInfo{15, 16, 17};
constexpr firebolt::rialto::PlaybackState playbackState{firebolt::rialto::PlaybackState::PLAYING};
constexpr firebolt::rialto::NetworkState networkState{firebolt::rialto::NetworkState::BUFFERED};
constexpr firebolt::rialto::QosInfo qosInfo{5u, 2u};
constexpr double rate{1.5};
} // namespace

MATCHER_P6(NeedMediaDataEventMatcher, sessionId, sourceId, needDataRequestId, frameCount, maxMediaBytes, shmInfo, "")
{
    std::shared_ptr<firebolt::rialto::NeedMediaDataEvent> event =
        std::dynamic_pointer_cast<firebolt::rialto::NeedMediaDataEvent>(arg);

    return ((sessionId == event->session_id()) && (sourceId == event->source_id()) &&
            (needDataRequestId == event->request_id()) && (frameCount == event->frame_count()) &&
            (shmInfo->maxMetadataBytes == event->shm_info().max_metadata_bytes()) &&
            (shmInfo->metadataOffset == event->shm_info().metadata_offset()) &&
            (shmInfo->mediaDataOffset == event->shm_info().media_data_offset()) &&
            (shmInfo->maxMediaBytes == event->shm_info().max_media_bytes()));
}

MATCHER_P(PositionChangeEventMatcher, position, "")
{
    std::shared_ptr<firebolt::rialto::PositionChangeEvent> event =
        std::dynamic_pointer_cast<firebolt::rialto::PositionChangeEvent>(arg);
    return (position == event->position());
}

MATCHER_P3(QosEventMatcher, sourceId, processed, dropped, "")
{
    std::shared_ptr<firebolt::rialto::QosEvent> event = std::dynamic_pointer_cast<firebolt::rialto::QosEvent>(arg);
    return ((sourceId == event->source_id()) && (processed == event->qos_info().processed()) &&
            (dropped == event->qos_info().dropped()));
}

MATCHER_P(PlaybackStateChangeEventMatcher, playbackState, "")
{
    std::shared_ptr<firebolt::rialto::PlaybackStateChangeEvent> event =
        std::dynamic_pointer_cast<firebolt::rialto::PlaybackStateChangeEvent>(arg);
    return (playbackState == event->state());
}

MATCHER_P(NetworkStateChangeEventMatcher, networkState, "")
{
    std::shared_ptr<firebolt::rialto::NetworkStateChangeEvent> event =
        std::dynamic_pointer_cast<firebolt::rialto::NetworkStateChangeEvent>(arg);
    return (networkState == event->state());
}

namespace firebolt::rialto
{
firebolt::rialto::LoadRequest_MediaType convertMediaType(const firebolt::rialto::MediaType &mediaType)
{
    switch (mediaType)
    {
    case firebolt::rialto::MediaType::UNKNOWN:
    {
        return firebolt::rialto::LoadRequest_MediaType::LoadRequest_MediaType_UNKNOWN;
    }
    case firebolt::rialto::MediaType::MSE:
    {
        return firebolt::rialto::LoadRequest_MediaType::LoadRequest_MediaType_MSE;
    }
    }
    return firebolt::rialto::LoadRequest_MediaType::LoadRequest_MediaType_UNKNOWN;
}

firebolt::rialto::ProtoMediaSourceType convertProtoMediaSourceType(const firebolt::rialto::MediaSourceType &mediaSourceType)
{
    switch (mediaSourceType)
    {
    case firebolt::rialto::MediaSourceType::UNKNOWN:
    {
        return firebolt::rialto::ProtoMediaSourceType::UNKNOWN;
    }
    case firebolt::rialto::MediaSourceType::AUDIO:
    {
        return firebolt::rialto::ProtoMediaSourceType::AUDIO;
    }
    case firebolt::rialto::MediaSourceType::VIDEO:
    {
        return firebolt::rialto::ProtoMediaSourceType::VIDEO;
    }
    }
    return firebolt::rialto::ProtoMediaSourceType::UNKNOWN;
}

firebolt::rialto::AttachSourceRequest_StreamFormat convertStreamFormat(const firebolt::rialto::StreamFormat &streamFormat)
{
    switch (streamFormat)
    {
    case firebolt::rialto::StreamFormat::UNDEFINED:
    {
        return firebolt::rialto::AttachSourceRequest_StreamFormat_STREAM_FORMAT_UNDEFINED;
    }
    case firebolt::rialto::StreamFormat::RAW:
    {
        return firebolt::rialto::AttachSourceRequest_StreamFormat_STREAM_FORMAT_RAW;
    }
    case firebolt::rialto::StreamFormat::AVC:
    {
        return firebolt::rialto::AttachSourceRequest_StreamFormat_STREAM_FORMAT_AVC;
    }
    case firebolt::rialto::StreamFormat::BYTE_STREAM:
    {
        return firebolt::rialto::AttachSourceRequest_StreamFormat_STREAM_FORMAT_BYTE_STREAM;
    }
    }
    return firebolt::rialto::AttachSourceRequest_StreamFormat_STREAM_FORMAT_UNDEFINED;
}

firebolt::rialto::HaveDataRequest_MediaSourceStatus
convertHaveDataRequestMediaSourceStatus(const firebolt::rialto::MediaSourceStatus &status)
{
    switch (status)
    {
    case firebolt::rialto::MediaSourceStatus::OK:
        return firebolt::rialto::HaveDataRequest_MediaSourceStatus_OK;
    case firebolt::rialto::MediaSourceStatus::EOS:
        return firebolt::rialto::HaveDataRequest_MediaSourceStatus_EOS;
    case firebolt::rialto::MediaSourceStatus::ERROR:
        return firebolt::rialto::HaveDataRequest_MediaSourceStatus_ERROR;
    case firebolt::rialto::MediaSourceStatus::CODEC_CHANGED:
        return firebolt::rialto::HaveDataRequest_MediaSourceStatus_CODEC_CHANGED;
    case firebolt::rialto::MediaSourceStatus::NO_AVAILABLE_SAMPLES:
        return firebolt::rialto::HaveDataRequest_MediaSourceStatus_NO_AVAILABLE_SAMPLES;
    }
    return firebolt::rialto::HaveDataRequest_MediaSourceStatus_UNKNOWN;
}

firebolt::rialto::PlaybackStateChangeEvent_PlaybackState
convertPlaybackState(const firebolt::rialto::PlaybackState &playbackState)
{
    switch (playbackState)
    {
    case firebolt::rialto::PlaybackState::UNKNOWN:
    {
        return firebolt::rialto::PlaybackStateChangeEvent_PlaybackState_UNKNOWN;
    }
    case firebolt::rialto::PlaybackState::IDLE:
    {
        return firebolt::rialto::PlaybackStateChangeEvent_PlaybackState_IDLE;
    }
    case firebolt::rialto::PlaybackState::PLAYING:
    {
        return firebolt::rialto::PlaybackStateChangeEvent_PlaybackState_PLAYING;
    }
    case firebolt::rialto::PlaybackState::PAUSED:
    {
        return firebolt::rialto::PlaybackStateChangeEvent_PlaybackState_PAUSED;
    }
    case firebolt::rialto::PlaybackState::SEEKING:
    {
        return firebolt::rialto::PlaybackStateChangeEvent_PlaybackState_SEEKING;
    }
    case firebolt::rialto::PlaybackState::FLUSHED:
    {
        return firebolt::rialto::PlaybackStateChangeEvent_PlaybackState_FLUSHED;
    }
    case firebolt::rialto::PlaybackState::STOPPED:
    {
        return firebolt::rialto::PlaybackStateChangeEvent_PlaybackState_STOPPED;
    }
    case firebolt::rialto::PlaybackState::END_OF_STREAM:
    {
        return firebolt::rialto::PlaybackStateChangeEvent_PlaybackState_END_OF_STREAM;
    }
    case firebolt::rialto::PlaybackState::FAILURE:
    {
        return firebolt::rialto::PlaybackStateChangeEvent_PlaybackState_FAILURE;
    }
    }
    return firebolt::rialto::PlaybackStateChangeEvent_PlaybackState_UNKNOWN;
}

firebolt::rialto::NetworkStateChangeEvent_NetworkState
convertNetworkState(const firebolt::rialto::NetworkState &networkState)
{
    switch (networkState)
    {
    case firebolt::rialto::NetworkState::UNKNOWN:
    {
        return firebolt::rialto::NetworkStateChangeEvent_NetworkState_UNKNOWN;
    }
    case firebolt::rialto::NetworkState::IDLE:
    {
        return firebolt::rialto::NetworkStateChangeEvent_NetworkState_IDLE;
    }
    case firebolt::rialto::NetworkState::BUFFERING:
    {
        return firebolt::rialto::NetworkStateChangeEvent_NetworkState_BUFFERING;
    }
    case firebolt::rialto::NetworkState::BUFFERING_PROGRESS:
    {
        return firebolt::rialto::NetworkStateChangeEvent_NetworkState_BUFFERING_PROGRESS;
    }
    case firebolt::rialto::NetworkState::BUFFERED:
    {
        return firebolt::rialto::NetworkStateChangeEvent_NetworkState_BUFFERED;
    }
    case firebolt::rialto::NetworkState::STALLED:
    {
        return firebolt::rialto::NetworkStateChangeEvent_NetworkState_STALLED;
    }
    case firebolt::rialto::NetworkState::FORMAT_ERROR:
    {
        return firebolt::rialto::NetworkStateChangeEvent_NetworkState_FORMAT_ERROR;
    }
    case firebolt::rialto::NetworkState::NETWORK_ERROR:
    {
        return firebolt::rialto::NetworkStateChangeEvent_NetworkState_NETWORK_ERROR;
    }
    case firebolt::rialto::NetworkState::DECODE_ERROR:
    {
        return firebolt::rialto::NetworkStateChangeEvent_NetworkState_DECODE_ERROR;
    }
    }
    return firebolt::rialto::NetworkStateChangeEvent_NetworkState_UNKNOWN;
}
} // namespace firebolt::rialto

MediaPipelineModuleServiceTests::MediaPipelineModuleServiceTests()
    : m_clientMock{std::make_shared<StrictMock<firebolt::rialto::ipc::ClientMock>>()},
      m_serverMock{std::make_shared<StrictMock<firebolt::rialto::ipc::ServerMock>>()},
      m_closureMock{std::make_shared<StrictMock<firebolt::rialto::ipc::ClosureMock>>()},
      m_controllerMock{std::make_shared<StrictMock<firebolt::rialto::ipc::ControllerMock>>()}
{
    m_service = std::make_shared<firebolt::rialto::server::ipc::MediaPipelineModuleService>(m_playbackServiceMock);
}

MediaPipelineModuleServiceTests::~MediaPipelineModuleServiceTests() {}

void MediaPipelineModuleServiceTests::clientWillConnect()
{
    EXPECT_CALL(*m_clientMock, exportService(_));
}

void MediaPipelineModuleServiceTests::clientWillDisconnect()
{
    EXPECT_CALL(m_playbackServiceMock, destroySession(hardcodedSessionId));
}

void MediaPipelineModuleServiceTests::playbackServiceWillCreateSession()
{
    expectRequestSuccess();
    EXPECT_CALL(*m_controllerMock, getClient()).Times(2).WillRepeatedly(Return(m_clientMock));
    EXPECT_CALL(m_playbackServiceMock, createSession(_, _, width, height))
        .WillOnce(DoAll(SaveArg<1>(&m_mediaPipelineClient), Return(true)));
}

void MediaPipelineModuleServiceTests::playbackServiceWillFailToCreateSession()
{
    expectRequestFailure();
    EXPECT_CALL(*m_controllerMock, getClient()).WillOnce(Return(m_clientMock));
    EXPECT_CALL(m_playbackServiceMock, createSession(_, _, width, height)).WillOnce(Return(false));
}

void MediaPipelineModuleServiceTests::playbackServiceWillDestroySession()
{
    expectRequestSuccess();
    EXPECT_CALL(*m_controllerMock, getClient()).WillOnce(Return(m_clientMock));
    EXPECT_CALL(m_playbackServiceMock, destroySession(hardcodedSessionId)).WillOnce(Return(true));
}

void MediaPipelineModuleServiceTests::playbackServiceWillFailToDestroySession()
{
    expectRequestFailure();
    EXPECT_CALL(m_playbackServiceMock, destroySession(hardcodedSessionId)).WillOnce(Return(false));
}

void MediaPipelineModuleServiceTests::playbackServiceWillLoadSession()
{
    expectRequestSuccess();
    EXPECT_CALL(m_playbackServiceMock, load(hardcodedSessionId, mediaType, mimeType, url)).WillOnce(Return(true));
}

void MediaPipelineModuleServiceTests::playbackServiceWillFailToLoadSession()
{
    expectRequestFailure();
    EXPECT_CALL(m_playbackServiceMock, load(hardcodedSessionId, mediaType, mimeType, url)).WillOnce(Return(false));
}

void MediaPipelineModuleServiceTests::playbackServiceWillAttachSource()
{
    firebolt::rialto::IMediaPipeline::MediaSource source{0, mediaSourceType, mimeType.c_str()};
    expectRequestSuccess();
    EXPECT_CALL(m_playbackServiceMock, attachSource(hardcodedSessionId, source)).WillOnce(Return(true));
}

void MediaPipelineModuleServiceTests::playbackServiceWillAttachAudioSourceWithAdditionaldata()
{
    std::vector<uint8_t> codecSpecificConfig;
    codecSpecificConfig.assign(codecSpecificConfigStr.begin(), codecSpecificConfigStr.end());
    firebolt::rialto::AudioConfig audioConfig{numberOfChannels, sampleRate, codecSpecificConfig};
    firebolt::rialto::IMediaPipeline::MediaSource source{0,
                                                         mimeType,
                                                         audioConfig,
                                                         firebolt::rialto::SegmentAlignment::UNDEFINED,
                                                         firebolt::rialto::StreamFormat::RAW,
                                                         codecData};
    expectRequestSuccess();
    EXPECT_CALL(m_playbackServiceMock, attachSource(hardcodedSessionId, source)).WillOnce(Return(true));
}

void MediaPipelineModuleServiceTests::playbackServiceWillFailToAttachSource()
{
    firebolt::rialto::IMediaPipeline::MediaSource source{0, mediaSourceType, mimeType.c_str()};
    expectRequestFailure();
    EXPECT_CALL(m_playbackServiceMock, attachSource(hardcodedSessionId, source)).WillOnce(Return(false));
}

void MediaPipelineModuleServiceTests::playbackServiceWillPlay()
{
    expectRequestSuccess();
    EXPECT_CALL(m_playbackServiceMock, play(hardcodedSessionId)).WillOnce(Return(true));
}

void MediaPipelineModuleServiceTests::playbackServiceWillFailToPlay()
{
    expectRequestFailure();
    EXPECT_CALL(m_playbackServiceMock, play(hardcodedSessionId)).WillOnce(Return(false));
}

void MediaPipelineModuleServiceTests::playbackServiceWillPause()
{
    expectRequestSuccess();
    EXPECT_CALL(m_playbackServiceMock, pause(hardcodedSessionId)).WillOnce(Return(true));
}

void MediaPipelineModuleServiceTests::playbackServiceWillFailToPause()
{
    expectRequestFailure();
    EXPECT_CALL(m_playbackServiceMock, pause(hardcodedSessionId)).WillOnce(Return(false));
}

void MediaPipelineModuleServiceTests::playbackServiceWillStop()
{
    expectRequestSuccess();
    EXPECT_CALL(m_playbackServiceMock, stop(hardcodedSessionId)).WillOnce(Return(true));
}

void MediaPipelineModuleServiceTests::playbackServiceWillFailToStop()
{
    expectRequestFailure();
    EXPECT_CALL(m_playbackServiceMock, stop(hardcodedSessionId)).WillOnce(Return(false));
}

void MediaPipelineModuleServiceTests::playbackServiceWillSetPosition()
{
    expectRequestSuccess();
    EXPECT_CALL(m_playbackServiceMock, setPosition(hardcodedSessionId, position)).WillOnce(Return(true));
}

void MediaPipelineModuleServiceTests::playbackServiceWillFailToSetPosition()
{
    expectRequestFailure();
    EXPECT_CALL(m_playbackServiceMock, setPosition(hardcodedSessionId, position)).WillOnce(Return(false));
}

void MediaPipelineModuleServiceTests::playbackServiceWillSetVideoWindow()
{
    expectRequestSuccess();
    EXPECT_CALL(m_playbackServiceMock, setVideoWindow(hardcodedSessionId, x, y, width, height)).WillOnce(Return(true));
}

void MediaPipelineModuleServiceTests::playbackServiceWillFailToSetVideoWindow()
{
    expectRequestFailure();
    EXPECT_CALL(m_playbackServiceMock, setVideoWindow(hardcodedSessionId, x, y, width, height)).WillOnce(Return(false));
}

void MediaPipelineModuleServiceTests::playbackServiceWillHaveData()
{
    expectRequestSuccess();
    EXPECT_CALL(m_playbackServiceMock, haveData(hardcodedSessionId, mediaSourceStatus, numFrames, requestId))
        .WillOnce(Return(true));
}

void MediaPipelineModuleServiceTests::playbackServiceWillFailToHaveData()
{
    expectRequestFailure();
    EXPECT_CALL(m_playbackServiceMock, haveData(hardcodedSessionId, mediaSourceStatus, numFrames, requestId))
        .WillOnce(Return(false));
}

void MediaPipelineModuleServiceTests::playbackServiceWillSetPlaybackRate()
{
    expectRequestSuccess();
    EXPECT_CALL(m_playbackServiceMock, setPlaybackRate(hardcodedSessionId, rate)).WillOnce(Return(true));
}

void MediaPipelineModuleServiceTests::playbackServiceWillFailToSetPlaybackRate()
{
    expectRequestFailure();
    EXPECT_CALL(m_playbackServiceMock, setPlaybackRate(hardcodedSessionId, rate)).WillOnce(Return(false));
}

void MediaPipelineModuleServiceTests::playbackServiceWillGetPosition()
{
    expectRequestSuccess();
    EXPECT_CALL(m_playbackServiceMock, getPosition(hardcodedSessionId, _))
        .WillOnce(Invoke(
            [&](int, std::int64_t &pos)
            {
                pos = position;
                return true;
            }));
}

void MediaPipelineModuleServiceTests::playbackServiceWillFailToGetPosition()
{
    expectRequestFailure();
    EXPECT_CALL(m_playbackServiceMock, getPosition(hardcodedSessionId, _)).WillOnce(Return(false));
}

void MediaPipelineModuleServiceTests::playbackServiceWillRenderFrame()
{
    expectRequestSuccess();
    EXPECT_CALL(m_playbackServiceMock, renderFrame(hardcodedSessionId)).WillOnce(Return(true));
}

void MediaPipelineModuleServiceTests::playbackServiceWillFailToRenderFrame()
{
    expectRequestFailure();
    EXPECT_CALL(m_playbackServiceMock, renderFrame(hardcodedSessionId)).WillOnce(Return(false));
}

void MediaPipelineModuleServiceTests::mediaClientWillSendPlaybackStateChangedEvent()
{
    EXPECT_CALL(*m_clientMock, sendEvent(PlaybackStateChangeEventMatcher(convertPlaybackState(playbackState))));
}

void MediaPipelineModuleServiceTests::mediaClientWillSendNetworkStateChangedEvent()
{
    EXPECT_CALL(*m_clientMock, sendEvent(NetworkStateChangeEventMatcher(convertNetworkState(networkState))));
}

void MediaPipelineModuleServiceTests::mediaClientWillSendNeedMediaDataEvent(int sessionId)
{
    std::shared_ptr<firebolt::rialto::ShmInfo> shmInfoPtr{std::make_shared<firebolt::rialto::ShmInfo>(shmInfo)};

    EXPECT_CALL(*m_clientMock, sendEvent(NeedMediaDataEventMatcher(sessionId, sourceId, needDataRequestId, frameCount,
                                                                   maxBytes, shmInfoPtr)));
}

void MediaPipelineModuleServiceTests::mediaClientWillSendPostionChangeEvent()
{
    EXPECT_CALL(*m_clientMock, sendEvent(PositionChangeEventMatcher(position)));
}

void MediaPipelineModuleServiceTests::mediaClientWillSendQosEvent()
{
    EXPECT_CALL(*m_clientMock, sendEvent(QosEventMatcher(sourceId, qosInfo.processed, qosInfo.dropped)));
}

void MediaPipelineModuleServiceTests::sendClientConnected()
{
    m_service->clientConnected(m_clientMock);
}

void MediaPipelineModuleServiceTests::sendClientDisconnected()
{
    m_service->clientDisconnected(m_clientMock);
}

int MediaPipelineModuleServiceTests::sendCreateSessionRequestAndReceiveResponse()
{
    firebolt::rialto::CreateSessionRequest request;
    firebolt::rialto::CreateSessionResponse response;

    // Set an invalid sessionId in the response
    response.set_session_id(-1);

    request.set_max_width(width);
    request.set_max_height(height);

    m_service->createSession(m_controllerMock.get(), &request, &response, m_closureMock.get());
    EXPECT_GE(response.session_id(), 0);

    return response.session_id();
}

void MediaPipelineModuleServiceTests::sendCreateSessionRequestAndExpectFailure()
{
    firebolt::rialto::CreateSessionRequest request;
    firebolt::rialto::CreateSessionResponse response;

    request.set_max_width(width);
    request.set_max_height(height);

    m_service->createSession(m_controllerMock.get(), &request, &response, m_closureMock.get());
}

void MediaPipelineModuleServiceTests::sendDestroySessionRequestAndReceiveResponse()
{
    firebolt::rialto::DestroySessionRequest request;
    firebolt::rialto::DestroySessionResponse response;

    request.set_session_id(hardcodedSessionId);

    m_service->destroySession(m_controllerMock.get(), &request, &response, m_closureMock.get());
}

void MediaPipelineModuleServiceTests::sendLoadRequestAndReceiveResponse()
{
    firebolt::rialto::LoadRequest request;
    firebolt::rialto::LoadResponse response;

    request.set_session_id(hardcodedSessionId);
    request.set_type(convertMediaType(mediaType));
    request.set_mime_type(mimeType);
    request.set_url(url);

    m_service->load(m_controllerMock.get(), &request, &response, m_closureMock.get());
}

void MediaPipelineModuleServiceTests::sendAttachSourceRequestAndReceiveResponse()
{
    firebolt::rialto::AttachSourceRequest request;
    firebolt::rialto::AttachSourceResponse response;

    request.set_session_id(hardcodedSessionId);
    request.set_media_type(convertProtoMediaSourceType(mediaSourceType));
    request.set_mime_type(mimeType);

    m_service->attachSource(m_controllerMock.get(), &request, &response, m_closureMock.get());
}

void MediaPipelineModuleServiceTests::sendAttachAudioSourceWithAdditionalDataRequestAndReceiveResponse()
{
    firebolt::rialto::AttachSourceRequest request;
    firebolt::rialto::AttachSourceResponse response;

    request.set_session_id(hardcodedSessionId);
    request.set_media_type(firebolt::rialto::ProtoMediaSourceType::AUDIO);
    request.set_mime_type(mimeType);
    request.mutable_audio_config()->set_number_of_channels(numberOfChannels);
    request.mutable_audio_config()->set_sample_rate(sampleRate);
    request.mutable_audio_config()->set_codec_specific_config(codecSpecificConfigStr);
    request.set_codec_data(codecData.data(), codecData.size());
    request.set_stream_format(convertStreamFormat(firebolt::rialto::StreamFormat::RAW));

    m_service->attachSource(m_controllerMock.get(), &request, &response, m_closureMock.get());
}

void MediaPipelineModuleServiceTests::sendPlayRequestAndReceiveResponse()
{
    firebolt::rialto::PlayRequest request;
    firebolt::rialto::PlayResponse response;

    request.set_session_id(hardcodedSessionId);

    m_service->play(m_controllerMock.get(), &request, &response, m_closureMock.get());
}

void MediaPipelineModuleServiceTests::sendPauseRequestAndReceiveResponse()
{
    firebolt::rialto::PauseRequest request;
    firebolt::rialto::PauseResponse response;

    request.set_session_id(hardcodedSessionId);

    m_service->pause(m_controllerMock.get(), &request, &response, m_closureMock.get());
}

void MediaPipelineModuleServiceTests::sendStopRequestAndReceiveResponse()
{
    firebolt::rialto::StopRequest request;
    firebolt::rialto::StopResponse response;

    request.set_session_id(hardcodedSessionId);

    m_service->stop(m_controllerMock.get(), &request, &response, m_closureMock.get());
}

void MediaPipelineModuleServiceTests::sendSetPositionRequestAndReceiveResponse()
{
    firebolt::rialto::SetPositionRequest request;
    firebolt::rialto::SetPositionResponse response;

    request.set_session_id(hardcodedSessionId);
    request.set_position(position);

    m_service->setPosition(m_controllerMock.get(), &request, &response, m_closureMock.get());
}

void MediaPipelineModuleServiceTests::sendGetPositionRequestAndReceiveResponse()
{
    firebolt::rialto::GetPositionRequest request;
    firebolt::rialto::GetPositionResponse response;

    request.set_session_id(hardcodedSessionId);

    m_service->getPosition(m_controllerMock.get(), &request, &response, m_closureMock.get());

    EXPECT_EQ(response.position(), position);
}

void MediaPipelineModuleServiceTests::sendGetPositionRequestAndReceiveResponseWithoutPositionMatch()
{
    firebolt::rialto::GetPositionRequest request;
    firebolt::rialto::GetPositionResponse response;

    request.set_session_id(hardcodedSessionId);

    m_service->getPosition(m_controllerMock.get(), &request, &response, m_closureMock.get());
}

void MediaPipelineModuleServiceTests::sendHaveDataRequestAndReceiveResponse()
{
    firebolt::rialto::HaveDataRequest request;
    firebolt::rialto::HaveDataResponse response;

    request.set_session_id(hardcodedSessionId);
    request.set_status(convertHaveDataRequestMediaSourceStatus(mediaSourceStatus));
    request.set_num_frames(numFrames);
    request.set_request_id(requestId);

    m_service->haveData(m_controllerMock.get(), &request, &response, m_closureMock.get());
}

void MediaPipelineModuleServiceTests::sendSetPlaybackRateRequestAndReceiveResponse()
{
    firebolt::rialto::SetPlaybackRateRequest request;
    firebolt::rialto::SetPlaybackRateResponse response;

    request.set_session_id(hardcodedSessionId);
    request.set_rate(rate);

    m_service->setPlaybackRate(m_controllerMock.get(), &request, &response, m_closureMock.get());
}

void MediaPipelineModuleServiceTests::sendSetVideoWindowRequestAndReceiveResponse()
{
    firebolt::rialto::SetVideoWindowRequest request;
    firebolt::rialto::SetVideoWindowResponse response;

    request.set_session_id(hardcodedSessionId);
    request.set_x(x);
    request.set_y(y);
    request.set_width(width);
    request.set_height(height);

    m_service->setVideoWindow(m_controllerMock.get(), &request, &response, m_closureMock.get());
}

void MediaPipelineModuleServiceTests::sendPlaybackStateChangedEvent()
{
    ASSERT_TRUE(m_mediaPipelineClient);
    m_mediaPipelineClient->notifyPlaybackState(playbackState);
}

void MediaPipelineModuleServiceTests::sendNetworkStateChangedEvent()
{
    ASSERT_TRUE(m_mediaPipelineClient);
    m_mediaPipelineClient->notifyNetworkState(networkState);
}

void MediaPipelineModuleServiceTests::sendNeedMediaDataEvent()
{
    ASSERT_TRUE(m_mediaPipelineClient);
    std::shared_ptr<firebolt::rialto::ShmInfo> shmInfoPtr{std::make_shared<firebolt::rialto::ShmInfo>(shmInfo)};
    m_mediaPipelineClient->notifyNeedMediaData(sourceId, frameCount, needDataRequestId, shmInfoPtr);
}

void MediaPipelineModuleServiceTests::sendPostionChangeEvent()
{
    ASSERT_TRUE(m_mediaPipelineClient);
    m_mediaPipelineClient->notifyPosition(position);
}

void MediaPipelineModuleServiceTests::sendQosEvent()
{
    ASSERT_TRUE(m_mediaPipelineClient);
    m_mediaPipelineClient->notifyQos(sourceId, qosInfo);
}

void MediaPipelineModuleServiceTests::expectRequestSuccess()
{
    EXPECT_CALL(*m_closureMock, Run());
}

void MediaPipelineModuleServiceTests::expectRequestFailure()
{
    EXPECT_CALL(*m_controllerMock, SetFailed(_));
    EXPECT_CALL(*m_closureMock, Run());
}

void MediaPipelineModuleServiceTests::sendRenderFrameRequestAndReceiveResponse()
{
    firebolt::rialto::RenderFrameRequest request;
    firebolt::rialto::RenderFrameResponse response;

    request.set_session_id(hardcodedSessionId);

    m_service->renderFrame(m_controllerMock.get(), &request, &response, m_closureMock.get());
}

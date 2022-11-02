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

#ifndef FIREBOLT_RIALTO_SERVER_MEDIA_KEY_SESSION_H_
#define FIREBOLT_RIALTO_SERVER_MEDIA_KEY_SESSION_H_

#include "IMediaKeySession.h"
#include "IOcdmSessionClient.h"
#include <atomic>
#include <memory>
#include <string>
#include <vector>

namespace firebolt::rialto::server
{
/**
 * @brief IMediaKeySessionFactory factory class definition.
 */
class MediaKeySessionFactory : public IMediaKeySessionFactory
{
public:
    MediaKeySessionFactory() = default;
    ~MediaKeySessionFactory() override = default;

    std::unique_ptr<IMediaKeySession> createMediaKeySession(const std::string &keySystem, int32_t keySessionId,
                                                            const IOcdmSystem &ocdmSystem, KeySessionType sessionType,
                                                            std::weak_ptr<IMediaKeysClient> client,
                                                            bool isLDL) const override;
};

/**
 * @brief The definition of the MediaKeySession.
 */
class MediaKeySession : public IMediaKeySession, public IOcdmSessionClient
{
public:
    /**
     * @brief The constructor.
     *
     * @param[in]  keySystem    : The key system for this session.
     * @param[in]  keySessionId : The key session id for this session.
     * @param[in]  ocdmSystem   : The ocdm system object to create the session on.
     * @param[in]  sessionType  : The session type.
     * @param[in]  client       : Client object for callbacks.
     * @param[in]  isLDL        : Is this an LDL.
     */
    MediaKeySession(const std::string &keySystem, int32_t keySessionId, const IOcdmSystem &ocdmSystem,
                    KeySessionType sessionType, std::weak_ptr<IMediaKeysClient> client, bool isLDL);

    /**
     * @brief Virtual destructor.
     */
    virtual ~MediaKeySession();

    MediaKeyErrorStatus generateRequest(InitDataType initDataType, const std::vector<uint8_t> &initData) override;

    MediaKeyErrorStatus loadSession() override;

    MediaKeyErrorStatus updateSession(const std::vector<uint8_t> &responseData) override;

    MediaKeyErrorStatus decrypt(GstBuffer *encrypted, GstBuffer *subSample, const uint32_t subSampleCount,
                                GstBuffer *IV, GstBuffer *keyId, uint32_t initWithLast15) override;

    MediaKeyErrorStatus closeKeySession() override;

    MediaKeyErrorStatus removeKeySession() override;

    MediaKeyErrorStatus getCdmKeySessionId(std::string &cdmKeySessionId) override;

    void onProcessChallenge(const char url[], const uint8_t challenge[], const uint16_t challengeLength) override;

    void onKeyUpdated(const uint8_t keyId[], const uint8_t keyIdLength) override;

    void onAllKeysUpdated() override;

    void onError(const char message[]) override;

private:
    /**
     * @brief KeySystem type of the MediaKeys.
     */
    const std::string m_keySystem;

    /**
     * @brief The key session id for this session.
     */
    const int32_t m_keySessionId;

    /**
     * @brief The key session type of this session.
     */
    const KeySessionType m_sessionType;

    /**
     * @brief The media keys client object.
     */
    std::weak_ptr<IMediaKeysClient> m_mediaKeysClient;

    /**
     * @brief The IOcdmSession instance.
     */
    std::unique_ptr<IOcdmSession> m_ocdmSession;

    /**
     * @brief Is the session LDL.
     */
    const bool m_isLDL;

    /**
     * @brief Is the ocdm session constructed.
     */
    bool m_isSessionConstructed;

    /**
     * @brief Set to true if generateRequest has complete and waiting for license response.
     */
    std::atomic<bool> m_licenseRequested;

    /**
     * @brief Store of the updated key statuses from a onKeyUpdated.
     */
    KeyStatusVector m_updatedKeyStatuses;
};
} // namespace firebolt::rialto::server

#endif // FIREBOLT_RIALTO_SERVER_MEDIA_KEY_SESSION_H_
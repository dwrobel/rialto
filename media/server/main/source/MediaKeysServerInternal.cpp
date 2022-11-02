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

#include <stdexcept>

#include "MediaKeysServerInternal.h"
#include "RialtoServerLogging.h"

namespace firebolt::rialto
{
std::shared_ptr<IMediaKeysFactory> IMediaKeysFactory::createFactory()
{
    return server::IMediaKeysServerInternalFactory::createFactory();
}
} // namespace firebolt::rialto

namespace firebolt::rialto::server
{
int32_t generateSessionId()
{
    static int32_t keySessionId{0};
    return keySessionId++;
}

std::shared_ptr<IMediaKeysServerInternalFactory> IMediaKeysServerInternalFactory::createFactory()
{
    std::shared_ptr<IMediaKeysServerInternalFactory> factory;

    try
    {
        factory = std::make_shared<MediaKeysServerInternalFactory>();
    }
    catch (const std::exception &e)
    {
        RIALTO_SERVER_LOG_ERROR("Failed to create the media keys factory, reason: %s", e.what());
    }

    return factory;
}

std::unique_ptr<IMediaKeys> MediaKeysServerInternalFactory::createMediaKeys(const std::string &keySystem) const
{
    RIALTO_SERVER_LOG_ERROR("This function can't be used by rialto server. Please use createMediaKeysServerInternal");
    return nullptr;
}

std::unique_ptr<IMediaKeysServerInternal>
MediaKeysServerInternalFactory::createMediaKeysServerInternal(const std::string &keySystem) const
{
    std::unique_ptr<IMediaKeysServerInternal> mediaKeys;
    try
    {
        mediaKeys = std::make_unique<server::MediaKeysServerInternal>(keySystem,
                                                                      server::IOcdmSystemFactory::createFactory(),
                                                                      server::IMediaKeySessionFactory::createFactory());
    }
    catch (const std::exception &e)
    {
        RIALTO_SERVER_LOG_ERROR("Failed to create the media keys, reason: %s", e.what());
    }

    return mediaKeys;
}
}; // namespace firebolt::rialto::server

namespace firebolt::rialto::server
{
MediaKeysServerInternal::MediaKeysServerInternal(const std::string &keySystem,
                                                 std::shared_ptr<IOcdmSystemFactory> ocdmSystemFactory,
                                                 std::shared_ptr<IMediaKeySessionFactory> mediaKeySessionFactory)
    : m_mediaKeySessionFactory(mediaKeySessionFactory), m_keySystem(keySystem)
{
    RIALTO_SERVER_LOG_DEBUG("entry:");

    m_ocdmSystem = ocdmSystemFactory->createOcdmSystem(keySystem);
    if (!m_ocdmSystem)
    {
        throw std::runtime_error("Ocdm system could not be created");
    }
}

MediaKeysServerInternal::~MediaKeysServerInternal()
{
    RIALTO_SERVER_LOG_DEBUG("entry:");
}

MediaKeyErrorStatus MediaKeysServerInternal::selectKeyId(int32_t keySessionId, const std::vector<uint8_t> &keyId)
{
    RIALTO_SERVER_LOG_ERROR("Not Implemented");

    return MediaKeyErrorStatus::FAIL;
}

bool MediaKeysServerInternal::containsKey(int32_t keySessionId, const std::vector<uint8_t> &keyId)
{
    RIALTO_SERVER_LOG_ERROR("Not Implemented");

    return false;
}

MediaKeyErrorStatus MediaKeysServerInternal::createKeySession(KeySessionType sessionType,
                                                              std::weak_ptr<IMediaKeysClient> client, bool isLDL,
                                                              int32_t &keySessionId)
{
    RIALTO_SERVER_LOG_DEBUG("entry:");

    int32_t keySessionIdTemp = generateSessionId();
    std::unique_ptr<IMediaKeySession> mediaKeySession =
        m_mediaKeySessionFactory->createMediaKeySession(m_keySystem, keySessionIdTemp, *m_ocdmSystem, sessionType,
                                                        client, isLDL);
    if (!mediaKeySession)
    {
        RIALTO_SERVER_LOG_ERROR("Failed to create a new media key session");
        return MediaKeyErrorStatus::FAIL;
    }
    keySessionId = keySessionIdTemp;
    m_mediaKeySessions.emplace(std::make_pair(keySessionId, std::move(mediaKeySession)));

    return MediaKeyErrorStatus::OK;
}

MediaKeyErrorStatus MediaKeysServerInternal::generateRequest(int32_t keySessionId, InitDataType initDataType,
                                                             const std::vector<uint8_t> &initData)
{
    RIALTO_SERVER_LOG_DEBUG("entry:");

    auto sessionIter = m_mediaKeySessions.find(keySessionId);
    if (sessionIter == m_mediaKeySessions.end())
    {
        RIALTO_SERVER_LOG_ERROR("Failed to find the session");
        return MediaKeyErrorStatus::BAD_SESSION_ID;
    }

    MediaKeyErrorStatus status = sessionIter->second->generateRequest(initDataType, initData);
    if (MediaKeyErrorStatus::OK != status)
    {
        RIALTO_SERVER_LOG_ERROR("Failed to generate request for the key session");
        return status;
    }

    return status;
}

MediaKeyErrorStatus MediaKeysServerInternal::loadSession(int32_t keySessionId)
{
    RIALTO_SERVER_LOG_DEBUG("entry:");

    auto sessionIter = m_mediaKeySessions.find(keySessionId);
    if (sessionIter == m_mediaKeySessions.end())
    {
        RIALTO_SERVER_LOG_ERROR("Failed to find the session");
        return MediaKeyErrorStatus::BAD_SESSION_ID;
    }

    MediaKeyErrorStatus status = sessionIter->second->loadSession();
    if (MediaKeyErrorStatus::OK != status)
    {
        RIALTO_SERVER_LOG_ERROR("Failed to load the session");
        return status;
    }

    return status;
}

MediaKeyErrorStatus MediaKeysServerInternal::updateSession(int32_t keySessionId, const std::vector<uint8_t> &responseData)
{
    RIALTO_SERVER_LOG_DEBUG("entry:");

    auto sessionIter = m_mediaKeySessions.find(keySessionId);
    if (sessionIter == m_mediaKeySessions.end())
    {
        RIALTO_SERVER_LOG_ERROR("Failed to find the session");
        return MediaKeyErrorStatus::BAD_SESSION_ID;
    }

    MediaKeyErrorStatus status = sessionIter->second->updateSession(responseData);
    if (MediaKeyErrorStatus::OK != status)
    {
        RIALTO_SERVER_LOG_ERROR("Failed to update the session");
        return status;
    }

    return status;
}

MediaKeyErrorStatus MediaKeysServerInternal::setDrmHeader(int32_t keySessionId, const std::vector<uint8_t> &requestData)
{
    RIALTO_SERVER_LOG_ERROR("Not Implemented");

    return MediaKeyErrorStatus::FAIL;
}

MediaKeyErrorStatus MediaKeysServerInternal::closeKeySession(int32_t keySessionId)
{
    RIALTO_SERVER_LOG_DEBUG("entry:");

    auto sessionIter = m_mediaKeySessions.find(keySessionId);
    if (sessionIter == m_mediaKeySessions.end())
    {
        RIALTO_SERVER_LOG_ERROR("Failed to find the session");
        return MediaKeyErrorStatus::BAD_SESSION_ID;
    }

    MediaKeyErrorStatus status = sessionIter->second->closeKeySession();
    if (MediaKeyErrorStatus::OK != status)
    {
        RIALTO_SERVER_LOG_ERROR("Failed to close the key session");
        return status;
    }

    m_mediaKeySessions.erase(sessionIter);

    return status;
}

MediaKeyErrorStatus MediaKeysServerInternal::removeKeySession(int32_t keySessionId)
{
    RIALTO_SERVER_LOG_DEBUG("entry:");

    auto sessionIter = m_mediaKeySessions.find(keySessionId);
    if (sessionIter == m_mediaKeySessions.end())
    {
        RIALTO_SERVER_LOG_ERROR("Failed to find the session");
        return MediaKeyErrorStatus::BAD_SESSION_ID;
    }

    MediaKeyErrorStatus status = sessionIter->second->removeKeySession();
    if (MediaKeyErrorStatus::OK != status)
    {
        RIALTO_SERVER_LOG_ERROR("Failed to remove the key session");
        return status;
    }

    return status;
}

MediaKeyErrorStatus MediaKeysServerInternal::deleteDrmStore()
{
    RIALTO_SERVER_LOG_ERROR("Not Implemented");

    return MediaKeyErrorStatus::FAIL;
}

MediaKeyErrorStatus MediaKeysServerInternal::deleteKeyStore()
{
    RIALTO_SERVER_LOG_ERROR("Not Implemented");

    return MediaKeyErrorStatus::FAIL;
}

MediaKeyErrorStatus MediaKeysServerInternal::getDrmStoreHash(std::vector<unsigned char> &drmStoreHash)
{
    RIALTO_SERVER_LOG_ERROR("Not Implemented");

    return MediaKeyErrorStatus::FAIL;
}

MediaKeyErrorStatus MediaKeysServerInternal::getKeyStoreHash(std::vector<unsigned char> &keyStoreHash)
{
    RIALTO_SERVER_LOG_ERROR("Not Implemented");

    return MediaKeyErrorStatus::FAIL;
}

MediaKeyErrorStatus MediaKeysServerInternal::getLdlSessionsLimit(uint32_t &ldlLimit)
{
    RIALTO_SERVER_LOG_ERROR("Not Implemented");

    return MediaKeyErrorStatus::FAIL;
}

MediaKeyErrorStatus MediaKeysServerInternal::getLastDrmError(uint32_t &errorCode)
{
    RIALTO_SERVER_LOG_ERROR("Not Implemented");

    return MediaKeyErrorStatus::FAIL;
}

MediaKeyErrorStatus MediaKeysServerInternal::getDrmTime(uint64_t &drmTime)
{
    RIALTO_SERVER_LOG_ERROR("Not Implemented");

    return MediaKeyErrorStatus::FAIL;
}

MediaKeyErrorStatus MediaKeysServerInternal::getCdmKeySessionId(int32_t keySessionId, std::string &cdmKeySessionId)
{
    RIALTO_SERVER_LOG_DEBUG("entry:");

    auto sessionIter = m_mediaKeySessions.find(keySessionId);
    if (sessionIter == m_mediaKeySessions.end())
    {
        RIALTO_SERVER_LOG_ERROR("Failed to find the session");
        return MediaKeyErrorStatus::BAD_SESSION_ID;
    }

    MediaKeyErrorStatus status = sessionIter->second->getCdmKeySessionId(cdmKeySessionId);
    if (MediaKeyErrorStatus::OK != status)
    {
        RIALTO_SERVER_LOG_ERROR("Failed to get cdm key session id");
        return status;
    }

    return status;
}

MediaKeyErrorStatus MediaKeysServerInternal::decrypt(int32_t keySessionId, GstBuffer *encrypted, GstBuffer *subSample,
                                                     const uint32_t subSampleCount, GstBuffer *IV, GstBuffer *keyId,
                                                     uint32_t initWithLast15)
{
    auto sessionIter = m_mediaKeySessions.find(keySessionId);
    if (sessionIter == m_mediaKeySessions.end())
    {
        RIALTO_SERVER_LOG_ERROR("Failed to find the session");
        return MediaKeyErrorStatus::BAD_SESSION_ID;
    }

    MediaKeyErrorStatus status =
        sessionIter->second->decrypt(encrypted, subSample, subSampleCount, IV, keyId, initWithLast15);
    if (MediaKeyErrorStatus::OK != status)
    {
        RIALTO_SERVER_LOG_ERROR("Failed to decrypt buffer.");
        return status;
    }

    return status;
}

bool MediaKeysServerInternal::hasSession(int32_t keySessionId) const
{
    return m_mediaKeySessions.find(keySessionId) != m_mediaKeySessions.end();
}
}; // namespace firebolt::rialto::server
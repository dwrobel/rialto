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

#include "tasks/Stop.h"
#include "IGstPlayerPrivate.h"
#include "PlayerContext.h"
#include "RialtoServerLogging.h"

namespace firebolt::rialto::server
{
Stop::Stop(PlayerContext &context, IGstPlayerPrivate &player) : m_context{context}, m_player{player}
{
    RIALTO_SERVER_LOG_DEBUG("Constructing Stop");
}

Stop::~Stop()
{
    RIALTO_SERVER_LOG_DEBUG("Stop finished");
}

void Stop::execute() const
{
    RIALTO_SERVER_LOG_DEBUG("Executing Stop");
    m_player.stopPositionReportingAndCheckAudioUnderflowTimer();
    m_player.changePipelineState(GST_STATE_NULL);
    m_context.videoNeedData = false;
    m_context.audioNeedData = false;
}
} // namespace firebolt::rialto::server

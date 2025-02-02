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

#include "tasks/Pause.h"
#include "IGstPlayerPrivate.h"
#include "RialtoServerLogging.h"

namespace firebolt::rialto::server
{
Pause::Pause(IGstPlayerPrivate &player) : m_player{player}
{
    RIALTO_SERVER_LOG_DEBUG("Constructing Pause");
}

Pause::~Pause()
{
    RIALTO_SERVER_LOG_DEBUG("Pause finished");
}

void Pause::execute() const
{
    RIALTO_SERVER_LOG_DEBUG("Executing Pause");
    m_player.stopPositionReportingAndCheckAudioUnderflowTimer();
    m_player.changePipelineState(GST_STATE_PAUSED);
}
} // namespace firebolt::rialto::server

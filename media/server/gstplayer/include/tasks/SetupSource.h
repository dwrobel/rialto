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

#ifndef FIREBOLT_RIALTO_SERVER_SETUP_SOURCE_H_
#define FIREBOLT_RIALTO_SERVER_SETUP_SOURCE_H_

#include "IGstPlayerPrivate.h"
#include "IPlayerTask.h"
#include "PlayerContext.h"
#include <gst/gst.h>

namespace firebolt::rialto::server
{
class SetupSource : public IPlayerTask
{
public:
    SetupSource(PlayerContext &context, IGstPlayerPrivate &player, GstElement *source);
    ~SetupSource() override;
    void execute() const override;

private:
    PlayerContext &m_context;
    IGstPlayerPrivate &m_player;
    GstElement *m_source;
};
} // namespace firebolt::rialto::server

#endif // FIREBOLT_RIALTO_SERVER_SETUP_SOURCE_H_

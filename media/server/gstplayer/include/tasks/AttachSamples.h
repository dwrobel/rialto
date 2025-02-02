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

#ifndef FIREBOLT_RIALTO_SERVER_ATTACH_SAMPLES_H_
#define FIREBOLT_RIALTO_SERVER_ATTACH_SAMPLES_H_

#include "IGstPlayerPrivate.h"
#include "IMediaPipeline.h"
#include "IPlayerTask.h"
#include "PlayerContext.h"
#include <gst/gst.h>
#include <vector>

namespace firebolt::rialto::server
{
class AttachSamples : public IPlayerTask
{
public:
    AttachSamples(PlayerContext &context, IGstPlayerPrivate &player,
                  const IMediaPipeline::MediaSegmentVector &mediaSegments);
    ~AttachSamples() override;
    void execute() const override;

private:
    struct AudioData
    {
        GstBuffer *buffer;
        int32_t rate;
        int32_t channels;
    };
    struct VideoData
    {
        GstBuffer *buffer;
        int32_t width;
        int32_t height;
    };

    PlayerContext &m_context;
    IGstPlayerPrivate &m_player;
    std::vector<AudioData> m_audioData;
    std::vector<VideoData> m_videoData;
};
} // namespace firebolt::rialto::server

#endif // FIREBOLT_RIALTO_SERVER_ATTACH_SAMPLES_H_

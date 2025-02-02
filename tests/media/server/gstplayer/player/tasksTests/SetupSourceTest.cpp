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

#include "tasks/SetupSource.h"
#include "GstPlayerPrivateMock.h"
#include "PlayerContext.h"
#include <gst/gst.h>
#include <gtest/gtest.h>

using testing::StrictMock;

class SetupSourceTest : public testing::Test
{
protected:
    firebolt::rialto::server::PlayerContext m_context{};
    StrictMock<firebolt::rialto::server::GstPlayerPrivateMock> m_gstPlayer;
    GstElement m_element{};
};

TEST_F(SetupSourceTest, shouldSetupSource)
{
    firebolt::rialto::server::SetupSource task{m_context, m_gstPlayer, &m_element};
    EXPECT_CALL(m_gstPlayer, scheduleSourceSetupFinish());
    task.execute();
    EXPECT_EQ(m_context.source, &m_element);
}

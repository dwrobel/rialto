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

#ifndef FIREBOLT_RIALTO_SERVER_GST_DECRYPTOR_ELEMENT_FACTORY_H_
#define FIREBOLT_RIALTO_SERVER_GST_DECRYPTOR_ELEMENT_FACTORY_H_

#include "IGstDecryptorElementFactory.h"

namespace firebolt::rialto::server
{
/**
 * @brief IGstDecryptorElement factory class definition.
 */
class GstDecryptorElementFactory : public IGstDecryptorElementFactory
{
public:
    GstDecryptorElementFactory() = default;
    ~GstDecryptorElementFactory() override = default;

    GstElement *createDecryptorElement(const gchar *name,
                                       firebolt::rialto::server::IDecryptionService *decryptionService) const override;
};

}; // namespace firebolt::rialto::server

#endif // FIREBOLT_RIALTO_SERVER_GST_DECRYPTOR_ELEMENT_FACTORY_H_

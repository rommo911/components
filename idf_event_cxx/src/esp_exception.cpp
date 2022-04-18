// Copyright 2020 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//         http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifdef __cpp_exceptions

#include "esp_exception.hpp"
#include "string.h"
ESPException::ESPException(esp_err_t error, const char *e_tag) : error(error), errStr(""), eTAG(e_tag) {}
ESPException::ESPException(const char *err_str, const char *e_tag) : error(ESP_OK), errStr((err_str)), eTAG(e_tag) {}

const char *ESPException::what() const noexcept
{
    std::string ret = eTAG;
    if (error != ESP_OK)
        ret += esp_err_to_name(error);
    if (strlen(errStr) > 1)
        ret += errStr;
    return ret.c_str();
}

#endif // __cpp_exceptions

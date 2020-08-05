/**
 * Copyright 2020 Huawei Technologies Co., Ltd
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

#include "common/duplex_pipe.h"

#include <iostream>
#include <vector>

namespace mindspore {
int DuplexPipe::Open(std::initializer_list<std::string> arg_list, bool append_fds) {
  DP_EXCEPTION << "Not support for Windows by now.";
}

void DuplexPipe::Write(const std::string &buf, bool flush) { DP_EXCEPTION << "Not support for Windows by now."; }

std::string DuplexPipe::Read() { DP_EXCEPTION << "Not support for Windows by now."; }

void DuplexPipe::WriteWithStdout(const std::string &buf, bool flush) {
  DP_EXCEPTION << "Not support for Windows by now.";
}

std::string DuplexPipe::ReadWithStdin() { DP_EXCEPTION << "Not support for Windows by now."; }

DuplexPipe &DuplexPipe::operator<<(const std::string &buf) { DP_EXCEPTION << "Not support for Windows by now."; }

DuplexPipe &DuplexPipe::operator>>(std::string &buf) { DP_EXCEPTION << "Not support for Windows by now."; }

void DuplexPipe::Close() { DP_EXCEPTION << "Not support for Windows by now."; }

void DuplexPipe::Alarm::Set(std::shared_ptr<DuplexPipe> dp, unsigned int interval_secs) {
  DP_EXCEPTION << "Not support for Windows by now.";
}

void DuplexPipe::Alarm::Cancel() { DP_EXCEPTION << "Not support for Windows by now."; }
}  // namespace mindspore
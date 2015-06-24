// Copyright 2015 National ICT Australia Limited (NICTA)
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef STOPWATCH_DOT_H
#define STOPWATCH_DOT_H

#include <chrono>

/// @brief Simple stopwatch style timer, using std::chrono.
/// @ingroup Utilities
class Stopwatch
{
    public:
        Stopwatch() : running_(false), dur_(std::chrono::duration<double>::zero()) {}
        void start() {running_ = true; start_ = std::chrono::system_clock::now();}
        void stop() {if (running_) dur_ += std::chrono::system_clock::now() - start_; running_ = false;}
        void reset() {dur_ = std::chrono::duration<double>::zero();}
        double seconds() {return dur_.count();}
    private:
        bool running_;
        std::chrono::time_point<std::chrono::system_clock> start_;
        std::chrono::duration<double> dur_;
};

#endif // STOPWATCH_DOT_H
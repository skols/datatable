//------------------------------------------------------------------------------
// Copyright 2018 H2O.ai
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
//------------------------------------------------------------------------------
#include <chrono>
#include <string>

class TimeIt {
  public:
    using clock = std::chrono::steady_clock;
    using ms = std::chrono::milliseconds;
    static constexpr double time_factor = 1000;

    TimeIt(std::string message_in) {
      t0 = clock::now();
      message = message_in;
    }

    ~TimeIt() {
      clock::time_point t1 = clock::now();
      int64_t ms_duration = std::chrono::duration_cast<ms>(t1 - t0).count();
      printf("Thread %zu [%s] %.3f [ms]\n",
             dt::this_thread_index(),
             message.c_str(),
             ms_duration / time_factor
            );
    }

  private:
    clock::time_point t0;
    std::string message;
};

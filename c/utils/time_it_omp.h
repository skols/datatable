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
#include <iostream>
#include <string>
#include <stack>
#include <mutex>


class TimeItOmp {
  private:
    using clock = std::chrono::steady_clock;
    using ms = std::chrono::microseconds;
    clock::time_point t0;
    std::string message;
    size_t thread_index;
    static std::stack<std::string> indents;
    static std::mutex mtx;

  public:
    static constexpr double time_factor = 1000000;
    static constexpr size_t MASTER = 0; //size_t(-1);

    TimeItOmp(std::string message_in) {
      t0 = clock::now();
      message = std::move(message_in);
      // thread_index = dt::this_thread_index();
      thread_index = static_cast<size_t>(omp_get_thread_num());

      if (indents.empty()) {
        xassert(thread_index == MASTER);
        indents.push("");
      } else if (thread_index == MASTER && omp_get_num_threads() == 1) {
        indents.push(indents.top() + "  ");
      }

      #pragma omp critical
      if (thread_index == MASTER && omp_get_num_threads() == 1) {
        std::cout << indents.top() << "Thread [M] start "
                  << message << "\n";
      } else {
        // std::lock_guard<std::mutex> lock(mtx);
        std::cout << indents.top() << "  Thread [" << thread_index
                  << "] start " << message << "\n";
      }
    }


    void time() {
      clock::time_point t1 = clock::now();
      int64_t ms_duration = std::chrono::duration_cast<ms>(t1 - t0).count();
      #pragma omp critical
      {
        if (thread_index == MASTER && omp_get_num_threads() == 1) {
          std::cout << indents.top() << "Thread [M] run   " << message << " "
                    << ms_duration / time_factor << " [s]\n";
        } else {
          // std::lock_guard<std::mutex> lock(mtx);
          std::cout << indents.top() << "  Thread [" << omp_get_thread_num()
                    << "] run   " << message << " "
                    << ms_duration / time_factor << " [s]\n";
        }
      }
    }


    ~TimeItOmp() {
      clock::time_point t1 = clock::now();
      int64_t ms_duration = std::chrono::duration_cast<ms>(t1 - t0).count();
      // printf("Thread %zu: %s %.3f [s]\n",
      //        dt::this_thread_index(),
      //        message.c_str(),
      //        ms_duration / time_factor
      //       );
      #pragma omp critical
      {
        if (thread_index == MASTER && omp_get_num_threads() == 1) {
          std::cout << indents.top() << "Thread [M] fini  " << message << " "
                    << ms_duration / time_factor << " [s]\n";
          indents.pop();
        } else {
          // std::lock_guard<std::mutex> lock(mtx);
          std::cout << indents.top() << "  Thread [" << thread_index
                    << "] fini  " << message << " "
                    << ms_duration / time_factor << " [s]\n";
        }
      }
    }

};

std::stack<std::string> TimeItOmp::indents;
std::mutex TimeItOmp::mtx;


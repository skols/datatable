//------------------------------------------------------------------------------
// Copyright 2019 H2O.ai
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
//------------------------------------------------------------------------------
// Original source taken from:
//
//   https://github.com/preshing/turf/blob/master/turf/Affinity.h
//
// which was ditributed under the following license:
//
//   Copyright (c) 2015 Jeff Preshing
//
//   This software is provided 'as-is', without any express or implied
//   warranty. In no event will the authors be held liable for any damages
//   arising from the use of this software.
//
//   Permission is granted to anyone to use this software for any purpose,
//   including commercial applications, and to alter it and redistribute it
//   freely, subject to the following restrictions:
//
//   1. The origin of this software must not be misrepresented; you must not
//      claim that you wrote the original software. If you use this software
//      in a product, an acknowledgement in the product documentation would be
//      appreciated but is not required.
//   2. Altered source versions must be plainly marked as such, and must not be
//      misrepresented as being the original software.
//   3. This notice may not be removed or altered from any source distribution.
//
//------------------------------------------------------------------------------
// This file was modified from the original.
//------------------------------------------------------------------------------
#ifndef dt_PARALLEL_AFFINITY_h
#define dt_PARALLEL_AFFINITY_h

#include "utils/assert.h"

// namespace dt {


//------------------------------------------------------------------------------
// Affinity (Windows)
//------------------------------------------------------------------------------
#if defined(_WIN32)

class Affinity {
  private:
    typedef ULONG_PTR AffinityMask;
    static const int MaxHWThreads = sizeof(AffinityMask) * 8;

    bool m_isAccurate;
    int m_numPhysicalCores;
    int m_numHWThreads;
    AffinityMask m_physicalCoreMasks[MaxHWThreads];

  public:

    bool isAccurate() const {
        return m_isAccurate;
    }

    size_t getNumPhysicalCores() const {
        return static_cast<size_t>(m_numPhysicalCores);
    }

    size_t getNumHWThreads() const {
        return static_cast<size_t>(m_numHWThreads);
    }

    size_t getNumHWThreadsForCore(int core) const {
        xassert(core < m_numPhysicalCores);
        return static_cast<size_t>(util::countSetBits(m_physicalCoreMasks[core]));
    }

    Affinity_Win32() {
      m_isAccurate = false;
      m_numPhysicalCores = 0;
      m_numHWThreads = 0;
      for (int i = 0; i < MaxHWThreads; i++)
          m_physicalCoreMasks[i] = 0;

      SYSTEM_LOGICAL_PROCESSOR_INFORMATION* startProcessorInfo = NULL;
      DWORD length = 0;
      BOOL result = GetLogicalProcessorInformation(NULL, &length);
      if (result == FALSE && GetLastError() == ERROR_INSUFFICIENT_BUFFER && length > 0) {
          startProcessorInfo = (SYSTEM_LOGICAL_PROCESSOR_INFORMATION*) TURF_HEAP.alloc(length);
          result = GetLogicalProcessorInformation(startProcessorInfo, &length);
          if (result == TRUE) {
              m_isAccurate = true;
              m_numPhysicalCores = 0;
              m_numHWThreads = 0;
              SYSTEM_LOGICAL_PROCESSOR_INFORMATION* endProcessorInfo =
                  (SYSTEM_LOGICAL_PROCESSOR_INFORMATION*) TURF_PTR_OFFSET(startProcessorInfo, length);
              for (SYSTEM_LOGICAL_PROCESSOR_INFORMATION* processorInfo = startProcessorInfo; processorInfo < endProcessorInfo;
                   processorInfo++) {
                  if (processorInfo->Relationship == RelationProcessorCore) {
                      int hwt = util::countSetBits(processorInfo->ProcessorMask);
                      if (hwt == 0)
                          m_isAccurate = false;
                      else if (m_numHWThreads + hwt > MaxHWThreads)
                          m_isAccurate = false;
                      else {
                          xassert(m_numPhysicalCores <= m_numHWThreads && m_numHWThreads < MaxHWThreads);
                          m_physicalCoreMasks[m_numPhysicalCores] = processorInfo->ProcessorMask;
                          m_numPhysicalCores++;
                          m_numHWThreads += hwt;
                      }
                  }
              }
          }
      }

      xassert(m_numPhysicalCores <= m_numHWThreads);
      if (m_numHWThreads == 0) {
          m_isAccurate = false;
          m_numPhysicalCores = 1;
          m_numHWThreads = 1;
          m_physicalCoreMasks[0] = 1;
      }
    }

    bool setAffinity(int core, int hwThread) {
        xassert(hwThread < getNumHWThreadsForCore(core));
        AffinityMask availableMask = m_physicalCoreMasks[core];
        for (AffinityMask checkMask = 1;; checkMask <<= 1) {
            if ((availableMask & checkMask) != 0) {
                if (hwThread-- == 0) {
                    DWORD_PTR result = SetThreadAffinityMask(GetCurrentThread(), checkMask);
                    return (result != 0);
                }
            }
        }
    }


};


//------------------------------------------------------------------------------
// Affinity (Apple iOS and OSX)
//------------------------------------------------------------------------------
#elif defined(__MACH__)

#include <sys/sysctl.h>
#include <mach/mach_init.h>
#include <mach/thread_policy.h>
#include <mach/thread_act.h>

class Affinity {
private:
  bool m_isAccurate;
  size_t m_numHWThreads;
  size_t m_numPhysicalCores;
  size_t m_hwThreadsPerCore;

public:
  Affinity() : m_isAccurate(false), m_numHWThreads(1), m_numPhysicalCores(1), m_hwThreadsPerCore(1) {
    int count;
    // Get # of HW threads
    size_t countLen = sizeof(count);
    if (sysctlbyname("hw.logicalcpu", &count, &countLen, nullptr, 0) == 0) {
      if (count > 0) {
        m_numHWThreads = static_cast<size_t>(count);
        // Get # of physical cores
        countLen = sizeof(count);
        if (sysctlbyname("hw.physicalcpu", &count, &countLen, nullptr, 0) == 0) {
          if (count > 0) {
            m_numPhysicalCores = static_cast<size_t>(count);
            m_hwThreadsPerCore = m_numHWThreads / static_cast<size_t>(count);
            if (m_hwThreadsPerCore < 1) m_hwThreadsPerCore = 1;
            else m_isAccurate = true;
          }
        }
      }
    }
  }

  bool isAccurate() const {
    return m_isAccurate;
  }

  size_t getNumPhysicalCores() const {
    return m_numPhysicalCores;
  }

  size_t getNumHWThreads() const {
    return m_numHWThreads;
  }

  size_t getNumHWThreadsForCore(int core) const {
    xassert(static_cast<size_t>(core) < m_numPhysicalCores);
    return m_hwThreadsPerCore;
  }

  bool setAffinity(int core, int hwThread) {
    xassert(static_cast<size_t>(core) < m_numPhysicalCores);
    xassert(static_cast<size_t>(hwThread) < m_hwThreadsPerCore);
    size_t index = static_cast<size_t>(core) * m_hwThreadsPerCore + static_cast<size_t>(hwThread);
    thread_t thread = mach_thread_self();
    thread_affinity_policy_data_t policyInfo = {static_cast<integer_t>(index)};
    // Note: The following returns KERN_NOT_SUPPORTED on iOS. (Tested on iOS
    // 9.2.)
    kern_return_t result = thread_policy_set(thread, THREAD_AFFINITY_POLICY, reinterpret_cast<thread_policy_t>(&policyInfo), THREAD_AFFINITY_POLICY_COUNT);
    return (result == KERN_SUCCESS);
  }
};


//------------------------------------------------------------------------------
// Affinity (Linux)
//------------------------------------------------------------------------------
#elif defined(__unix__)

class Affinity {
private:
    struct CoreInfo {
        std::vector<size_t> hwThreadIndexToLogicalProcessor;
    };
    bool m_isAccurate;
    std::vector<CoreInfo> m_coreIndexToInfo;
    size_t m_numHWThreads;

    struct CoreInfoCollector {
        struct CoreID {
            int physical;
            int core;
            CoreID() : physical(-1), core(-1) {
            }
            bool operator<(const CoreID& other) const {
                if (physical != other.physical)
                    return physical < other.physical;
                return core < other.core;
            }
        };

        int logicalProcessor;
        CoreID coreID;
        std::map<CoreID, size_t> coreIDToIndex;

        CoreInfoCollector() : logicalProcessor(-1) {
        }

        void flush(Affinity& affinity) {
            if (logicalProcessor >= 0) {
                if (coreID.physical < 0 && coreID.core < 0) {
                    // On PowerPC Linux 3.2.0-4, /proc/cpuinfo outputs "processor", but not "physical id" or "core id".
                    // Emulate a single physical CPU with N cores:
                    coreID.physical = 0;
                    coreID.core = logicalProcessor;
                }
                std::map<CoreID, size_t>::iterator iter = coreIDToIndex.find(coreID);
                size_t coreIndex;
                if (iter == coreIDToIndex.end()) {
                    coreIndex = (size_t) affinity.m_coreIndexToInfo.size();
                    affinity.m_coreIndexToInfo.resize(coreIndex + 1);
                    coreIDToIndex[coreID] = coreIndex;
                } else {
                    coreIndex = iter->second;
                }
                affinity.m_coreIndexToInfo[coreIndex].hwThreadIndexToLogicalProcessor.push_back(logicalProcessor);
                affinity.m_numHWThreads++;
            }
            logicalProcessor = -1;
            coreID = CoreID();
        }
    };

public:

    Affinity::Affinity() : m_isAccurate(false), m_numHWThreads(0) {
        std::ifstream f("/proc/cpuinfo");
        if (f.is_open()) {
            CoreInfoCollector collector;
            while (!f.eof()) {
                std::string line;
                std::getline(f, line);
                size_t colon = line.find_first_of(':');
                if (colon != std::string::npos) {
                    size_t endLabel = line.find_last_not_of(" \t", colon > 0 ? colon - 1 : 0);
                    std::string label = line.substr(0, endLabel != std::string::npos ? endLabel + 1 : 0);
                    int value;
                    bool isIntegerValue = (sscanf(line.c_str() + colon + 1, " %d", &value) > 0);
                    if (isIntegerValue && label == "processor") {
                        collector.flush(*this);
                        collector.logicalProcessor = (int) value;
                    } else if (isIntegerValue && label == "physical id") {
                        collector.coreID.physical = (int) value;
                    } else if (isIntegerValue && label == "core id") {
                        collector.coreID.core = (int) value;
                    }
                }
            }
            collector.flush(*this);
        }
        m_isAccurate = (m_numHWThreads > 0);
        if (!m_isAccurate) {
            m_coreIndexToInfo.resize(1);
            m_coreIndexToInfo[0].hwThreadIndexToLogicalProcessor.push_back(0);
            m_numHWThreads = 1;
        }
    }

    bool isAccurate() const {
        return m_isAccurate;
    }

    size_t getNumPhysicalCores() const {
        return m_coreIndexToInfo.size();
    }

    size_t getNumHWThreads() const {
        return m_numHWThreads;
    }

    size_t getNumHWThreadsForCore(int core) const {
        return m_coreIndexToInfo[core].hwThreadIndexToLogicalProcessor.size();
    }

    bool setAffinity(int core, int hwThread) {
        size_t logicalProcessor = m_coreIndexToInfo[core].hwThreadIndexToLogicalProcessor[hwThread];
        cpu_set_t cpuSet;
        CPU_ZERO(&cpuSet);
        CPU_SET(logicalProcessor, &cpuSet);
        int rc = pthread_setaffinity_np(pthread_self(), sizeof(cpuSet), &cpuSet);
        return (rc == 0);
    }
};



#else
#error Unsupported platform!
#endif


// } // namespace dt
#endif

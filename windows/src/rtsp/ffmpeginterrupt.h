#pragma once

#include <atomic>
#include <chrono>

#include "logger.h"

namespace FFmpegInterrupt
{
    const int64_t TIMEOUT_MS = 3000;

    struct State
    {
        std::atomic<int64_t> lastActivityTime;
        std::atomic<bool> stopRequested;
    };

    static int64_t GetTimeMs() 
    {
        using namespace std::chrono;
        return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
    }

    /*
    * FFmpeg interrupt callback that is called internally by the library during blocking calls
    * If return value is 0 => continue waiting, otherwise ABORT
    */
    static int Callback(void* ctx)
    {
        State* state = static_cast<State*>(ctx);

        // Abort if a stop is requested
        if (state->stopRequested) 
        {
            logger << "[RTSP INTERRUPT] Aborting: stop requested\n";
            return 1;
        }

        const int64_t idle = GetTimeMs() - state->lastActivityTime;
        if (idle > TIMEOUT_MS)
        {
            logger << "[RTSP INTERRUPT] Aborting: idle for " << idle << " ms\n";
            return 1;
        }

        return 0;
    }
}
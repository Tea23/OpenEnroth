#include "SdlLogger.h"

#include <cassert>
#include <ctime>

#include <SDL_log.h>

#include "SdlEnumTranslation.h"

// This one is not in `SdlEnumTranslation.h` because it's closely tied to the implementation below.
static SDL_LogCategory TranslatePlatformLogCategory(PlatformLogCategory category) {
    if (category == ApplicationLog) {
        return SDL_LOG_CATEGORY_APPLICATION;
    } else {
        assert(category == PlatformLog);

        // `SDL_LOG_CATEGORY_CUSTOM` will query `SDL_default_priority` on a call to `SDL_LogGetPriority`.
        return SDL_LOG_CATEGORY_CUSTOM;
    }
}

void SdlLogger::SetLogLevel(PlatformLogCategory category, PlatformLogLevel logLevel) {
    if (category == ApplicationLog) {
        SDL_LogSetPriority(SDL_LOG_CATEGORY_APPLICATION, TranslatePlatformLogLevel(logLevel));
    } else {
        assert(category == PlatformLog);
        SDL_LogPriority applicationPriority = SDL_LogGetPriority(SDL_LOG_CATEGORY_APPLICATION);
        SDL_LogPriority assertPriority = SDL_LogGetPriority(SDL_LOG_CATEGORY_ASSERT);

        // This sets default log priority. Note that this call is better than going through all the different
        // categories and calling `SDL_LogSetPriority` because it sets a single global variable inside SDL
        // (`SDL_default_priority`). Making several calls would instead allocate a linked list of category-priority
        // pairs which will then be traversed in O(n) every time we call `SDL_Log`.
        SDL_LogSetAllPriority(TranslatePlatformLogLevel(logLevel));

        // Then we need to roll back assert priority. All SDL asserts are issued at `SDL_LOG_PRIORITY_WARN` which
        // makes little sense, they should be at `SDL_LOG_PRIORITY_CRITICAL`.
        SDL_LogSetPriority(SDL_LOG_CATEGORY_ASSERT, assertPriority);

        // And also roll back application priority.
        SDL_LogSetPriority(SDL_LOG_CATEGORY_APPLICATION, applicationPriority);
    }
}

PlatformLogLevel SdlLogger::LogLevel(PlatformLogCategory category) const {
    return TranslateSdlLogLevel(SDL_LogGetPriority(TranslatePlatformLogCategory(category)));
}

void SdlLogger::Log(PlatformLogCategory category, PlatformLogLevel logLevel, const char *message) {
    // SDL_LogMessage is thread-safe, thus this function is also thread-safe. The only caveat is that SDL_Init
    // should be called before creating several threads that do logging, but we don't care to check for that.
    SDL_LogCategory sdlCategory = TranslatePlatformLogCategory(category);
    SDL_LogPriority sdlPriority = TranslatePlatformLogLevel(logLevel);
#ifdef __APPLE__
    // SDL_LogMessage on apple uses NSLog, and it prepends timestamp
    // TODO(captainurist): check if android does the same.
    SDL_LogMessage(sdlCategory, sdlPriority, "%s", message);
#else
    // On other OSes we have to add timestamp manually
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    SDL_LogMessage(sdlCategory, sdlPriority, "[%04d/%02d/%02d %02d:%02d:%02d] %s",
                   tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, message);
#endif
}

#pragma once
#include <exo/maths/numerics.h>
#include <exo/collections/vector.h>
#include <exo/cross/prelude.h>

#include <functional>
#include <string>

#if defined(CROSS_WINDOWS)
#    include <array>
#    include <basetsd.h> // win32 types
#    include <wtypes.h>  // HANDLE type
#endif

namespace cross
{
struct Watch
{

#if defined(CROSS_LINUX)

#elif defined(CROSS_WINDOWS)
    HANDLE directory_handle;
    OVERLAPPED overlapped;

    std::array<u8, 2048> buffer;
#endif

    int wd; /* Watch descriptor.  */
    std::string path;
};

enum struct WatchEvent
{
    FileRenamed,
    FileChanged,
    FileRemoved,
    FileAdded
};

struct Event
{

#if defined(CROSS_LINUX)
    u32 mask;   /* Watch mask.  */
    u32 cookie; /* Cookie to synchronize two events.  */
#elif defined(CROSS_WINDOWS)

#endif

    int wd;     /* Watch descriptor.  */
    std::string name; /* filename. */
    usize len;
    WatchEvent action;
};

using FileEventF = std::function<void(const Watch &, const Event &)>;

struct FileWatcher
{
#if defined(CROSS_LINUX)
    int inotify_fd;
#elif defined(CROSS_WINDOWS)

#endif

    Vec<Watch> watches;
    Vec<Event> current_events;
    Vec<FileEventF> callbacks;

    static FileWatcher create();

    Watch add_watch(const char *path);
    void on_file_change(const FileEventF &f);

    void update();
    void destroy();
};
}
#if 0 // MIGRATE_FIXME - similar code exists now inside device, but unsure whether it is as fully fledged as the original
#ifndef GCM_DEBUG_H
#define GCM_DEBUG_H

#if !__FINAL && __PPU

namespace rage {

class gcmDebug
{
public:
    void Init();    
    void Push(const char* name);
    void Pop();
    void Report();
    void HandlePpuException();

    void HangThread();

private:
    static void ExceptionHandler(u32);

    const char* m_hangFromSpu[16] ;
    u32 m_labels;
    u32 m_stackLevel;
    bool m_needFifoDump;
};

extern gcmDebug g_gcmDebug;

struct gcmDebugAutoPush
{
    gcmDebugAutoPush(const char* name) {g_gcmDebug.Push(name);}
    ~gcmDebugAutoPush() {g_gcmDebug.Pop();}
};

} // namespace rage

#define GCMDBG_INIT() g_gcmDebug.Init()
#define GCMDBG_PUSH(name) g_gcmDebug.Push(name)
#define GCMDBG_POP() g_gcmDebug.Pop()
#define GCMDBG_AUTOPUSH(name) gcmDebugAutoPush _gcmDbg##__LINE__(name)
#define GCMDBG_REPORT() g_gcmDebug.Report()

#else // !__FINAL

#define GCMDBG_INIT()
#define GCMDBG_PUSH(name)
#define GCMDBG_POP()
#define GCMDBG_REPORT()
#define GCMDBG_AUTOPUSH(name)

#endif // !__FINAL

#endif // GCM_DEBUG_H

#endif // 0
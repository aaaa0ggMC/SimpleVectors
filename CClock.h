#ifndef CCLOCK_H_INCLUDED
#define CCLOCK_H_INCLUDED
#include <windows.h>

struct TMST0{
    DWORD all;
    DWORD offset;
};

class Clock{
public:
    Clock(bool start = true);
    void Start();
    TMST0 Stop();
    bool isStop();
    DWORD GetALLTime();//Do not set pre time
    DWORD GetOffset();//Set Pre Time
    TMST0 Now();//Do not reset preTime
private:
    DWORD m_StartTime;
    DWORD m_PreTime;
    bool m_start;
};


#endif // CCLOCK_H_INCLUDED

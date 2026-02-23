/*
================================================================================
FILE: proc_reader.h
PURPOSE: Read real running processes from the OS kernel
DESCRIPTION:
    - On macOS: uses proc_pidinfo() from <libproc.h>
    - On Linux: reads /proc/<pid>/status
    - Returns process info sorted by resident memory (RSS) descending
    - Used by Live System Mode in the React frontend
================================================================================
*/

#ifndef PROC_READER_H
#define PROC_READER_H


/*
================================================================================
STRUCTURE: ProcessInfo
================================================================================
PURPOSE: Holds information about one real OS process

FIELDS:
- pid:        Real process ID from the kernel
- name:       Process name (e.g., "Safari", "node", "Finder")
- vmRSS_KB:   Resident Set Size in KB (physical RAM actually used)
- vmSize_KB:  Virtual Memory Size in KB (total address space)
================================================================================
*/
typedef struct {
    int  pid;           // Real kernel PID
    char name[256];     // Process name
    long vmRSS_KB;      // Resident physical memory in KB
    long vmSize_KB;     // Virtual memory size in KB
} ProcessInfo;


/*
--------------------------------------------------------------------------------
FUNCTION: get_process_list
--------------------------------------------------------------------------------
PURPOSE: Read real processes from the OS and return top N by RSS

WHAT IT DOES:
- macOS: calls proc_listallpids() + proc_pidinfo(PROC_PIDTASKALLINFO)
- Linux: reads /proc/<pid>/status files
- Sorts by vmRSS_KB descending
- Caps at top 10 processes
- Caller must free(*out) when done

PARAMETERS:
- out:   Output pointer to array of ProcessInfo (caller frees)
- count: Output number of processes returned

RETURNS:
- 0 on success
- -1 on failure (permission denied, etc.)
*/
int get_process_list(ProcessInfo **out, int *count);


/*
--------------------------------------------------------------------------------
FUNCTION: processes_to_json
--------------------------------------------------------------------------------
PURPOSE: Serialize the process list to a JSON string

OUTPUT FORMAT:
{
  "count": 10,
  "processes": [
    {"pid":123,"name":"Safari","vmRSS_KB":45000,"vmSize_KB":900000},
    ...
  ],
  "permissionWarning": false
}

PARAMETERS:
- buf:     Output buffer for JSON string
- bufSize: Size of the output buffer
*/
void processes_to_json(char *buf, int bufSize);



/*
--------------------------------------------------------------------------------
FUNCTION: get_process_detail_json
--------------------------------------------------------------------------------
PURPOSE: Fetch extended detail about a single process by PID

OUTPUT FORMAT:
{
  "pid": 123,
  "name": "Safari",
  "vmRSS_KB": 45000,
  "vmSize_KB": 900000,
  "execPath": "/Applications/Safari.app/...",
  "threads": 24,
  "pageFaults": 1523,
  "pageins": 203,
  "cowFaults": 87,
  "startTime": "12:34:05",
  "found": true
}

PARAMETERS:
- pid:     The process ID to look up
- buf:     Output buffer for JSON string
- bufSize: Size of the output buffer
*/
void get_process_detail_json(int pid, char *buf, int bufSize);


/*
--------------------------------------------------------------------------------
FUNCTION: get_memory_pressure_json
--------------------------------------------------------------------------------
PURPOSE: Return system-wide memory pressure as JSON

OUTPUT FORMAT:
{
  "totalRAM_KB": 8388608,
  "usedRAM_KB": 4194304,
  "freeRAM_KB": 4194304,
  "usedPercent": 50.0,
  "pressureLevel": "MODERATE",
  "totalRAM_GB": "8.0",
  "usedRAM_GB": "4.0"
}

PARAMETERS:
- buf:     Output buffer for JSON string
- bufSize: Size of the output buffer
*/
void get_memory_pressure_json(char *buf, int bufSize);


#endif /* PROC_READER_H */

/*
================================================================================
FILE: proc_reader.c
PURPOSE: Read real running processes from the OS kernel
DESCRIPTION:
    - macOS: uses proc_listallpids() + proc_pidinfo() from <libproc.h>
    - Linux: reads /proc/<pid>/status
    - Sorts by RSS descending, returns top 10
    - Serializes to JSON for the /api/processes endpoint
================================================================================
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "../include/proc_reader.h"
#include "../include/os_memory.h"

/* Maximum number of processes to return (skyline can show ~10 legibly) */
#define MAX_DISPLAY_PROCS 10

/* Maximum PIDs we'll scan on either platform */
#define MAX_SCAN_PIDS 4096


/*
================================================================================
PLATFORM: macOS
================================================================================
Uses Apple's libproc API:
  - proc_listallpids()  → get all PIDs on the system
  - proc_pidinfo()      → get task info (memory, name) for each PID
  - proc_name()         → get process name string
================================================================================
*/
#ifdef __APPLE__

#include <libproc.h>
#include <sys/proc_info.h>

int get_process_list(ProcessInfo **out, int *count) {
    *out = NULL;
    *count = 0;

    /* Step 1: Get list of all PIDs */
    int numPids = proc_listallpids(NULL, 0);
    if (numPids <= 0) {
        return -1;
    }

    pid_t *pidList = (pid_t *)malloc(sizeof(pid_t) * numPids);
    if (!pidList) return -1;

    numPids = proc_listallpids(pidList, sizeof(pid_t) * numPids);
    if (numPids <= 0) {
        free(pidList);
        return -1;
    }

    /* Step 2: Allocate temporary array for all readable processes */
    ProcessInfo *tempList = (ProcessInfo *)malloc(sizeof(ProcessInfo) * numPids);
    if (!tempList) {
        free(pidList);
        return -1;
    }

    int validCount = 0;

    /* Step 3: Query each PID for task info */
    for (int i = 0; i < numPids && validCount < MAX_SCAN_PIDS; i++) {
        pid_t pid = pidList[i];
        if (pid <= 0) continue;  /* Skip invalid PIDs */

        struct proc_taskallinfo taskInfo;
        int ret = proc_pidinfo(pid, PROC_PIDTASKALLINFO, 0,
                               &taskInfo, sizeof(taskInfo));

        if (ret <= 0) {
            /* Permission denied or process exited — skip silently */
            continue;
        }

        /* Get process name */
        char procName[256] = {0};
        proc_name(pid, procName, sizeof(procName));

        /* If proc_name returns empty, use the name from taskInfo */
        if (procName[0] == '\0') {
            strncpy(procName, taskInfo.pbsd.pbi_comm, sizeof(procName) - 1);
        }

        /* Skip kernel_task and processes with no name */
        if (procName[0] == '\0') continue;
        if (strcmp(procName, "kernel_task") == 0) continue;

        /* Fill in ProcessInfo */
        ProcessInfo *p = &tempList[validCount];
        p->pid = (int)pid;
        strncpy(p->name, procName, sizeof(p->name) - 1);
        p->name[sizeof(p->name) - 1] = '\0';

        /* Resident size: taskInfo.ptinfo.pti_resident_size is in bytes */
        p->vmRSS_KB = (long)(taskInfo.ptinfo.pti_resident_size / 1024);

        /* Virtual size: taskInfo.ptinfo.pti_virtual_size is in bytes */
        p->vmSize_KB = (long)(taskInfo.ptinfo.pti_virtual_size / 1024);

        /* Skip processes with zero RSS (zombie/dead) */
        if (p->vmRSS_KB <= 0) continue;

        validCount++;
    }

    free(pidList);

    if (validCount == 0) {
        free(tempList);
        *out = NULL;
        *count = 0;
        return -1;  /* No readable processes (permission issue) */
    }

    /* Step 4: Partial sort — only find top MAX_DISPLAY_PROCS by RSS descending */
    int sortLimit = validCount < MAX_DISPLAY_PROCS ? validCount : MAX_DISPLAY_PROCS;
    for (int i = 0; i < sortLimit && i < validCount - 1; i++) {
        int maxIdx = i;
        for (int j = i + 1; j < validCount; j++) {
            if (tempList[j].vmRSS_KB > tempList[maxIdx].vmRSS_KB) {
                maxIdx = j;
            }
        }
        if (maxIdx != i) {
            ProcessInfo temp = tempList[i];
            tempList[i] = tempList[maxIdx];
            tempList[maxIdx] = temp;
        }
    }

    /* Step 5: Cap at top MAX_DISPLAY_PROCS */
    int resultCount = validCount < MAX_DISPLAY_PROCS ? validCount : MAX_DISPLAY_PROCS;

    ProcessInfo *result = (ProcessInfo *)malloc(sizeof(ProcessInfo) * resultCount);
    if (!result) {
        free(tempList);
        return -1;
    }

    memcpy(result, tempList, sizeof(ProcessInfo) * resultCount);
    free(tempList);

    *out = result;
    *count = resultCount;
    return 0;
}


/*
================================================================================
PLATFORM: Linux
================================================================================
Reads /proc/<pid>/status for each process directory.
Parses: Name, VmRSS, VmSize fields.
================================================================================
*/
#else  /* Linux */

#include <dirent.h>
#include <ctype.h>

int get_process_list(ProcessInfo **out, int *count) {
    *out = NULL;
    *count = 0;

    DIR *procDir = opendir("/proc");
    if (!procDir) return -1;

    ProcessInfo *tempList = (ProcessInfo *)malloc(sizeof(ProcessInfo) * MAX_SCAN_PIDS);
    if (!tempList) {
        closedir(procDir);
        return -1;
    }

    int validCount = 0;
    struct dirent *entry;

    while ((entry = readdir(procDir)) != NULL && validCount < MAX_SCAN_PIDS) {
        /* Only process numeric directories (PIDs) */
        if (!isdigit(entry->d_name[0])) continue;

        int pid = atoi(entry->d_name);
        if (pid <= 0) continue;

        /* Read /proc/<pid>/status */
        char statusPath[512];
        snprintf(statusPath, sizeof(statusPath), "/proc/%d/status", pid);

        FILE *fp = fopen(statusPath, "r");
        if (!fp) continue;  /* Permission denied — skip */

        ProcessInfo *p = &tempList[validCount];
        memset(p, 0, sizeof(ProcessInfo));
        p->pid = pid;

        char line[512];
        while (fgets(line, sizeof(line), fp)) {
            if (strncmp(line, "Name:", 5) == 0) {
                /* Parse: "Name:\tprocess_name\n" */
                char *val = line + 5;
                while (*val == ' ' || *val == '\t') val++;
                /* Remove trailing newline */
                char *nl = strchr(val, '\n');
                if (nl) *nl = '\0';
                strncpy(p->name, val, sizeof(p->name) - 1);
            }
            else if (strncmp(line, "VmRSS:", 6) == 0) {
                /* Parse: "VmRSS:\t   12345 kB\n" */
                p->vmRSS_KB = atol(line + 6);
            }
            else if (strncmp(line, "VmSize:", 7) == 0) {
                /* Parse: "VmSize:\t  123456 kB\n" */
                p->vmSize_KB = atol(line + 7);
            }
        }

        fclose(fp);

        /* Skip kernel threads with no RSS or no name */
        if (p->name[0] == '\0' || p->vmRSS_KB <= 0) continue;

        validCount++;
    }

    closedir(procDir);

    if (validCount == 0) {
        free(tempList);
        return -1;
    }

    /* Partial sort — only find top MAX_DISPLAY_PROCS by RSS descending */
    int sortLimit = validCount < MAX_DISPLAY_PROCS ? validCount : MAX_DISPLAY_PROCS;
    for (int i = 0; i < sortLimit && i < validCount - 1; i++) {
        int maxIdx = i;
        for (int j = i + 1; j < validCount; j++) {
            if (tempList[j].vmRSS_KB > tempList[maxIdx].vmRSS_KB) {
                maxIdx = j;
            }
        }
        if (maxIdx != i) {
            ProcessInfo temp = tempList[i];
            tempList[i] = tempList[maxIdx];
            tempList[maxIdx] = temp;
        }
    }

    /* Cap at top MAX_DISPLAY_PROCS */
    int resultCount = validCount < MAX_DISPLAY_PROCS ? validCount : MAX_DISPLAY_PROCS;

    ProcessInfo *result = (ProcessInfo *)malloc(sizeof(ProcessInfo) * resultCount);
    if (!result) {
        free(tempList);
        return -1;
    }

    memcpy(result, tempList, sizeof(ProcessInfo) * resultCount);
    free(tempList);

    *out = result;
    *count = resultCount;
    return 0;
}

#endif /* __APPLE__ vs Linux */


/*
================================================================================
FUNCTION: processes_to_json
================================================================================
PURPOSE: Serialize the process list to JSON for the HTTP API

OUTPUT FORMAT:
{
  "count": N,
  "processes": [ ... ],
  "permissionWarning": true/false
}
================================================================================
*/
void processes_to_json(char *buf, int bufSize) {

    ProcessInfo *procs = NULL;
    int count = 0;
    int result = get_process_list(&procs, &count);

    int permissionWarning = (result != 0 || count == 0) ? 1 : 0;

    /* Start building JSON */
    int offset = 0;
    offset += snprintf(buf + offset, bufSize - offset,
        "{\"count\":%d,\"processes\":[", count);

    for (int i = 0; i < count && offset < bufSize - 256; i++) {
        /* Escape process name for JSON (replace quotes/backslashes) */
        char safeName[512];
        int si = 0;
        for (int j = 0; procs[i].name[j] != '\0' && si < (int)sizeof(safeName) - 2; j++) {
            char c = procs[i].name[j];
            if (c == '"' || c == '\\') {
                safeName[si++] = '\\';
            }
            safeName[si++] = c;
        }
        safeName[si] = '\0';

        if (i > 0) {
            offset += snprintf(buf + offset, bufSize - offset, ",");
        }

        offset += snprintf(buf + offset, bufSize - offset,
            "{\"pid\":%d,\"name\":\"%s\",\"vmRSS_KB\":%ld,\"vmSize_KB\":%ld}",
            procs[i].pid,
            safeName,
            procs[i].vmRSS_KB,
            procs[i].vmSize_KB
        );
    }

    offset += snprintf(buf + offset, bufSize - offset,
        "],\"permissionWarning\":%s}",
        permissionWarning ? "true" : "false"
    );

    /* Cleanup */
    if (procs) {
        free(procs);
    }
}


/*
================================================================================
FUNCTION: get_process_detail_json
================================================================================
PURPOSE: Fetch extended detail about a single process by PID
         Returns JSON with threads, page faults, exec path, start time, etc.
================================================================================
*/

#ifdef __APPLE__

void get_process_detail_json(int pid, char *buf, int bufSize) {
    struct proc_taskallinfo taskInfo;
    int ret = proc_pidinfo(pid, PROC_PIDTASKALLINFO, 0,
                           &taskInfo, sizeof(taskInfo));

    if (ret <= 0) {
        /* PID not found or permission denied */
        snprintf(buf, bufSize, "{\"found\":false,\"pid\":%d}", pid);
        return;
    }

    /* Get process name */
    char procName[256] = {0};
    proc_name(pid, procName, sizeof(procName));
    if (procName[0] == '\0') {
        strncpy(procName, taskInfo.pbsd.pbi_comm, sizeof(procName) - 1);
    }

    /* Escape name for JSON */
    char safeName[512];
    int si = 0;
    for (int j = 0; procName[j] != '\0' && si < (int)sizeof(safeName) - 2; j++) {
        char c = procName[j];
        if (c == '"' || c == '\\') safeName[si++] = '\\';
        safeName[si++] = c;
    }
    safeName[si] = '\0';

    /* RSS and Virtual size */
    long vmRSS_KB  = (long)(taskInfo.ptinfo.pti_resident_size / 1024);
    long vmSize_KB = (long)(taskInfo.ptinfo.pti_virtual_size / 1024);

    /* Thread count */
    int threads = (int)taskInfo.ptinfo.pti_threadnum;

    /* Page fault stats */
    long pageFaults = (long)taskInfo.ptinfo.pti_faults;
    long pageins    = (long)taskInfo.ptinfo.pti_pageins;
    long cowFaults  = (long)taskInfo.ptinfo.pti_cow_faults;

    /* Get executable path via PROC_PIDVNODEPATHINFO */
    char execPath[1024] = {0};
    struct proc_vnodepathinfo vnodeInfo;
    int vnRet = proc_pidinfo(pid, PROC_PIDVNODEPATHINFO, 0,
                             &vnodeInfo, sizeof(vnodeInfo));
    if (vnRet > 0) {
        strncpy(execPath, vnodeInfo.pvi_cdir.vip_path, sizeof(execPath) - 1);
    }
    /* Fallback: try proc_pidpath */
    if (execPath[0] == '\0') {
        proc_pidpath(pid, execPath, sizeof(execPath));
    }

    /* Escape execPath for JSON */
    char safeExecPath[2048];
    si = 0;
    for (int j = 0; execPath[j] != '\0' && si < (int)sizeof(safeExecPath) - 2; j++) {
        char c = execPath[j];
        if (c == '"' || c == '\\') safeExecPath[si++] = '\\';
        safeExecPath[si++] = c;
    }
    safeExecPath[si] = '\0';

    /* Start time — convert unix timestamp to HH:MM:SS */
    char startTimeStr[32] = "unknown";
    time_t startSec = (time_t)taskInfo.pbsd.pbi_start_tvsec;
    if (startSec > 0) {
        struct tm *tmInfo = localtime(&startSec);
        if (tmInfo) {
            strftime(startTimeStr, sizeof(startTimeStr), "%H:%M:%S", tmInfo);
        }
    }

    /* Build JSON */
    snprintf(buf, bufSize,
        "{\"found\":true,\"pid\":%d,\"name\":\"%s\","
        "\"vmRSS_KB\":%ld,\"vmSize_KB\":%ld,"
        "\"execPath\":\"%s\","
        "\"threads\":%d,\"pageFaults\":%ld,"
        "\"pageins\":%ld,\"cowFaults\":%ld,"
        "\"startTime\":\"%s\"}",
        pid, safeName, vmRSS_KB, vmSize_KB,
        safeExecPath, threads, pageFaults,
        pageins, cowFaults, startTimeStr
    );
}

#else  /* Linux */

#include <unistd.h>

void get_process_detail_json(int pid, char *buf, int bufSize) {
    /* Read /proc/<pid>/status */
    char statusPath[512];
    snprintf(statusPath, sizeof(statusPath), "/proc/%d/status", pid);

    FILE *fp = fopen(statusPath, "r");
    if (!fp) {
        snprintf(buf, bufSize, "{\"found\":false,\"pid\":%d}", pid);
        return;
    }

    char procName[256] = {0};
    long vmRSS_KB = 0, vmSize_KB = 0;
    char line[512];

    while (fgets(line, sizeof(line), fp)) {
        if (strncmp(line, "Name:", 5) == 0) {
            char *val = line + 5;
            while (*val == ' ' || *val == '\t') val++;
            char *nl = strchr(val, '\n');
            if (nl) *nl = '\0';
            strncpy(procName, val, sizeof(procName) - 1);
        } else if (strncmp(line, "VmRSS:", 6) == 0) {
            vmRSS_KB = atol(line + 6);
        } else if (strncmp(line, "VmSize:", 7) == 0) {
            vmSize_KB = atol(line + 7);
        }
    }
    fclose(fp);

    /* Read /proc/<pid>/stat for threads and starttime */
    int threads = 0;
    unsigned long long starttime = 0;
    char statPath[512];
    snprintf(statPath, sizeof(statPath), "/proc/%d/stat", pid);
    fp = fopen(statPath, "r");
    if (fp) {
        char statLine[2048];
        if (fgets(statLine, sizeof(statLine), fp)) {
            /* Skip past comm field (enclosed in parentheses) */
            char *closeParen = strrchr(statLine, ')');
            if (closeParen) {
                closeParen += 2; /* skip ") " */
                /* Fields after comm: state(3) ppid(4) ... num_threads(20) ... starttime(22) */
                char state;
                int ppid, pgrp, session, tty_nr, tpgid;
                unsigned flags;
                unsigned long minflt, cminflt, majflt, cmajflt, utime, stime;
                long cutime, cstime, priority, nice, num_threads_val;
                unsigned long long itrealvalue;
                sscanf(closeParen,
                    "%c %d %d %d %d %d %u %lu %lu %lu %lu %lu %lu "
                    "%ld %ld %ld %ld %ld %llu %llu",
                    &state, &ppid, &pgrp, &session, &tty_nr, &tpgid,
                    &flags, &minflt, &cminflt, &majflt, &cmajflt,
                    &utime, &stime, &cutime, &cstime, &priority,
                    &nice, &num_threads_val, &itrealvalue, &starttime);
                threads = (int)num_threads_val;
            }
        }
        fclose(fp);
    }

    /* Read /proc/<pid>/exe symlink for executable path */
    char execPath[1024] = {0};
    char exeLink[512];
    snprintf(exeLink, sizeof(exeLink), "/proc/%d/exe", pid);
    ssize_t len = readlink(exeLink, execPath, sizeof(execPath) - 1);
    if (len > 0) execPath[len] = '\0';

    /* Convert starttime (clock ticks since boot) to HH:MM:SS */
    char startTimeStr[32] = "unknown";
    if (starttime > 0) {
        long ticksPerSec = sysconf(_SC_CLK_TCK);
        if (ticksPerSec > 0) {
            /* Read system uptime */
            FILE *uptimeFp = fopen("/proc/uptime", "r");
            if (uptimeFp) {
                double uptimeSecs;
                if (fscanf(uptimeFp, "%lf", &uptimeSecs) == 1) {
                    time_t now = time(NULL);
                    time_t bootTime = now - (time_t)uptimeSecs;
                    time_t procStart = bootTime + (time_t)(starttime / ticksPerSec);
                    struct tm *tmInfo = localtime(&procStart);
                    if (tmInfo) {
                        strftime(startTimeStr, sizeof(startTimeStr), "%H:%M:%S", tmInfo);
                    }
                }
                fclose(uptimeFp);
            }
        }
    }

    /* Escape name and execPath for JSON */
    char safeName[512];
    int si = 0;
    for (int j = 0; procName[j] != '\0' && si < (int)sizeof(safeName) - 2; j++) {
        char c = procName[j];
        if (c == '"' || c == '\\') safeName[si++] = '\\';
        safeName[si++] = c;
    }
    safeName[si] = '\0';

    char safeExecPath[2048];
    si = 0;
    for (int j = 0; execPath[j] != '\0' && si < (int)sizeof(safeExecPath) - 2; j++) {
        char c = execPath[j];
        if (c == '"' || c == '\\') safeExecPath[si++] = '\\';
        safeExecPath[si++] = c;
    }
    safeExecPath[si] = '\0';

    snprintf(buf, bufSize,
        "{\"found\":true,\"pid\":%d,\"name\":\"%s\","
        "\"vmRSS_KB\":%ld,\"vmSize_KB\":%ld,"
        "\"execPath\":\"%s\","
        "\"threads\":%d,\"pageFaults\":0,"
        "\"pageins\":0,\"cowFaults\":0,"
        "\"startTime\":\"%s\"}",
        pid, safeName, vmRSS_KB, vmSize_KB,
        safeExecPath, threads, startTimeStr
    );
}

#endif /* __APPLE__ vs Linux — get_process_detail_json */


/*
================================================================================
FUNCTION: get_memory_pressure_json
================================================================================
PURPOSE: Return system-wide memory pressure as JSON
         Calculates used RAM, free RAM, percentage, and pressure level
================================================================================
*/

#ifdef __APPLE__

void get_memory_pressure_json(char *buf, int bufSize) {
    /* Total RAM from os_memory.h */
    size_t totalRAM_bytes = os_get_total_ram();
    long totalRAM_KB = (long)(totalRAM_bytes / 1024);

    /* Sum RSS across ALL processes for used RAM estimate */
    int numPids = proc_listallpids(NULL, 0);
    long usedRAM_KB = 0;

    if (numPids > 0) {
        pid_t *pidList = (pid_t *)malloc(sizeof(pid_t) * numPids);
        if (pidList) {
            numPids = proc_listallpids(pidList, sizeof(pid_t) * numPids);
            for (int i = 0; i < numPids && i < MAX_SCAN_PIDS; i++) {
                if (pidList[i] <= 0) continue;
                struct proc_taskallinfo taskInfo;
                int ret = proc_pidinfo(pidList[i], PROC_PIDTASKALLINFO, 0,
                                       &taskInfo, sizeof(taskInfo));
                if (ret > 0) {
                    long rss = (long)(taskInfo.ptinfo.pti_resident_size / 1024);
                    if (rss > 0) usedRAM_KB += rss;
                }
            }
            free(pidList);
        }
    }

    /* Clamp used to total */
    if (usedRAM_KB > totalRAM_KB) usedRAM_KB = totalRAM_KB;

    long freeRAM_KB = totalRAM_KB - usedRAM_KB;
    double usedPercent = totalRAM_KB > 0
        ? (double)usedRAM_KB / (double)totalRAM_KB * 100.0
        : 0.0;

    /* Determine pressure level */
    const char *level;
    if (usedPercent < 50.0)       level = "LOW";
    else if (usedPercent < 70.0)  level = "MODERATE";
    else if (usedPercent < 85.0)  level = "HIGH";
    else                          level = "CRITICAL";

    /* Human-readable GB strings */
    char totalGB[16], usedGB[16];
    snprintf(totalGB, sizeof(totalGB), "%.1f", (double)totalRAM_KB / 1048576.0);
    snprintf(usedGB,  sizeof(usedGB),  "%.1f", (double)usedRAM_KB  / 1048576.0);

    snprintf(buf, bufSize,
        "{\"totalRAM_KB\":%ld,\"usedRAM_KB\":%ld,\"freeRAM_KB\":%ld,"
        "\"usedPercent\":%.1f,\"pressureLevel\":\"%s\","
        "\"totalRAM_GB\":\"%s\",\"usedRAM_GB\":\"%s\"}",
        totalRAM_KB, usedRAM_KB, freeRAM_KB,
        usedPercent, level, totalGB, usedGB
    );
}

#else  /* Linux */

void get_memory_pressure_json(char *buf, int bufSize) {
    long memTotal = 0, memAvailable = 0;

    FILE *fp = fopen("/proc/meminfo", "r");
    if (fp) {
        char line[256];
        while (fgets(line, sizeof(line), fp)) {
            if (strncmp(line, "MemTotal:", 9) == 0) {
                memTotal = atol(line + 9);
            } else if (strncmp(line, "MemAvailable:", 13) == 0) {
                memAvailable = atol(line + 13);
            }
        }
        fclose(fp);
    }

    long usedRAM_KB = memTotal - memAvailable;
    long freeRAM_KB = memAvailable;
    double usedPercent = memTotal > 0
        ? (double)usedRAM_KB / (double)memTotal * 100.0
        : 0.0;

    const char *level;
    if (usedPercent < 50.0)       level = "LOW";
    else if (usedPercent < 70.0)  level = "MODERATE";
    else if (usedPercent < 85.0)  level = "HIGH";
    else                          level = "CRITICAL";

    char totalGB[16], usedGB[16];
    snprintf(totalGB, sizeof(totalGB), "%.1f", (double)memTotal    / 1048576.0);
    snprintf(usedGB,  sizeof(usedGB),  "%.1f", (double)usedRAM_KB  / 1048576.0);

    snprintf(buf, bufSize,
        "{\"totalRAM_KB\":%ld,\"usedRAM_KB\":%ld,\"freeRAM_KB\":%ld,"
        "\"usedPercent\":%.1f,\"pressureLevel\":\"%s\","
        "\"totalRAM_GB\":\"%s\",\"usedRAM_GB\":\"%s\"}",
        memTotal, usedRAM_KB, freeRAM_KB,
        usedPercent, level, totalGB, usedGB
    );
}

#endif /* __APPLE__ vs Linux — get_memory_pressure_json */


/*
================================================================================
END OF FILE: proc_reader.c
================================================================================

WHAT WE IMPLEMENTED:
1. get_process_list()           — Read real OS processes using platform APIs
2. processes_to_json()          — Serialize to JSON for the HTTP endpoint
3. get_process_detail_json()    — Extended detail for a single PID
4. get_memory_pressure_json()   — System-wide memory pressure indicator

PLATFORM-SPECIFIC SYSTEM CALLS:
  macOS: proc_listallpids(), proc_pidinfo(PROC_PIDTASKALLINFO), proc_name()
         proc_pidinfo(PROC_PIDVNODEPATHINFO), proc_pidpath()
  Linux: opendir("/proc"), fopen("/proc/<pid>/status"), parse Name/VmRSS/VmSize
         readlink("/proc/<pid>/exe"), /proc/<pid>/stat, /proc/meminfo

KEY DESIGN DECISIONS:
- Top 10 processes only (skyline legibility)
- Silent permission failure handling (skip unreadable PIDs)
- permissionWarning flag in JSON (frontend can show sudo hint)
- Sorted by RSS descending (heaviest processes first)
- Process detail returns found:false on permission denied
- Memory pressure uses LOW/MODERATE/HIGH/CRITICAL thresholds
================================================================================
*/

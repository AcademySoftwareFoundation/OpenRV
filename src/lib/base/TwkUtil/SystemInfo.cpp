//******************************************************************************
// Copyright (c) 2005 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#include <cstring>
#include <iostream>
#include <fstream>
#include <limits>
#include <stl_ext/string_algo.h>

#ifdef _MSC_VER
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <assert.h>
#elif __APPLE__
#include <sys/sysctl.h>
#endif

#ifdef PLATFORM_APPLE_MACH_BSD
#include <mach/host_info.h>
#include <mach/mach_host.h>
#include <mach/mach_init.h>
#endif
#include <TwkUtil/SystemInfo.h>

namespace TwkUtil
{
    using namespace std;

    size_t SystemInfo::m_colorBitDepth = 0;
    size_t SystemInfo::m_maxVRAM = 1024 * 1024 * 24; // 24 Mb
    size_t SystemInfo::m_useableMemory = 0;

    //----------------------------------------------------------------------
    //----------------------------------------------------------------------
    //
    //  DARWIN
    //
    //----------------------------------------------------------------------
    //----------------------------------------------------------------------

#ifdef PLATFORM_APPLE_MACH_BSD
//
//  10.3 this is not defined (anywhere I can tell). In 10.4 it is.
//
#ifndef PAGE_SIZE
#define PAGE_SIZE 4096
#endif

    size_t SystemInfo::usableMemory()
    {
        if (!m_useableMemory)
        {
            vm_statistics_data_t vminfo;
            int n = HOST_VM_INFO_COUNT;

            if (host_statistics(mach_host_self(), HOST_VM_INFO,
                                (host_info_t)&vminfo,
                                (mach_msg_type_number_t*)&n)
                != KERN_SUCCESS)
            {
                throw SystemCallFailedException();
            }
            else
            {
                m_useableMemory = (vminfo.free_count + vminfo.active_count
                                   + vminfo.inactive_count)
                                  * static_cast<size_t>(PAGE_SIZE);
            }
        }

        return m_useableMemory;
    }

    size_t SystemInfo::numCPUs()
    {
        host_basic_info_data_t info;
        mach_msg_type_number_t count = HOST_BASIC_INFO_COUNT;

        kern_return_t kr = host_info(mach_host_self(), HOST_BASIC_INFO,
                                     (host_info_t)&info, &count);

        if (kr != KERN_SUCCESS)
        {
            cout << "ERROR: host_info() failed, using 1 cpu" << endl;
            return 1;
        }

        size_t cpus = size_t(info.avail_cpus);
        // size_t cpus = size_t(info.max_cpus);

        return cpus;
    }

    //------------------------------------------------------------------------------
    // Note: The following code was imported from Flame
    // (creative-finishing/common/src/base/osal/src/CoSystem_MACOSX.cpp)
    //
    bool SystemInfo::getSystemMemoryInfo(size_t* physTotal, size_t* physFree,
                                         size_t* physUsed, size_t* physInactive,
                                         size_t* physCached, size_t* swapTotal,
                                         size_t* swapFree, size_t* swapUsed)
    {
        bool ok = (physTotal || physFree || physUsed || physInactive
                   || physCached || swapTotal || swapFree || swapUsed);

        size_t tmpPhysTotal = 0;
        size_t tmpPhysFree = 0;
        size_t tmpPhysUsed = 0;
        size_t tmpPhysInactive = 0;
        size_t tmpPhysCached = 0;
        size_t tmpSwapTotal = 0;
        size_t tmpSwapFree = 0;
        size_t tmpSwapUsed = 0;

        if (ok)
        {
            //
            // The following observations were made in the macosx10.8/macosx10.9
            // SDKs and from the system_cmds-597.90.1/top-89.1.2 packages.
            //
            // The vm_statistics64_data_t structure contains the following
            // relevant information (from <mach/vm_statistics.h> in the
            // macosx10.8 and macosx10.9 SDKs):
            //	natural_t	free_count;		/* # of pages free */
            //	natural_t	active_count;		/* # of pages active */
            //	natural_t	inactive_count;		/* # of pages inactive
            //*/ 	natural_t	wire_count;		/* # of pages
            // wired down
            //*/ 	natural_t	purgeable_count;	/* # of pages
            // purgeable
            //*/
            //	/*
            //	 * NB: speculative pages are already accounted for in
            //"free_count",
            //	 * so "speculative_count" is the number of "free" pages that are
            //	 * used to hold data that was read speculatively from disk but
            //	 * haven't actually been used by anyone so far.
            //	 */
            //	natural_t	speculative_count;	/* # of pages
            // speculative */
            //
            // The following additional values are only available in the
            // macosx10.9 SDK:
            //	natural_t	compressor_page_count;	/* # of pages used by
            // the compressed pager to hold all the compressed data */ natural_t
            // throttled_count;	/* # of pages throttled */ 	natural_t
            // external_page_count;	/* # of pages that are file-backed
            //(non-swap) */ 	natural_t	internal_page_count;	/* # of
            // pages that are anonymous */
            //
            //  NOTE: free_count includes both speculative_count and
            //        external_page_count.
            //
            // In top (from the top-89.1.2 package):
            //   used = wire_count + inactive_count + active_count +
            //          compressor_page_count
            //   free = free_count
            //   wired = wire_count
            //
            // In Activity Monitor (from observations on 10.9):
            //   Memory used = internal_page_count + wire_count +
            //                 compressor_page_count +
            //                 external_page_count (approximately)
            //   App Memory = internal_page_count
            //   File Cache = external_page_count
            //   Wired Memory = wire_count
            //   Compressed = compressor_page_count
            //

            // Get total amount of physical memory using sysctl().
            if (ok && (physTotal || physUsed))
            {
                int mib[2] = {CTL_HW, HW_MEMSIZE};
                const unsigned int miblen = sizeof(mib) / sizeof(int);
                size_t len = sizeof(tmpPhysTotal);
                ok = (sysctl(mib, miblen, &tmpPhysTotal, &len, NULL, 0) == 0);
            }

            // Get the physical memory amounts.
            if (ok && (physFree || physUsed || physInactive || physCached))
            {
                mach_port_t port = mach_host_self();

                vm_size_t pageSize;
                host_page_size(port, &pageSize);

                vm_statistics64_data_t vm_stat;
                mach_msg_type_number_t count = HOST_VM_INFO64_COUNT;
                kern_return_t kr = host_statistics64(
                    port, HOST_VM_INFO64,
                    reinterpret_cast<host_info64_t>(&vm_stat), &count);

                if (kr == KERN_SUCCESS)
                {
                    tmpPhysFree = (vm_stat.free_count + vm_stat.inactive_count)
                                  * pageSize;
                    // Correspond approximately to active_count + wire_count +
                    // compressor_page_count.
                    tmpPhysUsed = tmpPhysTotal - tmpPhysFree;
                    tmpPhysInactive = vm_stat.inactive_count * pageSize;
                    tmpPhysCached = vm_stat.external_page_count * pageSize;
                }
                else
                {
                    ok = false;
                }
            }

            // Get the swap amounts.
            if (ok && (swapTotal || swapFree || swapUsed))
            {
                struct xsw_usage xsu;

                int mib[2] = {CTL_VM, VM_SWAPUSAGE};
                const unsigned int miblen = sizeof(mib) / sizeof(int);
                size_t len = sizeof(xsu);
                ok = (sysctl(mib, miblen, &xsu, &len, NULL, 0) == 0);

                if (ok)
                {
                    tmpSwapTotal = xsu.xsu_total;
                    tmpSwapFree = xsu.xsu_avail;
                    tmpSwapUsed = xsu.xsu_used;
                }
            }
        }

        if (!ok)
        {
            tmpPhysTotal = 0;
            tmpPhysFree = 0;
            tmpPhysUsed = 0;
            tmpPhysInactive = 0;
            tmpPhysCached = 0;
            tmpSwapTotal = 0;
            tmpSwapFree = 0;
            tmpSwapUsed = 0;
        }

        if (physTotal)
            *physTotal = tmpPhysTotal;
        if (physFree)
            *physFree = tmpPhysFree;
        if (physUsed)
            *physUsed = tmpPhysUsed;
        if (physInactive)
            *physInactive = tmpPhysInactive;
        if (physCached)
            *physCached = tmpPhysCached;
        if (swapTotal)
            *swapTotal = tmpSwapTotal;
        if (swapFree)
            *swapFree = tmpSwapFree;
        if (swapUsed)
            *swapUsed = tmpSwapUsed;

        return ok;
    }

#endif

    //----------------------------------------------------------------------
    //----------------------------------------------------------------------
    //
    //  LINUX
    //
    //----------------------------------------------------------------------
    //----------------------------------------------------------------------

#ifdef PLATFORM_LINUX

#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

    size_t SystemInfo::usableMemory()
    {
        if (!m_useableMemory)
        {
            int fd;

            ifstream meminfo("/proc/meminfo");

            if (!meminfo)
            {
                cerr << "WARNING: could not open /proc/meminfo, assuming 2Gb "
                        "of memory"
                     << endl;
                m_useableMemory =
                    size_t(2) * size_t(1024) * size_t(1024) * size_t(1024);
                return m_useableMemory;
            }

            size_t free = 0;
            size_t cached = 0;
            vector<string> tokens;

            while (meminfo.good())
            {
                string name;
                meminfo >> name;

                // if      (name == "MemFree:") meminfo >> free;
                // else if (name == "Cached:" ) meminfo >> cached;
                if (name == "MemTotal:")
                    meminfo >> free;

                while (meminfo.good() && meminfo.get() != '\n')
                    ;
            }

            m_useableMemory = (free + cached) * size_t(1024);
        }

        return m_useableMemory;
    }

    size_t SystemInfo::numCPUs()
    {
        ifstream cpuinfo("/proc/cpuinfo");

        if (!cpuinfo)
        {
            cerr << "WARNING: could not open /proc/cpuinfo, assuming 1 CPU"
                 << endl;
            return 1;
        }

        size_t num = 0;

        while (cpuinfo.good())
        {
            string name;
            cpuinfo >> name;

            if (name == "processor")
            {
                cpuinfo >> name; // read the colon
                cpuinfo >> num;  // last one read is the number
            }

            while (cpuinfo.good() && cpuinfo.get() != '\n')
                ;
        }

        return num + 1;
    }

    //------------------------------------------------------------------------------
    // Note: The following code was imported from Flame
    // (creative-finishing/common/src/base/osal/src/CoSystem_linux.cpp)
    //
    bool SystemInfo::getSystemMemoryInfo(size_t* physTotal, size_t* physFree,
                                         size_t* physUsed, size_t* physInactive,
                                         size_t* physCached, size_t* swapTotal,
                                         size_t* swapFree, size_t* swapUsed)
    {
        bool ok = (physTotal || physFree || physUsed || physInactive
                   || physCached || swapTotal || swapFree || swapUsed);

        size_t tmpPhysTotal = 0;
        size_t tmpPhysFree = 0;
        size_t tmpPhysUsed = 0;
        size_t tmpPhysInactive = 0;
        size_t tmpPhysCached = 0;
        size_t tmpSwapTotal = 0;
        size_t tmpSwapFree = 0;
        size_t tmpSwapUsed = 0;

        if (ok)
        {
            //
            // The following observations were made on RH5.5/RH6.2 and from the
            // procps-3.2.8-21.el6.src.rpm package.
            //
            // In /proc/meminfo (RH5.5):
            //   - MemFree doesn't include the Cached amount.
            //   - Inactive partially contains Cached so it's not possible to
            //   compute
            //     the amount of Inactive memory excluding Cached. Therefore, we
            //     ignore it and return 0 for physInactive.
            //   - Buffers is the amount of memory in the write file cache.
            //   - Cached is the amount of memory in the read file cache.
            //
            // In /proc/meminfo (RH6.2):
            //   - MemFree doesn't include the Cached and Inactive(anon)
            //   amounts.
            //   - Inactive = Inactive(anon) + Inactive(file)
            //   - Inactive(anon) is the amount of application memory that has
            //   not
            //     been recently used and can be reclaimed for other purposes.
            //   - Inactive(file) is the amount of Cached memory that has not
            //     been recently used and can be reclaimed for other purposes.
            //   - Buffers is the amount of memory in the write file cache.
            //   - Cached is the amount of memory in the read file cache.
            //
            // In top (from the procps-3.2.8-21.el6.src.rpm package source
            // code):
            //   free = MemFree
            //   used = MemTotal - MemFree
            //   buffers = Buffers
            //   cached = Cached
            //

            // Get total amount of physical memory using sysconf().
            if (physTotal || physUsed)
            {
                size_t pageSize = static_cast<size_t>(sysconf(_SC_PAGE_SIZE));
                tmpPhysTotal =
                    static_cast<size_t>(sysconf(_SC_PHYS_PAGES) * pageSize);
            }

            // Get other memory amounts from /proc/meminfo.
            int fd = open("/proc/meminfo", O_RDONLY);
            if (fd < 0)
                ok = false;

            if (ok)
            {
                const size_t bufsize = 4096;
                char buf[bufsize + 1];
                ssize_t len = 0;
                while (((len = read(fd, buf, bufsize)) < 0)
                       && (errno == EAGAIN || errno == EINTR))
                {
                }
                close(fd);

                ok = (len >= 0);
                if (ok)
                    buf[len] = '\0';

                size_t tmp = 0;
                char* p = NULL;

                // Get the physical memory amounts.
                if (ok && (physFree || physUsed || physInactive || physCached))
                {
                    p = strstr(buf, "MemFree:");
                    if (p && sscanf(p, "MemFree: %zu kB\n", &tmp) == 1)
                    {
                        tmpPhysFree = tmp * 1024;

                        p = strstr(buf, "Buffers:");
                        if (p && sscanf(p, "Buffers: %zu kB\n", &tmp) == 1)
                        {
                            tmpPhysCached = tmp * 1024;

                            p = strstr(buf, "Cached:");
                            if (p && sscanf(p, "Cached: %zu kB\n", &tmp) == 1)
                            {
                                tmpPhysCached += tmp * 1024;

                                // Try to get the Inactive(anon) value. Since
                                // it's not present on RH5.5 don't report an
                                // error if not present.
                                p = strstr(buf, "Inactive(anon):");
                                if (p
                                    && sscanf(p, "Inactive(anon): %zu kB\n",
                                              &tmp)
                                           == 1)
                                {
                                    tmpPhysInactive = tmp * 1024;
                                }

                                tmpPhysFree += tmpPhysCached + tmpPhysInactive;
                                tmpPhysUsed = tmpPhysTotal - tmpPhysFree;
                            }
                            else
                            {
                                ok = false;
                            }
                        }
                        else
                        {
                            ok = false;
                        }
                    }
                    else
                    {
                        ok = false;
                    }
                }

                // Get the swap amounts.
                if (ok && (swapTotal || swapFree || swapUsed))
                {
                    p = strstr(buf, "SwapTotal:");
                    if (p && sscanf(p, "SwapTotal: %zu kB\n", &tmp) == 1)
                    {
                        tmpSwapTotal = tmp * 1024;

                        p = strstr(buf, "SwapFree:");
                        if (p && sscanf(p, "SwapFree: %zu kB\n", &tmp) == 1)
                        {
                            tmpSwapFree = tmp * 1024;
                            tmpSwapUsed = tmpSwapTotal - tmpSwapFree;
                        }
                        else
                        {
                            ok = false;
                        }
                    }
                    else
                    {
                        ok = false;
                    }
                }
            }
        }

        if (!ok)
        {
            tmpPhysTotal = 0;
            tmpPhysFree = 0;
            tmpPhysUsed = 0;
            tmpPhysInactive = 0;
            tmpPhysCached = 0;
            tmpSwapTotal = 0;
            tmpSwapFree = 0;
            tmpSwapUsed = 0;
        }

        if (physTotal)
            *physTotal = tmpPhysTotal;
        if (physFree)
            *physFree = tmpPhysFree;
        if (physUsed)
            *physUsed = tmpPhysUsed;
        if (physInactive)
            *physInactive = tmpPhysInactive;
        if (physCached)
            *physCached = tmpPhysCached;
        if (swapTotal)
            *swapTotal = tmpSwapTotal;
        if (swapFree)
            *swapFree = tmpSwapFree;
        if (swapUsed)
            *swapUsed = tmpSwapUsed;

        return ok;
    }

#endif

    //----------------------------------------------------------------------
    //----------------------------------------------------------------------
    //
    //  WINDOWS
    //
    //----------------------------------------------------------------------
    //----------------------------------------------------------------------

#ifdef _MSC_VER

    /* AJG - numCPU's and physical memory functions */
    size_t SystemInfo::numCPUs()
    {
        SYSTEM_INFO nfo;
        GetSystemInfo(&nfo);

        return nfo.dwNumberOfProcessors;
    }

    size_t SystemInfo::usableMemory()
    {
        // mR - 10/29/07b
        MEMORYSTATUS memStat;

        GlobalMemoryStatus(&memStat);

        // return( memStat.dwAvailPhys ) ;
        return (memStat.dwTotalPhys);

        // ignored
        SYSTEM_INFO nfo;
        GetSystemInfo(&nfo);

        /* If you have 4 gig of ram, just return 4 billion - otherwise you're
         * past max unsigned int */
        if (nfo.dwPageSize > 4000)
            return std::numeric_limits<unsigned int>::max();
        else
            return size_t(1024) * size_t(nfo.dwPageSize) * size_t(1024);
    }

    //------------------------------------------------------------------------------
    //
    bool SystemInfo::getSystemMemoryInfo(size_t* physTotal, size_t* physFree,
                                         size_t* physUsed, size_t* physInactive,
                                         size_t* physCached, size_t* swapTotal,
                                         size_t* swapFree, size_t* swapUsed)
    {
        bool ok = (physTotal || physFree || physUsed || physInactive
                   || physCached || swapTotal || swapFree || swapUsed);

        // Not all features are implemented on windows - add implementation as
        // needed
        assert(
            !(physInactive || physCached || swapTotal || swapFree || swapUsed));

        size_t tmpPhysTotal = 0;
        size_t tmpPhysFree = 0;
        size_t tmpPhysUsed = 0;
        size_t tmpPhysInactive = 0;
        size_t tmpPhysCached = 0;
        size_t tmpSwapTotal = 0;
        size_t tmpSwapFree = 0;
        size_t tmpSwapUsed = 0;

        if (ok)
        {
            MEMORYSTATUS memStat;
            GlobalMemoryStatus(&memStat);

            tmpPhysTotal = memStat.dwTotalPhys;
            tmpPhysFree = memStat.dwAvailPhys;
            tmpPhysUsed = tmpPhysTotal - tmpPhysFree;
        }

        if (!ok)
        {
            tmpPhysTotal = 0;
            tmpPhysFree = 0;
            tmpPhysUsed = 0;
            tmpPhysInactive = 0;
            tmpPhysCached = 0;
            tmpSwapTotal = 0;
            tmpSwapFree = 0;
            tmpSwapUsed = 0;
        }

        if (physTotal)
            *physTotal = tmpPhysTotal;
        if (physFree)
            *physFree = tmpPhysFree;
        if (physUsed)
            *physUsed = tmpPhysUsed;
        if (physInactive)
            *physInactive = tmpPhysInactive;
        if (physCached)
            *physCached = tmpPhysCached;
        if (swapTotal)
            *swapTotal = tmpSwapTotal;
        if (swapFree)
            *swapFree = tmpSwapFree;
        if (swapUsed)
            *swapUsed = tmpSwapUsed;

        return ok;
    }

#endif

} // namespace TwkUtil

#ifdef _MSC_VER
#undef NOMINMAX
#endif

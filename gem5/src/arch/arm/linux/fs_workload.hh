/*
 * Copyright (c) 2010-2013 ARM Limited
 * All rights reserved
 *
 * The license below extends only to copyright in the software and shall
 * not be construed as granting a license to any other intellectual
 * property including but not limited to intellectual property relating
 * to a hardware implementation of the functionality of the software
 * licensed hereunder.  You may use the software subject to the license
 * terms below provided that you ensure that this notice is replicated
 * unmodified and in its entirety in all distributions of the software,
 * modified or unmodified, in source code or in binary form.
 *
 * Copyright (c) 2002-2005 The Regents of The University of Michigan
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer;
 * redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution;
 * neither the name of the copyright holders nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __ARCH_ARM_LINUX_FS_WORKLOAD_HH__
#define __ARCH_ARM_LINUX_FS_WORKLOAD_HH__

#include <cstdio>
#include <map>
#include <string>

#include "arch/arm/aapcs32.hh"
#include "arch/arm/aapcs64.hh"
#include "arch/arm/fs_workload.hh"
#include "arch/arm/system.hh"
#include "base/output.hh"
#include "kern/linux/events.hh"
#include "params/ArmFsLinux.hh"

namespace gem5
{

namespace ArmISA
{

class SkipFuncLinux32 : public SkipFunc
{
  public:
    using SkipFunc::SkipFunc;
    using ABI = Aapcs32Vfp;
};

class SkipFuncLinux64 : public SkipFunc
{
  public:
    using SkipFunc::SkipFunc;
    using ABI = Aapcs64;
};

class FsLinux : public ArmISA::FsWorkload
{
  protected:
    /**
     * PC based event to skip the dprink() call and emulate its
     * functionality
     */
    PCEvent *debugPrintk = nullptr;

    PCEvent *dumpStats = nullptr;

  public:
    /** Boilerplate params code */
    PARAMS(ArmFsLinux);

    /** When enabled, dump stats/task info on context switches for
     *  Streamline and per-thread cache occupancy studies, etc. */
    bool enableContextSwitchStatsDump;

    /** This map stores a mapping of OS process IDs to internal Task IDs. The
     * mapping is done because the stats system doesn't tend to like vectors
     * that are much greater than 1000 items and the entire process space is
     * 65K. */
    std::map<uint32_t, uint32_t> taskMap;

    /** This is a file that is placed in the run directory that prints out
     * mappings between taskIds and OS process IDs */
    OutputStream *taskFile = nullptr;

    FsLinux(const Params &p);
    ~FsLinux();

    void initState() override;

    void startup() override;

    /** This function creates a new task Id for the given pid.
     * @param tc thread context that is currentyl executing  */
    void mapPid(ThreadContext* tc, uint32_t pid);

  public: // Exported Python methods
    /**
     * Dump the kernel's dmesg buffer to stdout
     */
    void dumpDmesg();

  private:
    /** Event to halt the simulator if the kernel calls panic()  */
    PCEvent *kernelPanic = nullptr;

    /** Event to halt the simulator if the kernel calls oopses  */
    PCEvent *kernelOops = nullptr;

    /**
     * PC based event to skip udelay(<time>) calls and quiesce the
     * processor for the appropriate amount of time. This is not functionally
     * required but does speed up simulation.
     */
    PCEvent *skipUDelay = nullptr;

    /** Another PC based skip event for const_udelay(). Similar to the udelay
     * skip, but this function precomputes the first multiply that is done
     * in the generic case since the parameter is known at compile time.
     * Thus we need to do some division to get back to us.
     */
    PCEvent *skipConstUDelay = nullptr;
};

class DumpStats : public PCEvent
{
  public:
    DumpStats(PCEventScope *s, const std::string &desc, Addr addr)
        : PCEvent(s, desc, addr)
    {}

    void process(ThreadContext* tc) override;
  protected:
    virtual void getTaskDetails(ThreadContext *tc, uint32_t &pid,
            uint32_t &tgid, std::string &next_task_str, int32_t &mm);

};

class DumpStats64 : public DumpStats
{
  public:
    using DumpStats::DumpStats;

  private:
    void getTaskDetails(ThreadContext *tc, uint32_t &pid, uint32_t &tgid,
                        std::string &next_task_str, int32_t &mm) override;
};

} // namespace ArmISA
} // namespace gem5

#endif // __ARCH_ARM_LINUX_FS_WORKLOAD_HH__

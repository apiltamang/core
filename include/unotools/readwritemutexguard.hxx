/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * This file incorporates work covered by the following license notice:
 *
 *   Licensed to the Apache Software Foundation (ASF) under one or more
 *   contributor license agreements. See the NOTICE file distributed
 *   with this work for additional information regarding copyright
 *   ownership. The ASF licenses this file to you under the Apache
 *   License, Version 2.0 (the "License"); you may not use this file
 *   except in compliance with the License. You may obtain a copy of
 *   the License at http://www.apache.org/licenses/LICENSE-2.0 .
 */

#ifndef INCLUDED_UNOTOOLS_READWRITEMUTEXGUARD_HXX
#define INCLUDED_UNOTOOLS_READWRITEMUTEXGUARD_HXX

#include <osl/mutex.hxx>
#include <o3tl/typed_flags_set.hxx>

enum class ReadWriteGuardMode {
    ReadOnly       = 0x00,
    Write          = 0x01,
    CriticalChange = 0x02 | Write,
    BlockCritical  = 0x04,     // only a block, not a read, exclusive flag!
};
namespace o3tl {
    template<> struct typed_flags<ReadWriteGuardMode> : is_typed_flags<ReadWriteGuardMode, 0x7> {};
}

namespace utl {

class ReadWriteMutex
{
    friend class ReadWriteGuard;

            sal_uInt32          nReadCount;
            sal_uInt32          nBlockCriticalCount;
            ::osl::Mutex        maMutex;
            ::osl::Mutex        maWriteMutex;

public:
                                ReadWriteMutex()
                                    : nReadCount(0)
                                    , nBlockCriticalCount(0)
                                    {}
};

/** Enable multiple threads to read simultaneously, but a write blocks all
    other reads and writes, and a read blocks any write.
    Used in I18N wrappers to be able to maintain a single instance of a wrapper
    for the standard Office locale.
    NEVER construct a writing guard if there is already a reading guard in the
    same context, the following will dead lock EVEN IN THE SAME THREAD!
    void foo()
    {
        ReadWriteGuard aGuard1( aMutex );
        bar();
    }
    void bar()
    {
        // waits forever for aGuard1
        ReadWriteGuard aGuard2( aMutex, ReadWriteGuardMode::nWrite );
    }
 */
class ReadWriteGuard
{
            ReadWriteMutex&     rMutex;
            ReadWriteGuardMode  nMode;
public:
                                ReadWriteGuard(
                                    ReadWriteMutex& rMutex,
                                    ReadWriteGuardMode nRequestMode = ReadWriteGuardMode::ReadOnly  // read only
                                    );
                                ~ReadWriteGuard();

    /** Be careful with this, it does wait for ANY read to complete.
        The following will dead lock EVEN IN THE SAME THREAD!
        void foo()
        {
            ReadWriteGuard aGuard1( aMutex );
            bar();
        }
        void bar()
        {
            ReadWriteGuard aGuard2( aMutex );
            aGuard2.changeReadToWrite();    // waits forever for aGuard1
        }
     */
            void                changeReadToWrite();
};

}   // namespace utl

#endif // INCLUDED_UNOTOOLS_READWRITEMUTEXGUARD_HXX

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

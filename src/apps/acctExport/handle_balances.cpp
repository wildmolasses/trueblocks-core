/*-------------------------------------------------------------------------
 * This source code is confidential proprietary information which is
 * Copyright (c) 2017 by Great Hill Corporation.
 * All Rights Reserved
 *------------------------------------------------------------------------*/
#include "options.h"

int nn = 0;
#define HERE(a) if (isTestMode()) { cout << ++nn << ": " << (a) << endl; cout << "----------------------------------------------" << endl; }
//-----------------------------------------------------------------------
bool COptions::exportBalances(void) {

    ENTER("exportBalances");

    // If the node we're running against does not provide balances...
    bool nodeHasBals = nodeHasBalances();
    string_q rpcProvider = getGlobalConfig()->getConfigStr("settings", "rpcProvider", "http://localhost:8545");
    if (!nodeHasBals) {
        string_q balanceProvider = getGlobalConfig()->getConfigStr("settings", "balanceProvider", rpcProvider);

        // ...and the user has told us about another balance provider...
        if (rpcProvider == balanceProvider || balanceProvider.empty()) {
            EXIT_FAIL("Balances not available.");
        }

        // ..then we release the curl context, change the node server, and get a new context. We will replace this below.
        getCurlContext()->baseURL = balanceProvider;
        getCurlContext()->releaseCurl();
        getCurlContext()->getCurl();
    }

    bool isJson = (exportFmt == JSON1 || exportFmt == API1 || exportFmt == NONE1);
    if (isJson && !freshen_only)
        cout << "[";

    for (auto monitor : monitors) {

        CAppearanceArray_base apps;
        loadOneAddress(apps, monitor.address);

        map<blknum_t, CBalanceDelta> deltas;
        string_q balFile = getMonitorBals(monitor.address);
        if (fileExists(balFile)) {
            CArchive balIn(READING_ARCHIVE);
            if (balIn.Lock(balFile, modeReadOnly, LOCK_NOWAIT)) {
                uint64_t size;
                balIn >> size;
                while (deltas.size() < size) {
                    CBalanceDelta rec;
                    balIn >> rec;
                    rec.address = monitor.address;
                    deltas[rec.blockNumber] = rec;
                };
                balIn.Release();
            }
HERE("as read")
if (isTestMode()) {
    for (auto delta : deltas)
        cout << delta.first << "\t" << delta.second;
}
        }

        bool first = true;
        for (size_t i = 0 ; i < apps.size() && !shouldQuit() && apps[i].blk < ts_cnt ; i++) {

            const CAppearance_base *item = &apps[i];
            if (inRange((blknum_t)item->blk, scanRange.first, scanRange.second)) {

                CBalanceRecord record;
                record.blockNumber = item->blk;
                record.transactionIndex = item->txid;
                record.address = monitor.address;

                // handle the prior balance -- note we always have this in the delta map other than zero block
                record.priorBalance = record.blockNumber == 0 ? 0 : getBalanceAt(monitor.address, record.blockNumber - 1);
                record.balance = getBalanceAt(monitor.address, record.blockNumber);
#if 0
                    auto it = deltas.lower_bound(record.blockNumber - 1);
                    record.priorBalance = it->second.balance;

                    it = deltas.lower_bound(record.blockNumber);
                    if (it != deltas.end()) {
                        record.balance = it->second.balance;
                    } else {
                        record.balance = getBalanceAt(monitor.address, record.blockNumber);
                    }
#endif
                record.diff = (bigint_t(record.balance) - bigint_t(record.priorBalance));

                if (!freshen_only && !deltas_only) {
                    if (isJson && !first)
                        cout << ", ";
                    cout << record;
                    nExported++;
                    first = false;
                }

                CBalanceDelta rec;
                rec.blockNumber = record.blockNumber;
                rec.address = monitor.address;
                rec.balance = record.balance;
                rec.diff = record.diff;
                if (record.diff != 0)
                    deltas[rec.blockNumber] = rec;
            }
        }

        if (deltas_only) {
            for (auto delta : deltas) {
                if (isJson && !first)
                    cout << ", ";
                cout << delta.second;
                nExported++;
                first = false;
            }
        }

        // cache the deltas
        CArchive balOut(WRITING_ARCHIVE);
        if (balOut.Lock(balFile, modeWriteCreate, LOCK_NOWAIT)) {
            balOut << (uint64_t)deltas.size();
            for (auto delta : deltas) {
                balOut << delta.second;
            }
            balOut.Release();
        }

HERE("Out")
if (isTestMode()) {
    for (auto delta : deltas)
        cout << delta.first << "\t" << delta.second;
}

    }

    if (isJson && !freshen_only)
        cout << "]";

    // return to the default provider
    if (!nodeHasBals) {
        getCurlContext()->baseURL = rpcProvider;
        getCurlContext()->releaseCurl();
        getCurlContext()->getCurl();
    }

    EXIT_NOMSG(true);
}

    // So as to keep the file small, we only write balances where there is a change
#if 0
    if (balances.size() == 0 && fileExists(binaryFilename) && fileSize(binaryFilename) > 0) {

        CArchive balCache(READING_ARCHIVE);
        if (balCache.Lock(binaryFilename, modeReadOnly, LOCK_NOWAIT)) {
            blknum_t last = NOPOS;
            address_t lastA;
            do {
                blknum_t bn;
                address_t addr1;
                biguint_t bal;
                balCache >> bn >> addr1 >> bal;
                if (monitor.address == addr1) {
                    if (last != bn || bal != 0) {
                        CEthState newBal;
                        newBal.blockNumber = bn;
                        newBal.balance = bal;
                        record.push_back(newBal);
                        last = bn;
                    }
                }
            } while (!balCache.Eof());
        }
    }
#endif

#if 0
    // First, we try to find it using a binary search. Many times this will hit...
    CEthState search;
    search.blockNumber = blockNum;
    const CEthStateArray::iterator it = find(record.begin(), record.end(), search);
    if (it != record.end())
        return it->balance;

    // ...if it doesn't hit, we need to find the most recent balance
    biguint_t ret = 0;
    for (size_t i = 0 ; i < record.size() ; i++) {
        // if we hit the block number exactly return it
        if (record[i].blockNumber == blockNum)
            return record[i].balance;

        // ...If we've overshot, report the previous balance
        if (record[i].blockNumber > blockNum)
            return ret;

        ret = record[i].balance;
    }

    // We've run off the end of the array, return the most recent balance (if any)
    if (ret > 0)
        return ret;

    // We finally fall to the node in case we're near the head
    return getBalanceAt(addr, blockNum);
}
#endif

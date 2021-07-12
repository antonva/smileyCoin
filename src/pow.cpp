// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 The Bitcoin Core developers
// Copyright (c) 2021-2021 The Smileycoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <pow.h>

#include <multialgo/multialgo.h>
#include <arith_uint256.h>
#include <chain.h>
#include <primitives/block.h>
#include <uint256.h>
#include <chainparams.h>

/**
 * [smly] The multi algo fork. TODO Wax poetic
 */
unsigned int GetNextWorkRequired(const CBlockIndex *pindexLast, const CBlockHeader *pblock, const Consensus::Params &params)
{
    assert(pindexLast != nullptr);

    // [smly] Gate off original GetNextWorkRequired functionality
    if ((pindexLast->nHeight+1) < params.MultiAlgoForkHeight) {
        unsigned int nProofOfWorkLimit = UintToArith256(params.powLimit).GetCompact();
        // Only change once per difficulty adjustment interval
        if ((pindexLast->nHeight+1) % DifficultyAdjustmentInterval(params, pindexLast->nHeight+1) != 0)
        {
            if (params.fPowAllowMinDifficultyBlocks)
            {
                // Special difficulty rule for testnet:
                // If the new block's timestamp is more than 2* 10 minutes
                // then allow mining of a min-difficulty block.
                if (pblock->GetBlockTime() > pindexLast->GetBlockTime() + params.nPowTargetSpacing*2)
                    return nProofOfWorkLimit;
                else
                {
                    // Return the last non-special-min-difficulty-rules-block
                    const CBlockIndex* pindex = pindexLast;
                    while (pindex->pprev && pindex->nHeight % DifficultyAdjustmentInterval(params, pindex->nHeight) != 0 && pindex->nBits == nProofOfWorkLimit)
                        pindex = pindex->pprev;
                    return pindex->nBits;
                }
            }
            return pindexLast->nBits;
        }

        // Go back by what we want to be 14 days worth of blocks
        // Litecoin: This fixes an issue where a 51% attack can change difficulty at will.
        // Go back the full period unless it's the first retarget after genesis. Code courtesy of Art Forz
        int blockstogoback = DifficultyAdjustmentInterval(params, pindexLast->nHeight+1)-1;
        if ((pindexLast->nHeight+1) != DifficultyAdjustmentInterval(params,pindexLast->nHeight+1))
            blockstogoback = DifficultyAdjustmentInterval(params, pindexLast->nHeight+1);

        // Go back by what we want to be 14 days worth of blocks
        const CBlockIndex* pindexFirst = pindexLast;
        for (int i = 0; pindexFirst && i < blockstogoback; i++)
            pindexFirst = pindexFirst->pprev;

        assert(pindexFirst);

        return CalculateNextWorkRequired(pindexLast, pindexFirst->GetBlockTime(), params);
    }

    return GetNextMultiAlgoWorkRequired(pindexLast, pblock, params);
}

unsigned int GetNextMultiAlgoWorkRequired(const CBlockIndex* pindexLast, const CBlockHeader *pblock, const Consensus::Params& params)
{
    // [smly] No need to assert that pindexLast isn't null as that's handled by GetNextWorkRequired.
    int algo = pblock->GetAlgo();
    bool fDiffChange = pindexLast->nHeight >= params.DifficultyChangeForkHeight;
    unsigned int nProofOfWorkLimit = UintToArith256(params.powLimit).GetCompact();

    // Only change once per difficulty adjustment interval
    if ((pindexLast->nHeight + 1) % DifficultyAdjustmentInterval(params, pindexLast->nHeight + 1) != 0) {
        if (params.fPowAllowMinDifficultyBlocks) {
            // Special difficulty rule for testnet:
            // If the new block's timestamp is more than 2* 10 minutes
            // then allow mining of a min-difficulty block.
            if (pblock->GetBlockTime() > pindexLast->GetBlockTime() + params.nPowTargetSpacing * 2)
                return nProofOfWorkLimit;
        }
        return pindexLast->nBits;
    }

    // Find first block in averaging interval
    // Go back by what we want to be nAveragingInterval blocks per algo
    const CBlockIndex* pindexFirst = pindexLast;
    int64_t nAveragingInterval = MultiAlgoAveragingInterval(params, pindexLast->nHeight);
    for (int i = 0; pindexFirst && i < params.nMultiAlgoNum * nAveragingInterval; ++i)
    {
        pindexFirst = pindexFirst->pprev;
    }

    const CBlockIndex* pindexPrevAlgo = GetLastBLockIndexForAlgo(pindexLast, params, algo);
    if (pindexPrevAlgo == nullptr || pindexFirst == nullptr) {
        return nProofOfWorkLimit;
    }

    params.nMultiAlgoTargetSpacing = MultiAlgoTimespan(params, pindexLast->nHeight);
    // Limit adjustment step
    // Use medians to prevent time-warp attacks
    int64_t nActualTimespan = pindexLast->GetMedianTimePast() - pindexFirst->GetMedianTimePast();
    // nAveragingTargetTimespan was initially not initialized correctly (heh)
    int64_t nAveragingTargetTimespan;
    if (fDiffChange) {
        nAveragingTargetTimespan = params.nMultiAlgoAveragingIntervalV2 * params.nMultiAlgoTargetSpacing; // 2* 5 * 180 = 1800 seconds
        nActualTimespan = nAveragingTargetTimespan + (nActualTimespan - nAveragingTargetTimespan) / 4;
    } else {
        nActualTimespan = nAveragingTargetTimespan + (nActualTimespan - nAveragingTargetTimespan) / 4;
        nAveragingTargetTimespan = nAveragingInterval * multiAlgoTargetSpacing; // 60* 5 * 180 = 54000 seconds
    }


    // debug print
    LogPrintf("nTargetTimespan = %d    nActualTimespan = %d\n", retargetTimespan, nActualTimespan);
    LogPrintf("Before: %08x  %s\n", pindexLast->nBits, ArithToUint256(bnBefore).ToString());
    LogPrintf("After:  %08x  %s\n", bnNew.GetCompact(), ArithToUint256(bnNew).ToString());

    return bnNew.GetCompact();
}

unsigned int CalculateNextWorkRequired(const CBlockIndex* pindexLast, int64_t nFirstBlockTime, const Consensus::Params& params)
{
    if (params.fPowNoRetargeting)
        return pindexLast->nBits;

    // [smly] The timespan was changed at block 97050
    int64_t nPowTargetTimespan = params.nPowTargetTimespan;
    if (pindexLast->nHeight < params.FirstTimespanChangeHeight)
        nPowTargetTimespan = params.nPowOriginalTargetTimespan;

    // Limit adjustment step
    int64_t nActualTimespan = pindexLast->GetBlockTime() - nFirstBlockTime;
    if (nActualTimespan < nPowTargetTimespan/4)
        nActualTimespan = nPowTargetTimespan/4;
    if (nActualTimespan > nPowTargetTimespan*4)
        nActualTimespan = nPowTargetTimespan*4;

    // Retarget
    arith_uint256 bnNew;
    arith_uint256 bnOld;
    bnNew.SetCompact(pindexLast->nBits);
    bnOld = bnNew;
    // Litecoin: intermediate uint256 can overflow by 1 bit
    const arith_uint256 bnPowLimit = UintToArith256(params.powLimit);
    bool fShift = bnNew.bits() > bnPowLimit.bits() - 1;
    if (fShift)
        bnNew >>= 1;
    bnNew *= nActualTimespan;
    bnNew /= nPowTargetTimespan;
    if (fShift)
        bnNew <<= 1;

    if (bnNew > bnPowLimit)
        bnNew = bnPowLimit;

    return bnNew.GetCompact();
}

bool CheckProofOfWork(uint256 hash, unsigned int nBits, const Consensus::Params& params)
{
    bool fNegative;
    bool fOverflow;
    arith_uint256 bnTarget;

    bnTarget.SetCompact(nBits, &fNegative, &fOverflow);

    // Check range
    if (fNegative || bnTarget == 0 || fOverflow || bnTarget > UintToArith256(params.powLimit))
        return false;

    // Check proof of work matches claimed amount
    if (UintToArith256(hash) > bnTarget)
        return false;

    return true;
}

const CBlockIndex* GetLastBlockIndexForAlgo(const CBlockIndex* pindex, const Consensus::Params&, int algo)
{
    for (; pindex; pindex = pindex->pprev)
    {
        if (pindex->GetAlgo() != algo)
            continue;
        // Ignore special min-difficulty testnet blocks
        if (params.fPowAllowwMinDifficultyBlocks &&
            pindex->pprev &&
            pindex->nTime > pindex->pprev->nTime + params.nTargetSpacing * 2)
        {
            continue;
        }
        return pindex;
    }
    return nullptr;
}

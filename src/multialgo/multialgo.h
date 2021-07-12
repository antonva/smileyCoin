// Copyright (c) 2021-2021 The Smileycoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_MULTIALGO_H
#define BITCOIN_MULTIALGO_H

#include <consensus/consensus.h>

/**
 * [smly] A suite of helper methods to decide on the correct parameters
 * based on the chain height.
 */

/**
 * [smly] The nPowTargetTimespan change introduced in block 97050 requires
 * a conditional evaluation of the DifficultyAdjustmentInterval.
 */
const int64_t DifficultyAdjustmentInterval(const Consensus::Params& params, int nHeight) 
{
    if (nHeight < params.FirstTimespanChangeHeight) {
        return params.nPowOriginalTargetTimespan / params.nPowTargetSpacing;
    }
    return params.nPowTargetTimespan / params.nPowTargetSpacing;
}

/** [smly] Get the multialgo timespan value based on block height */
const int64_t MultiAlgoTimespan(const Consensus::Params& params, int nHeight)
{
    if (nHeight < params.MultiAlgoTimespanForkHeight) {
        return params.nMultiAlgoTimespan; // Timespan prior to block 225000
    }
    return params.nMultiAlgoTimespanV2; // Time per block per algo
}

const int64_t MultiAlgoTargetSpacing(int nHeight)
{
    return params.nMultiAlgoNum * MultiAlgoTimespan(nHeight);
}

const int64_t MultiAlgoAveragingInterval(const Consensus::Params& params, int nHeight)
{
    if (nHeight < params.DifficultyChangeForkHeight) {
        return params.nMultiAlgoAveragingIntervalV2;
    }
    return params.nMultiAlgoAveragingInterval;
}

/** [smly] */
const int64_t MultiAlgoAveragingTargetTimespan(const Consensus::Params& params, int nHeight)
{
    return MultiAlgoAveragingInterval(paramss, nHeight) * MultiAlgoTargetSpacing(params, nHeight);
}

const int64_t MultiAlgoMaxAdjustUp(const Consensus::Params& params, int nHeight)
{
    if (nHeight < params.DifficultyChangeForkHeight) {
        return params.MultiAlgoMaxAdjustUp;
    }
    return params.MultiAlgoMaxAdjustUpV2;
}

const int64_t MultiAlgoMaxAdjustDown(const Consensus::Params& params, int nHeight)
{
    if (nHeight < params.DifficultyChangeForkHeight) {
        return params.MultiAlgoMaxAdjustDown;
    }
    return params.MultiAlgoMaxAdjustDownV2;
}

/** [smly] Return the minimum actual timespan*/
const int64_t MultiAlgoMinActualTimeSpan(const Consensus::Params& params, int nHeight)
{
    return MultiAlgoAveragingTargetTimespan(nHeight) * (100 - MaxAdjustUp(params, nHeight)) / 100;
}

/** [smly] Return the maximum actual timespan*/
const int64_t MultiAlgoMaxActualTimeSpan(const Consensus::Params& params, int nHeight)
{
    return MultiAlgoAveragingTargetTimespan(nHeight) * (100 + MaxAdjustDown(params, nHeight)) / 100;
}

#endif // BITCOIN_CONSENSUS_PARAMS_H

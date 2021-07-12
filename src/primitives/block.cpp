// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <primitives/block.h>

#include <arith_uint256.h>
#include <consensus/consensus.h>
#include <crypto/common.h>
#include <multialgo/hashgroestl.h>
#include <multialgo/hashqubit.h>
#include <multialgo/hashskein.h>
#include <crypto/scrypt.h>
#include <hash.h>
#include <tinyformat.h>
#include <util/strencodings.h>

uint256 CBlockHeader::GetHash() const
{
    return SerializeHash(*this);
}

int CBlockHeader::GetAlgo() const
{
    switch (nVersion & BLOCK_VERSION_ALGO) {
    case 1:
        return ALGO_SCRYPT;
    case BLOCK_VERSION_SCRYPT:
        return ALGO_SCRYPT;
    case BLOCK_VERSION_SHA256D:
        return ALGO_SHA256D;
    case BLOCK_VERSION_GROESTL:
        return ALGO_GROESTL;
    case BLOCK_VERSION_SKEIN:
        return ALGO_SKEIN;
    case BLOCK_VERSION_QUBIT:
        return ALGO_QUBIT;
    }
    return ALGO_SCRYPT;
}

uint256 CBlockHeader::GetPoWHash() const
{
    switch (GetAlgo()) {
    case ALGO_SHA256D:
        return GetHash();
    case ALGO_SCRYPT:
    {
        uint256 thash;
        scrypt_1024_1_1_256(BEGIN(nVersion), BEGIN(thash));
        return thash;
    }
    case ALGO_GROESTL:
        return HashGroestl(BEGIN(nVersion), END(nNonce));
    case ALGO_SKEIN:
        return HashSkein(BEGIN(nVersion), END(nNonce));
    case ALGO_QUBIT:
        return HashQubit(BEGIN(nVersion), END(nNonce));
    }
}

std::string CBlock::ToString() const
{
    std::stringstream s;
    s << strprintf("CBlock(hash=%s, ver=0x%08x, pow_algo=%d, pow_hash=%s, hashPrevBlock=%s, hashMerkleRoot=%s, nTime=%u, nBits=%08x, nNonce=%u, vtx=%u)\n",
        GetHash().ToString(),
        nVersion,
        GetAlgo(),
        GetPoWHash().ToString(),
        hashPrevBlock.ToString(),
        hashMerkleRoot.ToString(),
        nTime, nBits, nNonce,
        vtx.size());
    for (const auto& tx : vtx) {
        s << "  " << tx->ToString() << "\n";
    }
    return s.str();
}

std::string GetAlgoName(int algo)
{
    switch (algo) {
    case ALGO_SHA256D:
        return std::string("sha256d");
    case ALGO_SCRYPT:
        return std::string("scrypt");
    case ALGO_GROESTL:
        return std::string("groestl");
    case ALGO_SKEIN:
        return std::string("skein");
    case ALGO_QUBIT:
        return std::string("qubit");
    }
    return std::string("unknown");
}

int GetAlgoByName(std::string strAlgo, int fallback)
{
    transform(strAlgo.begin(), strAlgo.end(), strAlgo.begin(), ::tolower);
    if (strAlgo == "sha" || strAlgo == "sha256" || strAlgo == "sha256d")
        return ALGO_SHA256D;
    if (strAlgo == "scrypt")
        return ALGO_SCRYPT;
    if (strAlgo == "groestl")
        return ALGO_GROESTL;
    if (strAlgo == "skein")
        return ALGO_SKEIN;
    if (strAlgo == "qubit")
        return ALGO_QUBIT;
    return fallback;
}

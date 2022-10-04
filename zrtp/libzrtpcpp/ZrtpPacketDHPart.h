/*
 * Copyright 2006 - 2018, Werner Dittmann
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *         http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _ZRTPPACKETDHPART_H_
#define _ZRTPPACKETDHPART_H_

/**
 * @file ZrtpPacketDHPart.h
 * @brief The ZRTP DHPart message
 *
 * @ingroup ZRTP
 * @{
 */

#include <libzrtpcpp/ZrtpPacketBase.h>
#include <common/typedefs.h>

/**
 * Implement the DHPart packet.
 *
 * The ZRTP message DHPart. The implementation sends this
 * to exchange the Diffie-Hellman public keys and the shared
 * secrets between the two parties.
 *
 * @author Werner Dittmann <Werner.Dittmann@t-online.de>
 */

class __EXPORT ZrtpPacketDHPart : public ZrtpPacketBase {

public:
    /// Creates a DHPart packet no data, must use setPubKeyType(...)
    ZrtpPacketDHPart();

    /// Creates a DHPart packet from received data
    explicit ZrtpPacketDHPart(uint8_t const * data);

    /// Standard destructor
    ~ZrtpPacketDHPart() override = default;

    /// Get pointer to public key value, variable length byte array
    [[nodiscard]] uint8_t* getPv() const            { return pv; }

    /// Get pointer to first retained secret id, fixed length byte array
    [[nodiscard]] uint8_t* getRs1Id() const         { return DHPartHeader->rs1Id; };

    /// Get pointer to second retained secret id, fixed length byte array
    [[nodiscard]] uint8_t* getRs2Id() const         { return DHPartHeader->rs2Id; };

    /// Get pointer to additional retained secret id, fixed length byte array
    [[nodiscard]] uint8_t* getAuxSecretId() const   { return DHPartHeader->auxSecretId; };

    /// Get pointer to PBX retained secret id, fixed length byte array
    [[nodiscard]] uint8_t* getPbxSecretId() const   { return DHPartHeader->pbxSecretId; };

    /// Get pointer to first hash (H1) for hash chain, fixed length byte array
    [[nodiscard]] uint8_t* getH1() const            { return DHPartHeader->hashH1; };

    /// Get pointer to HMAC, fixed length byte array
    [[nodiscard]] uint8_t* getHMAC() const          { return pv+roundUp; };

    /// Check if packet length makes sense. DHPart packets are 29 words at minumum, using E255
    [[nodiscard]] bool isLengthOk() const           {return (getLength() >= 29);}

    /// Set public key value, variable length byte array
    void setPv(uint8_t const * text)         { memcpy(pv, text, dhLength); };

    /// Set first retained secret id, fixed length byte array
    void setRs1Id(zrtp::RetainedSecArray const & text) { memcpy(DHPartHeader->rs1Id, text.data(), sizeof(DHPartHeader->rs1Id)); };

    /// Set second retained secret id, fixed length byte array
    void setRs2Id(zrtp::RetainedSecArray const & text) { memcpy(DHPartHeader->rs2Id, text.data(), sizeof(DHPartHeader->rs2Id)); };

    /// Set additional retained secret id, fixed length byte array
    void setAuxSecretId(zrtp::RetainedSecArray const & text) { memcpy(DHPartHeader->auxSecretId, text.data(), sizeof(DHPartHeader->auxSecretId)); };

    /// Set PBX retained secret id, fixed length byte array
    void setPbxSecretId(zrtp::RetainedSecArray const & text) { memcpy(DHPartHeader->pbxSecretId, text.data(), sizeof(DHPartHeader->pbxSecretId)); };

    /// Set first hash (H1) of hash chain, fixed length byte array
    void setH1(uint8_t const * t)            { memcpy(DHPartHeader->hashH1, t, sizeof(DHPartHeader->hashH1)); };

    /// Set key length and compute overall packet length
    void setPacketLength(size_t pubKeyLen);

    /// Set first MAC, fixed length byte array
    void setHMAC(zrtp::ImplicitDigest const & hmac) { memcpy(pv+roundUp, hmac.data(), 2*ZRTP_WORD_SIZE); };

private:
    void initialize();
    uint8_t  * pv = nullptr;                  ///< points to public key value inside DH message
    DHPart_t* DHPartHeader = nullptr;         ///< points to DH message structure
    size_t dhLength = 0;                      ///< length of public key
    size_t roundUp = 0;                       ///< Public key length, rounded up to ZRTP_WORD_SIZE

    // DHPart packet is of variable length:
    // - 21 words fixed size
    // - up to 539 words variable part, depending on algorithm (max: NP12 ciphertext + EC414 compressed)
    //   leads to a maximum of 4*560=2240 bytes.
    // - HMAC (2 words)
    // - CRC (1 word)
    uint8_t data[2300] = {};       // large enough to hold a full-blown DHPart packet (DHPart1 only)
};

/**
 * @}
 */
#endif // _ZRTPPACKETDHPART_H_


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

#ifndef _ZRTP_H_
#define _ZRTP_H_
/**
 * @file ZRtp.h
 * @brief The ZRTP main engine
 * @defgroup ZRTP ZRTP C++ implementation
 * @{
 */

#include <cstdlib>

#include <common/SecureArray.h>
#include <libzrtpcpp/ZrtpPacketHello.h>
#include <libzrtpcpp/ZrtpPacketHelloAck.h>
#include <libzrtpcpp/ZrtpPacketCommit.h>
#include <libzrtpcpp/ZrtpPacketDHPart.h>
#include <libzrtpcpp/ZrtpPacketConfirm.h>
#include <libzrtpcpp/ZrtpPacketConf2Ack.h>
#include <libzrtpcpp/ZrtpPacketGoClear.h>
#include <libzrtpcpp/ZrtpPacketClearAck.h>
#include <libzrtpcpp/ZrtpPacketError.h>
#include <libzrtpcpp/ZrtpPacketErrorAck.h>
#include <libzrtpcpp/ZrtpPacketPing.h>
#include <libzrtpcpp/ZrtpPacketPingAck.h>
#include <libzrtpcpp/ZrtpPacketSASrelay.h>
#include <libzrtpcpp/ZrtpPacketRelayAck.h>
#include <libzrtpcpp/ZrtpCallback.h>
#include <libzrtpcpp/ZIDCache.h>

#include <botan_all.h>

#include "common/typedefs.h"

#include "../logging/ZrtpLogging.h"
#include "libzrtpcpp/ZrtpStateEngine.h"

class __EXPORT ZrtpStateEngineImpl;
class ZrtpDH;
class ZRtp;

/**
 * The main ZRTP class.
 *
 * This is the main class of the RTP/SRTP independent part of the GNU
 * ZRTP. It handles the ZRTP HMAC, DH, and other data management. The
 * user of this class needs to know only a few methods and needs to
 * provide only a few external functions to connect to a Timer
 * mechanism and to send data via RTP and SRTP. Refer to the
 * ZrtpCallback class to get detailed information regarding the
 * callback methods required by GNU RTP.
 *
 * This class does not directly handle the protocol states, timers,
 * and packet resend. The protocol state engine is responsible for
 * these actions.
 *
 * Example how to use ZRtp:
 *<pre>
 *    zrtpEngine = new ZRtp((uint8_t*)ownZid, (ZrtpCallback*)this, idString);
 *    zrtpEngine->startZrtpEngine();
 *</pre>
 * @see ZrtpCallback
 *
 * @author Werner Dittmann <Werner.Dittmann@t-online.de>
 */
class __EXPORT ZRtp {

    public:

    typedef enum _secrets {
        Rs1 = 1,
        Rs2 = 2,
        Pbx = 4,
        Aux = 8
    } secrets;

    typedef struct _zrtpInfo {
        uint32_t secretsCached;
        uint32_t secretsMatched;
        uint32_t secretsMatchedDH;
        const char *hash;
        const char *cipher;
        const char *pubKey;
        const char *sasType;
        const char *authLength;
    } zrtpInfo;

    /**
     * Faster access to Hello packets with different versions.
     */
    typedef struct HelloPacketVersion {
        int32_t version;
        ZrtpPacketHello* packet;
        uint8_t helloHash[IMPL_MAX_DIGEST_LENGTH];
    } HelloPacketVersion_t;

    /**
     * Constructor initializes all relevant data but does not start the
     * engine.
     *
     * @param id Client id, maximum length is 16 characters, will be truncated if it is too long
     * @param callback pointer to helper functions in filter/glue code
     * @param config pointer to algorithm configuration flags
     *
     * @deprecated Use new, streamlined constructor ZRtp(const std::string& , std::shared_ptr<ZrtpCallback>& , std::shared_ptr<ZrtpConfigure>& );
     */
    DEPRECATED_ZRTP ZRtp(uint8_t const * myZid, std::shared_ptr<ZrtpCallback>& callback, const std::string& id,
         std::shared_ptr<ZrtpConfigure>& config, bool mitm = false, bool sasSignSupport = false);

    /**
     * @brief Constructor initializes all relevant data but does not start the engine.
     *
     * @param id Client id, maximum length is 16 characters, will be truncated if it is too long
     * @param callback pointer to helper functions in filter/glue code
     * @param config pointer to algorithm configuration flags
     */
    ZRtp(const std::string& id, std::shared_ptr<ZrtpCallback>& callback, std::shared_ptr<ZrtpConfigure>& config);

    /**
     * Destructor cleans up.
     */
    ~ZRtp();

    /**
     * Kick off the ZRTP protocol engine.
     *
     * This method calls the ZrtpStateEngine#evInitial() state of the state
     * engine. After this call we are able to process ZRTP packets
     * from our peer and to process them.
     */
    void startZrtpEngine();

    /**
     * Stop ZRTP security.
     */
    void stopZrtp();

    /**
     * @brief Process ZRTP message.
     *
     * The method takes the data and forwards it to the ZRTP state engine for further
     * processing. It's the caller's duty to check the ZRTP CRC and the ZRTP magic
     * cookie before calling this function.
     *
     * @param zrtpMessage
     *    A pointer to the first byte of the ZRTP message. Refer to RFC6189.
     * @param peerSSRC
     *    The peer's SSRC.
     * @param length
     *     of the received data packet, this includes the length of the ZRTP CRC field and
     *     may include length of transport header, for example length of RTP header. Use
     *     setTransportOverhead(int32_t overhead) to set the length of the transport overhead.
     *
     * @sa  setTransportOverhead(int32_t)
     */
    void processZrtpMessage(uint8_t const * zrtpMessage, uint32_t peerSSRC, size_t length);

    /**
     * Process a timeout event.
     *
     * We got a timeout from the timeout provider. Forward it to the
     * protocol state engine.
     *
     */
    void processTimeout();
#if 0
    /**
     * Check for and handle GoClear ZRTP packet header.
     *
     * This method checks if this is a GoClear packet. If not, just return
     * false. Otherwise handle it according to the specification.
     *
     * @param extHeader
     *    A pointer to the first byte of the extension header. Refer to
     *    RFC3550.
     * @return
     *    False if not a GoClear, true otherwise.
     */
    bool handleGoClear(uint8_t *extHeader);
#endif
    /**
     * Set the auxiliary secret.
     *
     * Use this method to set the auxiliary secret data. Refer to ZRTP
     * specification, chapter 4.3 ff
     *
     * @param data
     *     Points to the secret data.
     * @param length
     *     Length of the auxiliary secret in bytes
     */
    void setAuxSecret(uint8_t* data, uint32_t length);

    /**
     * Check current state of the ZRTP state engine
     *
     * @param state
     *    The state to check.
     * @return
     *    Returns true id ZRTP engine is in the given state, false otherwise.
     */
    bool inState(int32_t state);

    /**
     * Set SAS as verified.
     *
     * Call this method if the user confirmed (verfied) the SAS. ZRTP
     * remembers this together with the retained secrets data.
     */
    void SASVerified();

    /**
     * Reset the SAS verfied flag for the current active user's retained secrets.
     *
     */
    void resetSASVerified();

     /**
      * Check if SAS verfied by both parties, valid after received Confirm1 or Confirm2.
      *
      */
     bool isSASVerified() { return zidRec->isSasVerified(); }

     /**
     * Get the ZRTP Hello Hash data.
     *
     * Use this method to get the ZRTP Hello hash data. The method
     * returns the data as a string containing the ZRTP protocol version and
     * hex-digits.
     * 
     * The index defines which Hello packet to use. Each supported ZRTP procol version
     * uses a different Hello packet and thus computes different hashes.
     *
     * Refer to ZRTP specification, chapter 8.
     * 
     * @param index
     *     Hello hash of the Hello packet identfied by index. Index must be 0 <= index < MAX_ZRTP_VERSIONS.
     *
     * @return
     *    a std::string formatted according to RFC6189 section 8 without the leading 'a=zrtp-hash:'
     *    SDP attribute identifier. The hello hash is available immediately after class instantiation.
     * 
     * @see getNumberSupportedVersions()
     */
    std::string getHelloHash(int index);

    /**
     * Get the peer's ZRTP Hello Hash data.
     *
     * Use this method to get the peer's ZRTP Hello Hash data. The method
     * returns the data as a string containing the ZRTP protocol version and
     * hex-digits.
     *
     * The peer's hello hash is available only after ZRTP received a hello. If
     * no data is available the function returns an empty string.
     *
     * Refer to ZRTP specification, chapter 8.
     *
     * @return
     *    a std:string containing the Hello version and the hello hash as hex digits.
     */
    std::string getPeerHelloHash();

    /**
     * Get Multi-stream parameters.
     *
     * Use this method to get the Multi-stream that were computed during
     * the ZRTP handshake. An application may use these parameters to
     * enable multi-stream processing for an associated SRTP session.
     *
     * Refer to chapter 4.4.2 in the ZRTP specification for further details
     * and restriction how and when to use multi-stream mode.
     *
     * @param zrtpMaster
     *     Where the function returns the pointer of the ZRTP master stream.
     * @return
     *    a string that contains the multi-stream parameters. The application
     *    must not modify the contents of this string, it is opaque data. The
     *    application may hand over this string to a new ZrtpQueue instance
     *    to enable multi-stream processing for this ZrtpQueue.
     *    If ZRTP was not started or ZRTP is not yet in secure state the method
     *    returns an empty string.
     */
    std::string getMultiStrParams(ZRtp **zrtpMaster);

    /**
     * Set Multi-stream parameters.
     *
     * Use this method to set the parameters required to enable Multi-stream
     * processing of ZRTP. The multi-stream parameters must be set before the
     * application starts the ZRTP protocol engine.
     *
     * Refer to chapter 4.4.2 in the ZRTP specification for further details
     * of multi-stream mode.
     *
     * @param parameters
     *     A string that contains the multi-stream parameters. See also
     *     <code>getMultiStrParams(ZRtp **zrtpMaster)</code>
     * @param zrtpMaster
     *     The pointer of the ZRTP master stream.
     */
    void setMultiStrParams(std::string parameters, ZRtp* zrtpMaster);

    /**
     * Check if this ZRTP session is a Multi-stream session.
     *
     * Use this method to check if this ZRTP instance uses multi-stream.
     * Refer to chapters 4.2 and 4.4.2 in the ZRTP.
     *
     * @return
     *     True if multi-stream is used, false otherwise.
     */
    [[nodiscard]] bool isMultiStream() const;

    /**
     * Check if the other ZRTP client supports Multi-stream.
     *
     * Use this method to check if the other ZRTP client supports
     * Multi-stream mode.
     *
     * @return
     *     True if multi-stream is available, false otherwise.
     */
    [[nodiscard]] bool isMultiStreamAvailable() const;

    /**
     * Accept a PBX enrollment request.
     *
     * If a PBX service asks to enroll the PBX trusted MitM key and the user
     * accepts this request, for example by pressing an OK button, the client
     * application shall call this method and set the parameter
     * <code>accepted</code> to true. If the user does not accept the request
     * set the parameter to false.
     *
     * @param accepted
     *     True if the enrollment request is accepted, false otherwise.
     */
    void acceptEnrollment(bool accepted);

    /**
     * Check the state of the enrollment mode.
     * 
     * If true then we will set the enrollment flag (E) in the confirm
     * packets and perform the enrollment actions. A MitM (PBX) enrollment service
     * started this ZRTP session. Can be set to true only if mitmMode is also true.
     * 
     * @return status of the enrollmentMode flag.
     */
    [[nodiscard]] bool isEnrollmentMode() const;

    /**
     * Set the state of the enrollment mode.
     * 
     * If true then we will set the enrollment flag (E) in the confirm
     * packets and perform the enrollment actions. A MitM (PBX) enrollment 
     * service must sets this mode to true. 
     * 
     * Can be set to true only if mitmMode is also true. 
     * 
     * @param enrollmentMode defines the new state of the enrollmentMode flag
     */
    void setEnrollmentMode(bool enrollmentMode);

    /**
     * Check if a peer's cache entry has a vaild MitM key.
     *
     * If true then the other peer ha a valid MtiM key, i.e. the peer has performed
     * the enrollment procedure. A PBX ZRTP Back-2-Back application can use this function
     * to check which of the peers is enrolled.
     *
     * @return True if the other peer has a valid Mitm key (is enrolled).
     */
    [[nodiscard]] bool isPeerEnrolled() const;

    /**
     * Send the SAS relay packet.
     * 
     * The method creates and sends a SAS relay packet according to the ZRTP
     * specifications. Usually only a MitM capable user agent (PBX) uses this
     * function.
     * 
     * @param sh the full SAS hash value, 32 bytes
     * @param render the SAS rendering algorithm
     */
    bool sendSASRelayPacket(uint8_t* sh, const std::string& render);

    /**
     * Get the committed SAS rendering algorithm for this ZRTP session.
     * 
     * @return the committed SAS rendering algorithm
     */
    [[nodiscard]] std::string getSasType() const { return sasType->getName(); }
 
    /**
     * Get the computed SAS hash for this ZRTP session.
     *
     * A PBX ZRTP back-to-Back function uses this function to get the SAS
     * hash of an enrolled client to construct the SAS relay packet for
     * the other client.
     *
     * @return a reference to the byte array that contains the full
     *         SAS hash.
     */
    [[nodiscard]] uint8_t const * getSasHash() const {return sasHash.data();}

    /**
     * @brief Get the short name of the confirmed public key algorithm.
     *
     * @return Short name of the public key algorithm
     */
    [[nodiscard]] std::string getPublicKeyAlgoName() { return pubKey->getName(); }

    /**
     * Set signature data.
     *
     * This functions stores signature data and transmits it during ZRTP
     * processing to the other party as part of the Confirm packets. Refer to
     * chapters 5.7 and 7.2.
     *
     * The signature data must be set before ZRTP the application calls
     * <code>start()</code>.
     *
     * @param data
     *    The signature data including the signature type block. The method
     *    copies this data into the Confirm packet at signature type block.
     * @param length
     *    The length of the signature data in bytes. This length must be
     *    multiple of 4.
     * @return
     *    True if the method stored the data, false otherwise.
     */
    bool setSignatureData(uint8_t* data, int32_t length);

    /**
     * Get signature data.
     *
     * This functions returns a pointer to the signature data that was received
     * during ZRTP processing. Refer to chapters 5.7 and 7.2.
     *
     * The returned pointer points to volatile data that is valid only during the
     * <code>checkSASSignature()</code> callback funtion. The application must copy
     * the signature data if it will be used after the callback function returns.
     *
     * The signature data can be retrieved after ZRTP enters secure state.
     * <code>start()</code>.
     *
     * @return
     *    Signature data.
     */
    [[nodiscard]] uint8_t const * getSignatureData() const { return signatureData; }

    /**
     * Get length of signature data in number of bytes.
     *
     * This functions returns the length of signature data that was receivied
     * during ZRTP processing. Refer to chapters 5.7 and 7.2.
     *
     * @return
     *    Length in bytes of the received signature data. The method returns
     *    zero if no signature data is available.
     */
    [[nodiscard]] int32_t getSignatureLength() const { return signatureLength * ZRTP_WORD_SIZE; }

    /**
     * Emulate a Conf2Ack packet.
     *
     * This method emulates a Conf2Ack packet. According to ZRTP specification
     * the first valid SRTP packet that the Initiator receives must switch
     * on secure mode. Refer to chapter 4 in the specificaton
     *
     */
    void conf2AckSecure();

     /**
      * Get other party's ZID (ZRTP Identifier) data
      *
      * This functions returns the other party's ZID that was receivied
      * during ZRTP processing.
      *
      * The ZID data can be retrieved after ZRTP receive the first Hello
      * packet from the other party. The application may call this method
      * for example during SAS processing in showSAS(...) user callback
      * method.
      *
      * @param data
      *    Pointer to a data buffer. This buffer must have a size of
      *    at least 12 bytes (96 bit) (ZRTP Identifier, see chap. 4.9)
      * @return
      *    Number of bytes copied into the data buffer - must be equivalent
      *    to 96 bit, usually 12 bytes.
      */
     int32_t getPeerZid(uint8_t* data) const {
         memcpy(data, peerZid.data(), IDENTIFIER_LEN);
         return IDENTIFIER_LEN;
     }

     /**
      * Return gathered detailed information structure.
      *
      * This structure contains some detailed information about the negotiated
      * algorithms, the cached and matched shared secrets.
      */
     [[nodiscard]] zrtpInfo const & getDetailInfo() const { return detailInfo; }

     /**
      * Get peer's client id.
      *
      * @return the peer's client id or an empty @c string if not set.
      */
     [[nodiscard]] std::string const & getPeerClientId() const {return peerClientId;};

     /**
      * Get peer's protocol version string.
      *
      * @return the peer's protocol version or an empty @c string if not set.
      */
     [[nodiscard]] std::string getPeerProtocolVersion() const {
         return (peerHelloVersion[0] == 0) ? std::string() : std::string((char*)peerHelloVersion);
     };

     /**
      * Get number of supported ZRTP protocol versions.
      *
      * @return the number of supported ZRTP protocol versions.
      */
     static int32_t getNumberSupportedVersions() {return SUPPORTED_ZRTP_VERSIONS;}

     /**
      * Get negotiated ZRTP protocol version.
      *
      * @return the integer representation of the negotiated ZRTP protocol version.
      */
     int32_t getCurrentProtocolVersion() {return currentHelloPacket->getVersionInt();}

     /**
      * Validate the RS2 data if necessary.
      *
      * The cache functions stores the RS2 data but does not set its valid flag. The
      * application may decide to set this flag.
      */
     void setRs2Valid();

     /**
      * Get the secure since field
      * 
      * Returns the secure since field or 0 if no such field is available. Secure since
      * uses the unixepoch.
      */
     [[nodiscard]] int64_t getSecureSince() const {
         return (zidRec != nullptr) ? zidRec->getSecureSince() :  0;
     }

     /**
      * Set the resend counter of timer T1 - T1 controls the Hello packets.
      * 
      * This overwrites the standard value of 20 retries. Setting to <0 means
      * 'indefinite', counter values less then 10 are ignored.
      * 
      * Applications may set the resend counter based on network  or some other 
      * conditions. Applications may set this value any time and it's in effect
      * for the current call. Setting the counter after the hello phase has no
      * effect.
      */
     void setT1Resend(int32_t counter);

     /**
      * Set the extended resend counter of timer T1 - T1 controls the Hello packets.
      *
      * More retries to extend time, see RFC6189 chap. 6. This overwrites the standard 
      * value of 60 extended retiries.
      * 
      * Applications may set the resend counter based on network  or some other 
      * conditions. 
      */
     void setT1ResendExtend(int32_t counter);

     /**
      * Set the time capping of timer T1 - T1 controls the Hello packets.
      * 
      * Values <50ms are not set.
      */
     void setT1Capping(int32_t capping);

     /**
      * Set the resend counter of timer T2 - T2 controls other (post-Hello) packets.
      * 
      * This overwrites the standard value of 10 retiries. Setting to <0 means
      * 'indefinite', counter values less then 10 are ignored.
      * 
      * Applications may set the resend counter based on network  or some other 
      * conditions. Applications may set this value any time and it's in effect
      * for the current call. Setting the counter after tZRTP enetered secure state
      * has no effect.
      */
     void setT2Resend(int32_t counter);

     /**
      * Set the time capping of timer T2 - T2 controls other (post-Hello) packets.
      * 
      * Values <150ms are not set.
      */
     void setT2Capping(int32_t capping);

     /**
      * @brief Get required buffer size to get all 32-bit statistic counters of ZRTP
      *
      * @return number of 32 bit integer elements required or < 0 on error
      */
     int getNumberOfCountersZrtp();

     /**
      * @brief Read statistic counters of ZRTP
      * 
      * @param counters Pointer to buffer of 32-bit integers. The buffer must be able to
      *         hold at least getNumberOfCountersZrtp() 32-bit integers
      * @return number of 32-bit counters returned in buffer or < 0 on error
      */
     int getCountersZrtp(int32_t* counters);
     
     /**
      * @brief Get the computed ZRTP exported key.
      * 
      * Returns the computed exported key. The application should copy
      * the data it needs.
      * 
      * @param length pointer to an int, gets the length of the exported key.
      * @return pointer to the exported key data.
      */
     const uint8_t& getExportedKey(uint32_t *length) const {
         if (length != nullptr)
             *length = hashLength;
         return *zrtpExport.data();
     };

     /**
      * @brief Return either Initiator or Responder.
      */
     [[nodiscard]] int32_t getZrtpRole() const { return myRole; }

     /**
      * @brief Get status of our peer's disclosure flag
      */
     [[nodiscard]] bool isPeerDisclosureFlag() const { return peerDisclosureFlagSeen; }

    /**
     * @brief Get the ZID cache instance of this ZRTP connection.
     */
     [[nodiscard]] std::shared_ptr<ZIDCache>& getZidCache() const { return configureAlgos->getZidCache(); }

    /**
     * @brief Get the configuration data of this ZRTP connection.
     */
     [[nodiscard]] std::shared_ptr<ZrtpConfigure> getZrtpConfigure() const { return configureAlgos; }

    /**
     * @brief Set the length of the transport protocol overhead in bytes.
     *
     * ZRTP uses this to check consitency of input data. For example the transport protocol
     * overhead of an RTP packet that contains ZRTP data is the fixed length 12.
     *
     * @param overhead Length of the transport overhead.
     */
    void setTransportOverhead(int32_t overhead);

#ifndef UNIT_TESTS
private:
#endif
    /**
     * @brief Sent the ZRTP message as ZRTP frame(s)
     *
     * @param packet
     *     points to the ZRTP message/packet to wrap and send in ZRTP frame(s)
     * @return
     *     zero if sending failed, one if packet was send
     */
    int32_t sendAsZrtpFrames(ZrtpPacketBase *packet);

    /**
     * @brief Sent the ZRTP message as ZRTP multi-frames
     *
     * @param packets
     *    list of ZRTP message/packet pointers to wrap and send in ZRTP frame(s)
     * @return
     *     zero if sending failed, one if packet was send
     */
    int32_t sendAsZrtpMultiFrames(std::unique_ptr<std::list<std::reference_wrapper<ZrtpPacketBase>>> packets);

    /**
     * @brief Process ZRTP frame packet.
     *
     * The method takes the data and unpacks the ZRTP messages in case of a multi-frame packet
     * or assembles a ZRTP message in case of a fragmented ZRTP message.
     * After processing the data it forwards the resulting ZRTP message(s) for further
     * processing. It's the caller's duty to check the ZRTP CRC and the ZRTP magic
     * cookie before calling this function.
     *
     * @param zrtpMessage
     *    A pointer to the first byte of the ZRTP message. Refer to RFC6189.
     * @param peerSSRC
     *    The peer's SSRC.
     * @param length
     *     of the received data packet, this includes the length of the ZRTP CRC field and
     *     may include length of transport header, for example length of RTP header. Use
     *     setTransportOverhead(int32_t overhead) to set the length of the transport overhead.
     * @param frameByte
     *     The second byte of the ZRTP/RTP header. If bit 1 is set then it's a ZRTP frame packet.
     *
     * @sa  setTransportOverhead(int32_t)
     */
    void processZrtpFramePacket(uint8_t const * zrtpMessage, uint32_t peerSSRC, size_t length, uint8_t frameByte);

private:
    friend ZrtpStateEngineImpl;

    /**
     * The state engine takes care of protocol processing.
     */
    std::unique_ptr<ZrtpStateEngine> stateEngine;

    /**
     * This is my ZID that I send to the peer.
     */
    secUtilities::SecureArray<IDENTIFIER_LEN> ownZid;

    /**
     * The peer's ZID
     */
    secUtilities::SecureArray<IDENTIFIER_LEN> peerZid;

    /**
     * The callback class provides me with the interface to send
     * data and to deal with timer management of the hosting system.
     */
    std::weak_ptr<ZrtpCallback> callback;

    /**
     * My active Diffie-Hellman context
     */
    std::unique_ptr<ZrtpDH> dhContext;

    /**
     * The computed DH shared secret
     */
    zrtp::SecureArray1k DHss;

    /**
     * My computed public key
     */
    zrtp::SecureArray1k pubKeyBytes;

    /**
     * My Role in the game
     */
    Role myRole = NoRole;

    /**
     * The human readable SAS value
     */
    std::string SAS;

    /**
     * The SAS hash for signaling and alike. Refer to chapters
     * 4.5 and 7 how sasHash, sasValue and the SAS string are derived.
     */
    zrtp::NegotiatedArray  sasHash;
    /**
     * The ids for the retained and other shared secrets
     */
    zrtp::RetainedSecArray rs1IDr;
    zrtp::RetainedSecArray rs2IDr;
    zrtp::RetainedSecArray auxSecretIDr;
    zrtp::RetainedSecArray pbxSecretIDr;

    zrtp::RetainedSecArray rs1IDi;
    zrtp::RetainedSecArray rs2IDi;
    zrtp::RetainedSecArray auxSecretIDi;
    zrtp::RetainedSecArray pbxSecretIDi;

    /**
     * pointers to aux secret storage and length of aux secret
     */
    std::unique_ptr<uint8_t[]> auxSecret;
    uint32_t auxSecretLength = 0;

    /**
     * Record if valid rs1 and/or rs1 were found in the
     * retained secret cache.
     */
    bool rs1Valid = false;
    bool rs2Valid = false;
    /**
     * My hvi
     */
    uint8_t hvi[MAX_DIGEST_LENGTH]  = {0};

    /**
     * The peer's hvi
     */
    uint8_t peerHvi[8*ZRTP_WORD_SIZE]  = {0};

    /**
     * Context to compute the SHA256 hash of selected messages.
     * Used to compute the s0, refer to chapter 4.4.1.4
     */
    void* msgShaContext = nullptr;
    /**
     * Committed Hash, Cipher, and public key algorithms
     */
    AlgorithmEnum* hash = nullptr;
    AlgorithmEnum* cipher = nullptr;
    AlgorithmEnum* pubKey = nullptr;
    /**
     * The selected SAS type.
     */
    AlgorithmEnum* sasType = nullptr;

    /**
     * The selected SAS type.
     */
    AlgorithmEnum* authLength = nullptr;

    /**
     * The Hash images as defined in chapter 5.1.1 (H0 is a random value,
     * not stored here). Need full SHA 256 length to store hash value but
     * only the leftmost 128 bits are used in computations and comparisons.
     */
    uint8_t H0[IMPL_MAX_DIGEST_LENGTH] = {0};
    uint8_t H1[IMPL_MAX_DIGEST_LENGTH] = {0};
    uint8_t H2[IMPL_MAX_DIGEST_LENGTH] = {0};
    uint8_t H3[IMPL_MAX_DIGEST_LENGTH] = {0};

    uint8_t peerHelloHash[IMPL_MAX_DIGEST_LENGTH] = {0};
    uint8_t peerHelloVersion[ZRTP_WORD_SIZE + 1] = {0};   // +1 for nul byte

    // We get the peer's H? from the message where length is defined as 8 words
    uint8_t peerH2[8*ZRTP_WORD_SIZE] = {0};
    uint8_t peerH3[8*ZRTP_WORD_SIZE] = {0};

    /**
     * The hash over selected messages, use negotiated hash function
     */
    zrtp::NegotiatedArray messageHash;

    /**
     * The s0
     */
    zrtp::NegotiatedArray s0;

    /**
     * The new Retained Secret
     */
    zrtp::NegotiatedArray  newRs1;

    /**
     * The confirm HMAC key
     */
    zrtp::NegotiatedArray  hmacKeyI;
    zrtp::NegotiatedArray  hmacKeyR;

    /**
     * The Initiator's srtp key and salt
     */
    zrtp::NegotiatedArray  srtpKeyI;
    zrtp::NegotiatedArray  srtpSaltI;

    /**
     * The Responder's srtp key and salt
     */
    zrtp::NegotiatedArray  srtpKeyR;
    zrtp::NegotiatedArray  srtpSaltR;

    /**
     * The keys used to encrypt/decrypt the confirm message
     */
    zrtp::NegotiatedArray  zrtpKeyI;
    zrtp::NegotiatedArray  zrtpKeyR;

    /**
     * Pointers to negotiated hash and HMAC functions
     */
    void (*hashListFunction)(const std::vector<const uint8_t*>& data,
                             const std::vector<uint64_t>& dataLength,
                             uint8_t *digest) = nullptr;

    void (*hmacFunction)(const uint8_t* key, uint64_t key_length,
                         const uint8_t* data, uint64_t data_length,
                         zrtp::RetainedSecArray & macOut) = nullptr;

    void (*hmacListFunction)(const uint8_t* key, uint64_t key_length,
                             const std::vector<const uint8_t*>& data,
                             const std::vector<uint64_t>& data_length,
                             zrtp::RetainedSecArray & macOut) = nullptr;

    void* (*createHashCtx)() = nullptr;

    void (*closeHashCtx)(void* ctx, zrtp::RetainedSecArray & macOut) = nullptr;

    void (*hashCtxFunction)(void* ctx, const uint8_t* data, uint64_t dataLength) = nullptr;

    uint32_t hashLength = 0;

    // Function pointers to implicit hash and hmac functions
    void (*hashFunctionImpl)(const uint8_t *data,
                             uint64_t data_length,
                             uint8_t *digest) = nullptr;

    void (*hmacFunctionImpl)(const uint8_t* key, uint64_t key_length,
                             const uint8_t* data, uint64_t data_length,
                             zrtp::RetainedSecArray &) = nullptr;

    int32_t hashLengthImpl = 0;

    /**
     * The ZRTP Session Key
     * Refer to chapter 4.5.2
     */
    zrtp::NegotiatedArray  zrtpSession;

    /**
     * The ZRTP export Key
     * Refer to chapter 4.5.2
     */
    zrtp::NegotiatedArray  zrtpExport;

    /**
     * True if this ZRTP instance uses multi-stream mode.
     */
    bool multiStream = false;

        /**
     * True if the other ZRTP client supports multi-stream mode.
     */
    bool multiStreamAvailable = false;

    /**
     * Enable MitM (PBX) enrollment
     * 
     * If set to true then ZRTP honors the PBX enrollment flag in
     * Commit packets and calls the appropriate user callback
     * methods. If the parameter is set to false ZRTP ignores the PBX
     * enrollment flags.
     */
    bool enableMitmEnrollment = false;

    /**
     * True if a valid trusted MitM key of the other peer is available, i.e. enrolled.
     */
    bool peerIsEnrolled = false;

    /**
     * Set to true if the Hello packet contained the M-flag (MitM flag).
     * We use this later to check some stuff for SAS Relay processing
     */
    bool mitmSeen = false;

    /**
     * Temporarily store computed pbxSecret, if user accepts enrollment then
     * it will copied to our ZID record of the PBX (MitM)  
     */
    uint8_t* pbxSecretTmp = nullptr;
    uint8_t  pbxSecretTmpBuffer[MAX_DIGEST_LENGTH] = {0};

    /**
     * If true then we will set the enrollment flag (E) in the confirm
     * packets. Set to true if the PBX enrollment service started this ZRTP 
     * session. Can be set to true only if mitmMode is also true. 
     */
    bool enrollmentMode = false;

    /**
     * Configuration data which algorithms to use.
     */
    std::shared_ptr<ZrtpConfigure> configureAlgos;

    /**
     * Pre-initialized packets.
     */
    ZrtpPacketHello    zrtpHello_11;
    ZrtpPacketHello    zrtpHello_12;   // Prepare for ZRTP protocol version 1.2

    ZrtpPacketHelloAck zrtpHelloAck;
    ZrtpPacketConf2Ack zrtpConf2Ack;
//    ZrtpPacketClearAck zrtpClearAck;
//    ZrtpPacketGoClear  zrtpGoClear;
    ZrtpPacketError    zrtpError;
    ZrtpPacketErrorAck zrtpErrorAck;
    ZrtpPacketDHPart   zrtpDH1;
    ZrtpPacketDHPart   zrtpDH2;
    ZrtpPacketCommit   zrtpCommit;
    ZrtpPacketConfirm  zrtpConfirm1;
    ZrtpPacketConfirm  zrtpConfirm2;
    ZrtpPacketPingAck  zrtpPingAck;
    ZrtpPacketSASrelay zrtpSasRelay;
    ZrtpPacketRelayAck zrtpRelayAck;

    HelloPacketVersion_t helloPackets[MAX_ZRTP_VERSIONS + 1] = {};

    /// Pointer to Hello packet sent to partner, initialized in ZRtp, modified by ZrtpStateEngineImpl
    ZrtpPacketHello* currentHelloPacket = nullptr;

    /**
     * ZID cache record
     */
    std::unique_ptr<ZIDRecord> zidRec;

    /**
     * Save record
     * 
     * If false don't save record until user verified and confirmed the SAS after a cache mismatch.
     * See RFC6189, sections 4.6.1 and 4.6.1.1 and explanation.
     */
    bool saveZidRecord = true;
    /**
     * Random IV data to encrypt the confirm data, 128 bit for AES
     */
    uint8_t randomIV[16] = {0};

    uint8_t tempMsgBuffer[1024] = {0};
    uint32_t lengthOfMsgData = 0;

    /**
     * Variables to store signature data. Includes the signature type block
     */
    const uint8_t* signatureData = nullptr;       // will be set when needed
    int32_t  signatureLength = 0;     // overall length in bytes

    /**
     * Is true if the other peer signaled SAS signature support in its Hello packet.
     */
    bool signSasSeen = false;

    uint32_t peerSSRC = 0;           // peer's SSRC, required to set up PingAck packet

    zrtpInfo detailInfo = {};         // filled with some more detailed information if application would like to know

    std::string peerClientId;    // store the peer's client Id

    ZRtp* masterStream = nullptr;          // This is the master stream in case this is a multi-stream
    std::vector<std::string> peerNonces;   // Store nonces we got from our partner. Using std::string
                                           // just simplifies memory management, nonces are binary data, not strings :-)
    /**
     * Enable or disable paranoid mode.
     *
     * The Paranoid mode controls the behaviour and handling of the SAS verify flag. If
     * Paranoid mode is set to false then ZRtp applies the normal handling. If Paranoid
     * mode is set to true then the handling is:
     *
     * <ul>
     * <li> Force the SAS verify flag to be false at srtpSecretsOn() callback. This gives
     *      the user interface (UI) the indication to handle the SAS as <b>not verified</b>.
     *      See implementation note below.</li>
     * <li> Don't set the SAS verify flag in the <code>Confirm</code> packets, thus the other
     *      also must report the SAS as <b>not verified</b>.</li>
     * <li> ignore the <code>SASVerified()</code> function, thus do not set the SAS to verified
     *      in the ZRTP cache. </li>
     * <li> Disable the <b>Trusted PBX MitM</b> feature. Just send the <code>SASRelay</code> packet
     *      but do not process the relayed data. This protects the user from a malicious
     *      "trusted PBX".</li>
     * </ul>
     * ZRtp performs calls other steps during the ZRTP negotiations as usual, in particular it
     * computes, compares, uses, and stores the retained secrets. This avoids unnecessary warning
     * messages. The user may enable or disable the Paranoid mode on a call-by-call basis without
     * breaking the key continuity data.
     *
     * <b>Implementation note:</b></br>
     * An application shall always display the SAS code if the SAS verify flag is <code>false</code>.
     * The application shall also use mechanisms to remind the user to compare the SAS code, for
     * example using larger fonts, different colours and other display features.
     */
    bool paranoidMode = false;

    /**
     * Is true if the other peer sent a Disclosure flag in its Confirm packet.
     */
    bool peerDisclosureFlagSeen = false;

    /**
     * If true then use ZRTP frames according to ZRTP 2022 spec.
     */
    bool isZrtpFrames = false;

    /**
     * Last packet sent using ZRTP frame(s)
     */
    ZrtpPacketBase *sentFramePacket = nullptr;

    /**
     * Frame batch
     */
     uint8_t frameBatch = 0;

    /**
     * @brief Initialize ZRTP data, packets etc
     * @param id the client id to use in Hello packet
     */
    void initialize(const std::string& id);

    /**
     * Find the best Hash algorithm that is offered in Hello.
     *
     * Find the best, that is the strongest, Hash algorithm that our peer
     * offers in its Hello packet.
     *
     * @param hello
     *    The Hello packet.
     * @return
     *    The Enum that identifies the best offered Hash algorithm. Return
     *    mandatory algorithm if no match was found.
     */
    AlgorithmEnum* findBestHash(ZrtpPacketHello *hello);

    /**
     * Find the best symmetric cipher algorithm that is offered in Hello.
     *
     * Find the best, that is the strongest, cipher algorithm that our peer
     * offers in its Hello packet.
     *
     * @param hello
     *    The Hello packet.
     * @param pk
     *    The id of the selected public key algorithm
     * @return
     *    The Enum that identifies the best offered Cipher algorithm. Return
     *    mandatory algorithm if no match was found.
     */
    AlgorithmEnum* findBestCipher(ZrtpPacketHello *hello,  AlgorithmEnum* pk);

    /**
     * Find the best Public Key algorithm that is offered in Hello.
     *
     * Find the best, that is the strongest, public key algorithm that our peer
     * offers in its Hello packet.
     *
     * @param hello
     *    The Hello packet.
     * @return
     *    The Enum that identifies the best offered Public Key algorithm. Return
     *    mandatory algorithm if no match was found.
     */
    AlgorithmEnum* findBestPubkey(ZrtpPacketHello *hello);

    /**
     * Find the best SAS algorithm that is offered in Hello.
     *
     * Find the best, that is the strongest, SAS algorithm that our peer
     * offers in its Hello packet. The method works as defined in RFC 6189,
     * chapter 4.1.2.
     *
     * The list of own supported public key algorithms must follow the rules
     * defined in RFC 6189, chapter 4.1.2, thus the order in the list must go
     * from fastest to slowest.
     *
     * @param hello
     *    The Hello packet.
     * @return
     *    The Enum that identifies the best offered SAS algorithm. Return
     *    mandatory algorithm if no match was found.
     */
    AlgorithmEnum* findBestSASType(ZrtpPacketHello* hello);

    /**
     * Find the best authentication length that is offered in Hello.
     *
     * Find the best, that is the strongest, authentication length that our peer
     * offers in its Hello packet.
     *
     * @param hello
     *    The Hello packet.
     * @return
     *    The Enum that identifies the best offered authentication length. Return
     *    mandatory algorithm if no match was found.
     */
    AlgorithmEnum* findBestAuthLen(ZrtpPacketHello* hello);

    /**
     * Check if MultiStream mode is offered in Hello.
     *
     * Find the best, that is the strongest, authentication length that our peer
     * offers in its Hello packet.
     *
     * @param hello
     *    The Hello packet.
     * @return
     *    True if multi stream mode is available, false otherwise.
     */
    static bool checkMultiStream(ZrtpPacketHello* hello);

    /**
     * Checks if Hello packet contains a strong (384bit) hash based on selection policy.
     * 
     * The function currently implements the non-NIST policy only:
     * If the public key algorithm is a non-NIST ECC algorithm this function prefers
     * non-NIST HASH algorithms (Skein etc).
     * 
     * If Hello packet does not contain a strong hash then this functions returns @c NULL.
     *
     * @param hello The Hello packet.
     * @param algoName name of selected PK algorithm
     * @return @c hash algorithm if found in Hello packet, @c NULL otherwise.
     */
    AlgorithmEnum* getStrongHashOffered(ZrtpPacketHello *hello, int32_t algoName);

    /**
     * Checks if Hello packet offers a strong (256bit) symmetric cipher based on selection policy.
     *
     * The function currently implements the nonNist policy only:
     * If the public key algorithm is a non-NIST ECC algorithm this function prefers
     * non-NIST symmetric cipher algorithms (Twofish etc).
     *
     * If Hello packet does not contain a symmetric cipher then this functions returns @c NULL.

     * @param hello The Hello packet.
     * @param algoName name of selected PK algorithm
     * @return @c hash algorithm if found in Hello packet, @c NULL otherwise.
     *
     * @return @c cipher algorithm if found in Hello packet, @c NULL otherwise.
     */
    AlgorithmEnum* getStrongCipherOffered(ZrtpPacketHello *hello, int32_t algoName);

    /**
     * Checks if Hello packet contains a hash based on selection policy.
     *
     * The function currently implements the nonNist policy only:
     * If the public key algorithm is a non-NIST ECC algorithm this function prefers
     * non-NIST HASH algorithms (Skein etc).
     *
     * @param hello The Hello packet.
     * @param algoName name of selected PK algorithm
     * @return @c hash algorithm found in Hello packet.
     */
    AlgorithmEnum* getHashOffered(ZrtpPacketHello *hello, int32_t algoName);

    /**
     * Checks if Hello packet offers a symmetric cipher based on selection policy.
     *
     * The function currently implements the nonNist policy only:
     * If the public key algorithm is a non-NIST ECC algorithm this function prefers
     * non-NIST symmetric cipher algorithms (Twofish etc).
     *
     * @param hello The Hello packet.
     * @param algoName name of selected PK algorithm
     * @return non-NIST @c cipher algorithm if found in Hello packet, @c NULL otherwise
     */
    AlgorithmEnum* getCipherOffered(ZrtpPacketHello *hello, int32_t algoName);

    /**
     * Checks if Hello packet offers a SRTP authentication length based on selection policy.
     *
     * The function currently implements the nonNist policy only:
     * If the public key algorithm is a non-NIST ECC algorithm this function prefers
     * non-NIST algorithms (Skein etc).
     *
     * @param hello The Hello packet.
     * @param algoName algoName name of selected PK algorithm
     * @return @c authLen algorithm found in Hello packet
     */
    AlgorithmEnum* getAuthLenOffered(ZrtpPacketHello *hello, int32_t algoName);

    /**
     * Compute my hvi value according to ZRTP specification.
     */
    void computeHvi(ZrtpPacketDHPart* dh, ZrtpPacketHello *hello);

    void computeSharedSecretSet(ZIDRecord& zidRecord);

    void computeAuxSecretIds();

    void computeSRTPKeys();

    void KDF(uint8_t* key, size_t keyLength, char const * label, size_t labelLength,
               uint8_t* context, size_t contextLength, size_t L, zrtp::NegotiatedArray & output);

    void generateKeysInitiator(ZrtpPacketDHPart *dhPart, ZIDRecord& zidRecord);

    void generateKeysResponder(ZrtpPacketDHPart *dhPart, ZIDRecord& zidRecord);

    void generateKeysMultiStream();

    void computePBXSecret();

    void setNegotiatedHash(AlgorithmEnum* hash);

    /*
     * The following methods are helper functions for ZrtpStateEngineImpl.
     * ZrtpStateEngineImpl calls them to prepare packets, send data, report
     * problems, etc.
     */
    /**
     * Send a ZRTP packet.
     *
     * The state engines calls this method to send a packet via the RTP
     * stack.
     *
     * @param packet
     *    Points to the ZRTP packet.
     * @return
     *    zero if sending failed, one if packet was send
     */
    int32_t sendPacketZRTP(ZrtpPacketBase *packet);

    /**
     * Activate a Timer using the host callback.
     *
     * @param tm
     *    The time in milliseconds.
     * @return
     *    zero if activation failed, one if timer was activated
     */
    int32_t activateTimer(int32_t tm);

    /**
     * Cancel the active Timer using the host callback.
     *
     * @return
     *    zero if activation failed, one if timer was activated
     */
    int32_t cancelTimer();

    /**
     * Prepare a Hello packet.
     *
     * Just take the pre-initialized Hello packet and return it. No
     * further processing required.
     *
     * @return
     *    A pointer to the initialized Hello packet.
     */
    ZrtpPacketHello* prepareHello();

    /**
     * Prepare a HelloAck packet.
     *
     * Just take the preinitialized HelloAck packet and return it. No
     * further processing required.
     *
     * @return
     *    A pointer to the initialized HelloAck packet.
     */
    ZrtpPacketHelloAck* prepareHelloAck();

    /**
     * Prepare a Commit packet.
     *
     * We have received a Hello packet from our peer. Check the offers
     * it makes to us and select the most appropriate. Using the
     * selected values prepare a Commit packet and return it to protocol
     * state engine.
     *
     * @param hello
     *    Points to the received Hello packet
     * @param errMsg
     *    Points to an integer that can hold a ZRTP error code.
     * @return
     *    A pointer to the prepared Commit packet
     */
    ZrtpPacketCommit* prepareCommit(ZrtpPacketHello *hello, uint32_t* errMsg);

    /**
     * Prepare a Commit packet for Multi Stream mode.
     *
     * Using the selected values prepare a Commit packet and return it to protocol
     * state engine.
     *
     * @param hello
     *    Points to the received Hello packet
     * @return
     *    A pointer to the prepared Commit packet for multi stream mode
     */
    ZrtpPacketCommit* prepareCommitMultiStream(ZrtpPacketHello *hello);

    /**
     * Prepare the DHPart1 packet.
     *
     * This method prepares a DHPart1 packet. The input to the method is always
     * a Commit packet received from the peer. Also we a in the role of the
     * Responder.
     *
     * When we receive a Commit packet we get the selected ciphers, hashes, etc
     * and cross-check if this is ok. Then we need to initialize a set of DH
     * keys according to the selected cipher. Using this data we prepare our DHPart1
     * packet.
     */
    ZrtpPacketDHPart* prepareDHPart1(ZrtpPacketCommit *commit, uint32_t* errMsg);

    /**
     * Prepare the DHPart2 packet.
     *
     * This method prepares a DHPart2 packet. The input to the method is always
     * a DHPart1 packet received from the peer. Our peer sends the DH1Part as
     * response to our Commit packet. Thus we are in the role of the
     * Initiator.
     *
     */
    ZrtpPacketDHPart* prepareDHPart2(ZrtpPacketDHPart* dhPart1, uint32_t* errMsg);

    /**
     * Prepare the Confirm1 packet.
     *
     * This method prepare the Confirm1 packet. The input to this method is the
     * DHPart2 packet received from our peer. The peer sends the DHPart2 packet
     * as response of our DHPart1. Here we are in the role of the Responder
     *
     */
    ZrtpPacketConfirm* prepareConfirm1(ZrtpPacketDHPart* dhPart2, uint32_t* errMsg);

    /**
     * Prepare the Confirm1 packet in multi stream mode.
     *
     * This method prepares the Confirm1 packet. The state engine call this method
     * if multi stream mode is selected and a Commit packet was received. The input to
     * this method is the Commit.
     * Here we are in the role of the Responder
     *
     */
    ZrtpPacketConfirm* prepareConfirm1MultiStream(ZrtpPacketCommit* commit, uint32_t* errMsg);

    /**
     * Prepare the Confirm2 packet.
     *
     * This method prepare the Confirm2 packet. The input to this method is the
     * Confirm1 packet received from our peer. The peer sends the Confirm1 packet
     * as response of our DHPart2. Here we are in the role of the Initiator
     */
    ZrtpPacketConfirm* prepareConfirm2(ZrtpPacketConfirm* confirm1, uint32_t* errMsg);

    /**
     * Prepare the Confirm2 packet in multi stream mode.
     *
     * This method prepares the Confirm2 packet. The state engine call this method if
     * multi stream mode is active and in state CommitSent. The input to this method is
     * the Confirm1 packet received from our peer. The peer sends the Confirm1 packet
     * as response of our Commit packet in multi stream mode.
     * Here we are in the role of the Initiator
     */
    ZrtpPacketConfirm* prepareConfirm2MultiStream(ZrtpPacketConfirm* confirm1, uint32_t* errMsg);

    /**
     * Prepare the Conf2Ack packet.
     *
     * This method prepare the Conf2Ack packet. The input to this method is the
     * Confirm2 packet received from our peer. The peer sends the Confirm2 packet
     * as response of our Confirm1. Here we are in the role of the Initiator
     */
    ZrtpPacketConf2Ack* prepareConf2Ack(ZrtpPacketConfirm* confirm2, uint32_t* errMsg);

    /**
     * Prepare the ErrorAck packet.
     *
     * This method prepares the ErrorAck packet. The input to this method is the
     * Error packet received from the peer.
     */
    ZrtpPacketErrorAck* prepareErrorAck(ZrtpPacketError* epkt);

    /**
     * Prepare the Error packet.
     *
     * This method prepares the Error packet. The input to this method is the
     * error code to be included into the message.
     */
    ZrtpPacketError* prepareError(uint32_t errMsg);

#if 0
    /**
     * Prepare a ClearAck packet.
     *
     * This method checks if the GoClear message is valid. If yes then switch
     * off SRTP processing, stop sending of RTP packets (pause transmit) and
     * inform the user about the fact. Only if user confirms the GoClear message
     * normal RTP processing is resumed.
     *
     * @return
     *     NULL if GoClear could not be authenticated, a ClearAck packet
     *     otherwise.
     */
    ZrtpPacketClearAck* prepareClearAck(ZrtpPacketGoClear* gpkt);
#endif
    /**
     * Prepare the PingAck packet.
     *
     * This method prepares the PingAck packet. The input to this method is the
     * Ping packet received from the peer.
     */
    ZrtpPacketPingAck* preparePingAck(ZrtpPacketPing* ppkt);

    /**
     * Prepare the RelayAck packet.
     *
     * This method prepares the RelayAck packet. The input to this method is the
     * SASrelay packet received from the peer.
     */
    ZrtpPacketRelayAck* prepareRelayAck(ZrtpPacketSASrelay* srly, const uint32_t* errMsg);
#if 0
    /**
     * Prepare a GoClearAck packet w/o HMAC
     *
     * Prepare a GoCLear packet without a HMAC but with a short error message.
     * This type of GoClear is used if something went wrong during the ZRTP
     * negotiation phase.
     *
     * @return
     *     A goClear packet without HMAC
     */
    ZrtpPacketGoClear* prepareGoClear(uint32_t errMsg = 0);
#endif
    /**
     * Compare the hvi values.
     *
     * Compare a received Commit packet with our Commit packet and returns
     * which Commit packt is "more important". See chapter 5.2 to get further
     * information how to compare Commit packets.
     *
     * @param commit
     *    Pointer to the peer's commit packet we just received.
     * @return
     *    <0 if our Commit packet is "less important"
     *    >0 if our Commit is "more important"
     *     0 shouldn't happen because we compare crypto hashes
     */
    int32_t compareCommit(ZrtpPacketCommit *commit);

    /**
     * Verify the H2 hash image.
     *
     * Verifies the H2 hash contained in a received commit message.
     * This functions just verifies H2 but does not store it.
     *
     * @param commit
     *    Pointer to the peer's commit packet we just received.
     * @return
     *    true if H2 is ok and verified
     *    false if H2 could not be verified
     */
    bool verifyH2(ZrtpPacketCommit *commit);

    /**
     * Send information messages to the hosting environment.
     *
     * The ZRTP implementation uses this method to send information messages
     * to the host. Along with the message ZRTP provides a severity indicator
     * that defines: Info, Warning, Error, Alert. Refer to the MessageSeverity
     * enum in the ZrtpCallback class.
     *
     * @param severity
     *     This defines the message's severity
     * @param subCode
     *     The subcode identifying the reason.
     * @see ZrtpCodes#MessageSeverity
     */
    void sendInfo(GnuZrtpCodes::MessageSeverity severity, int32_t subCode);

    /**
     * ZRTP state engine calls this if the negotiation failed.
     *
     * ZRTP calls this method in case ZRTP negotiation failed. The parameters
     * show the severity as well as some explanatory text.
     *
     * @param severity
     *     This defines the message's severity
     * @param subCode
     *     The subcode identifying the reason.
     * @see ZrtpCodes#MessageSeverity
     */
    void zrtpNegotiationFailed(GnuZrtpCodes::MessageSeverity severity, int32_t subCode);

    /**
     * ZRTP state engine calls this method if the other side does not support ZRTP.
     *
     * If the other side does not answer the ZRTP <em>Hello</em> packets then
     * ZRTP calls this method,
     *
     */
    void zrtpNotSuppOther();

    /**
     * Signal SRTP secrets are ready.
     *
     * This method calls a callback method to inform the host that the SRTP
     * secrets are ready.
     *
     * @param part
     *    Defines for which part (sender or receiver) to switch on security
     * @return
     *    Returns false if something went wrong during initialization of SRTP
     *    context. Propagate error back to state engine.
     */
    bool srtpSecretsReady(EnableSecurity part);

    /**
     * Switch off SRTP secrets.
     *
     * This method calls a callback method to inform the host that the SRTP
     * secrets shall be cleared.
     *
     * @param part
     *    Defines for which part (sender or receiver) to clear
     */
    void srtpSecretsOff(EnableSecurity part);

    /**
     * Helper function to store ZRTP message data in a temporary buffer
     *
     * This functions first clears the temporary buffer, then stores
     * the packet's data to it. We use this to check the packet's HMAC
     * after we received the HMAC key in to following packet.
     *
     * @param data
     *    Pointer to the packet's ZRTP message
    */
     void storeMsgTemp(ZrtpPacketBase* pkt);

     /**
      * Helper function to check a ZRTP message HMAC
      *
      * This function gets a HMAC key and uses it to compute a HMAC
      * with this key and the stored data of a previous received ZRTP
      * message. It compares the computed HMAC and the HMAC stored in
      * the received message and returns the result.
      *
      * @param key
      *    Pointer to the HMAC key.
      * @return
      *    Returns true if the computed HMAC and the stored HMAC match,
      *    false otherwise.
      */
     bool checkMsgHmac(uint8_t* key);

     /**
      * Set the client ID for ZRTP Hello message.
      *
      * The user of ZRTP must set its id to identify itself in the
      * ZRTP HELLO message. The maximum length is 16 characters. Shorter
      * id string are allowed, they will be filled with blanks. A longer id
      * is truncated to 16 characters.
      *
      * The identifier is set in the Hello packet of ZRTP. Thus only after
      * setting the identifier ZRTP can compute the HMAC and the final
      * helloHash.
      *
      * @param id
      *     The client's id
      * @param hpv
      *     Pointer to hello packet version structure.
      */
     void setClientId(const std::string& id, HelloPacketVersion_t* hpv);
     
     /**
      * Check and set a nonce.
      * 
      * The function first checks if the nonce is already in use (was seen) in this ZRTP
      * session. Refer to 4.4.3.1.
      * 
      * @param nonce
      *     The nonce to check and to store if not already seen.
      * 
      * @return
      *     True if the the nonce was stored, thus not yet seen.
      */
     bool checkAndSetNonce(uint8_t* nonce);
};

/**
 * @}
 */
#endif // ZRTP


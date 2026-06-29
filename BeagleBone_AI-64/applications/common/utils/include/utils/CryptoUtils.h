#ifndef CRYPTO_UTILS_H
#define CRYPTO_UTILS_H

#include <string>
#include <vector>
#include <cstdint>

namespace common {
namespace utils {

/**
 * @brief Cryptographic utilities
 * 
 * Provides encryption, hashing, and cryptographic operations
 */
class CryptoUtils {
public:
    /**
     * @brief Hash algorithms
     */
    enum class HashAlgorithm {
        MD5,
        SHA1,
        SHA256,
        SHA384,
        SHA512,
        BLAKE2B,
        BLAKE2S
    };
    
    /**
     * @brief Symmetric encryption algorithms
     */
    enum class SymmetricAlgorithm {
        AES_128_CBC,
        AES_128_GCM,
        AES_256_CBC,
        AES_256_GCM,
        CHACHA20_POLY1305,
        DES3_CBC
    };
    
    /**
     * @brief Hash a string
     * @param input Input string
     * @param algorithm Hash algorithm
     * @return Hexadecimal hash string
     */
    static std::string hash(const std::string& input, 
                           HashAlgorithm algorithm = HashAlgorithm::SHA256);
    
    /**
     * @brief Hash binary data
     * @param data Input data
     * @param algorithm Hash algorithm
     * @return Hexadecimal hash string
     */
    static std::string hashBinary(const std::vector<uint8_t>& data,
                                  HashAlgorithm algorithm = HashAlgorithm::SHA256);
    
    /**
     * @brief HMAC (Hash-based Message Authentication Code)
     * @param key Secret key
     * @param data Input data
     * @param algorithm Hash algorithm
     * @return HMAC as hex string
     */
    static std::string hmac(const std::string& key, const std::string& data,
                           HashAlgorithm algorithm = HashAlgorithm::SHA256);
    
    /**
     * @brief Symmetric encryption
     * @param plaintext Data to encrypt
     * @param key Encryption key (size depends on algorithm)
     * @param iv Initialization vector (size depends on algorithm)
     * @param algorithm Encryption algorithm
     * @return Encrypted data (including IV if needed)
     */
    static std::vector<uint8_t> encrypt(const std::vector<uint8_t>& plaintext,
                                       const std::vector<uint8_t>& key,
                                       const std::vector<uint8_t>& iv,
                                       SymmetricAlgorithm algorithm = SymmetricAlgorithm::AES_256_GCM);
    
    /**
     * @brief Symmetric decryption
     * @param ciphertext Data to decrypt
     * @param key Decryption key
     * @param iv Initialization vector
     * @param algorithm Encryption algorithm
     * @return Decrypted data
     */
    static std::vector<uint8_t> decrypt(const std::vector<uint8_t>& ciphertext,
                                       const std::vector<uint8_t>& key,
                                       const std::vector<uint8_t>& iv,
                                       SymmetricAlgorithm algorithm = SymmetricAlgorithm::AES_256_GCM);
    
    /**
     * @brief Generate random bytes
     * @param count Number of bytes
     * @return Random bytes
     */
    static std::vector<uint8_t> randomBytes(size_t count);
    
    /**
     * @brief Generate random number
     * @param min Minimum value (inclusive)
     * @param max Maximum value (inclusive)
     * @return Random number
     */
    static int randomInt(int min, int max);
    
    /**
     * @brief Generate random UUID (v4)
     */
    static std::string generateUUID();
    
    /**
     * @brief Derive key from password (PBKDF2)
     * @param password Password string
     * @param salt Salt for key derivation
     * @param iterations Number of iterations
     * @param keyLength Desired key length in bytes
     * @return Derived key
     */
    static std::vector<uint8_t> deriveKey(const std::string& password,
                                         const std::vector<uint8_t>& salt,
                                         int iterations = 100000,
                                         size_t keyLength = 32);
    
    /**
     * @brief Generate RSA key pair
     * @param bits Key size in bits (2048, 4096)
     * @return Pair of (private_key, public_key) as PEM strings
     */
    static std::pair<std::string, std::string> generateRSAKeyPair(int bits = 2048);
    
    /**
     * @brief Sign data with RSA private key
     * @param data Data to sign
     * @param privateKey PEM private key
     * @return Signature (base64 encoded)
     */
    static std::string signRSA(const std::string& data, const std::string& privateKey);
    
    /**
     * @brief Verify RSA signature
     * @param data Original data
     * @param signature Base64 signature
     * @param publicKey PEM public key
     * @return true if signature is valid
     */
    static bool verifyRSA(const std::string& data, const std::string& signature,
                         const std::string& publicKey);
    
    /**
     * @brief Compress data (zlib)
     * @param data Input data
     * @return Compressed data
     */
    static std::vector<uint8_t> compress(const std::vector<uint8_t>& data);
    
    /**
     * @brief Decompress data (zlib)
     * @param data Compressed data
     * @return Decompressed data
     */
    static std::vector<uint8_t> decompress(const std::vector<uint8_t>& data);
    
    /**
     * @brief Calculate CRC32 checksum
     * @param data Input data
     * @return CRC32 checksum
     */
    static uint32_t crc32(const std::vector<uint8_t>& data);
    
    /**
     * @brief Calculate CRC32 checksum
     * @param str Input string
     * @return CRC32 checksum
     */
    static uint32_t crc32(const std::string& str);
    
    /**
     * @brief Generate JWT token
     * @param payload JSON payload
     * @param secret Secret for signing
     * @param expiresIn Expiration time in seconds
     * @return JWT token string
     */
    static std::string generateJWT(const std::string& payload, 
                                  const std::string& secret,
                                  int expiresIn = 3600);
    
    /**
     * @brief Verify and decode JWT token
     * @param token JWT token
     * @param secret Secret for verification
     * @return Decoded payload (empty if invalid)
     */
    static std::string verifyJWT(const std::string& token, const std::string& secret);
    
    /**
     * @brief Certificate utilities
     */
    struct Certificate {
        std::string subject;
        std::string issuer;
        std::string serialNumber;
        std::string notBefore;
        std::string notAfter;
        std::string fingerprint;
        std::string pem;
    };
    
    /**
     * @brief Generate self-signed certificate
     * @param commonName Common name
     * @param days Validity in days
     * @param keySize Key size in bits
     * @return Certificate and private key (PEM)
     */
    static std::pair<Certificate, std::string> generateSelfSignedCertificate(
        const std::string& commonName, int days = 365, int keySize = 2048);
    
    /**
     * @brief Parse X.509 certificate
     * @param pem PEM certificate
     * @return Certificate info
     */
    static Certificate parseCertificate(const std::string& pem);
    
    /**
     * @brief Validate certificate
     * @param cert Certificate to validate
     * @param caCert CA certificate
     * @return true if valid
     */
    static bool validateCertificate(const std::string& cert, const std::string& caCert);

private:
    static std::string bytesToHex(const std::vector<uint8_t>& bytes);
    static std::vector<uint8_t> hexToBytes(const std::string& hex);
    static std::string base64Encode(const std::vector<uint8_t>& data);
    static std::vector<uint8_t> base64Decode(const std::string& data);
};

} // namespace utils
} // namespace common

#endif // CRYPTO_UTILS_H

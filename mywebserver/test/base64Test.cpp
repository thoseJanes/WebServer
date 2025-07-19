#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/rand.h>

size_t base64Encode(unsigned char* input, size_t inputSize, char* output){
    BIO* bio;
    BIO* b64;
    bio = BIO_new(BIO_s_mem());
    b64 = BIO_new(BIO_f_base64());
    BIO_push(b64, bio);

    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    BIO_write(b64, input, inputSize);
    BIO_flush(b64);

    size_t len = BIO_pending(bio);
    
    BIO_read(bio, output, len);
    BIO_free_all(bio);
    
    return len;
}

size_t base64Decode(char* input, size_t inputSize, void* output){
    BIO* bio;
    BIO* b64;
    bio = BIO_new(BIO_s_mem());
    BIO_write(bio, input, inputSize);

    b64 = BIO_new(BIO_f_base64());
    BIO_set_flags(b64 ,BIO_FLAGS_BASE64_NO_NL);
    BIO_push(b64, bio);
    BIO_flush(bio);
    
    size_t len = BIO_pending(bio);
    
    
    len = BIO_read(b64, output, len);
    printf("sizeof getkey:%ld\n", len);
    return len;
}

    inline size_t encodeBase64(void* input, size_t inputSize, void* output, size_t outputSize){
        BIO* bio;BIO* b64;
        b64 = BIO_new(BIO_f_base64());
        bio = BIO_new(BIO_s_mem());
        BIO_push(b64, bio);
        BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
        BIO_write(b64, input, inputSize);
        BIO_flush(b64);

        size_t encodingLen = BIO_pending(bio);

        BIO_read(bio, output, outputSize);
        BIO_free_all(bio);
        return encodingLen;
    }
    inline size_t decodeBase64(void* input, size_t inputSize, void* output, size_t outputSize){
        BIO* bio;BIO* b64;
        b64 = BIO_new(BIO_f_base64());
        BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
        bio = BIO_new(BIO_s_mem());
        BIO_write(bio, input, inputSize);
        
        BIO_push(b64, bio);
        BIO_flush(bio);

        size_t len = BIO_read(b64, output, outputSize);
        BIO_free_all(bio);
        return len;
    }

int main(){
    unsigned char key[16];
    RAND_poll();
    RAND_bytes(key, sizeof(key));
    printf("orgKey:%s\n", key);

    //bio方式进行base64编码
    char output[256];
    size_t len = encodeBase64(key, sizeof(key), output, 256);
    printf("len: %ld, \noutput:%s\n", len, output);

    // Base64 编码
    char base64[EVP_ENCODE_LENGTH(sizeof(key))]; // 自动计算足够大的缓冲区
    int length = EVP_EncodeBlock((unsigned char *)base64, key, sizeof(key));
    printf("EVP_EncodeBlock:%s\n", base64);

    //base64解码
    unsigned char getKey[32];
    size_t len2 = decodeBase64(output, len, getKey, 32);
    printf("len:%ld, \ngetKey:%s\n", len2, getKey);

    return 0;
}
#include "webSocket.h"

#include <openssl/bio.h>
#include <openssl/evp.h>

#include "../../myNetLib/common/format.h"
#include "../http/httpRequest.h"
#include "../http/httpResponse.h"

using namespace mywebserver;

size_t webSocket::encodeBase64(void* input, size_t inputSize, void* output, size_t outputSize){
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
size_t webSocket::decodeBase64(void* input, size_t inputSize, void* output, size_t outputSize){
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

string webSocket::calAcceptKey(const string& webSocketKey){
    string connectKey = webSocketKey + string(magicNumber, strlen(magicNumber));

    unsigned char hash[SHA_DIGEST_LENGTH];
    SHA_CTX sha1;
    SHA1_Init(&sha1);
    SHA1_Update(&sha1, connectKey.data(), connectKey.size());
    SHA1_Final(hash, &sha1);

    // char hexStr[SHA_DIGEST_LENGTH*2];
    // char* pos = hexStr;
    // for(int i=0;i<SHA_DIGEST_LENGTH;i++){
    //     size_t out = mynetlib::detail::formatInteger<unsigned char>(pos, hash[i]);
    //     pos += out;
    //     assert(out == 2);
    // }
    //不需要把哈希值转化成16进制字符。
    
    char base64Str[kAcceptKeyLength];
    encodeBase64(hash, sizeof(hash), base64Str, kAcceptKeyLength);
    return string(base64Str, kAcceptKeyLength);
}

bool webSocket::isWebSocketHandShakeRequestValid(const mywebserver::HttpRequest& request){
    return request.getMethod() == http::mGET &&
            (request.getHeaderValue("Sec-WebSocket-Key").size() == webSocket::kKeyLength_) &&
            (!request.getHeaderValue("Sec-WebSocket-Version").empty()) &&
            (request.getHeaderValue("Upgrade") == "websocket") &&
            (request.getHeaderValue("Connection") == "Upgrade");
}

bool webSocket::isWebSocketHandShakeResponseValid(const mywebserver::HttpRequest& request, const mywebserver::HttpResponse& response){
    return isWebSocketHandShakeRequestValid(request) &&
            response.getStatusCode() == 101 &&
            (response.getHeaderValue("Sec-WebSocket-Version") == request.getHeaderValue("Sec-WebSocket-Version")) &&
            (response.getHeaderValue("Sec-WebSocket-Accept") == webSocket::calAcceptKey(request.getHeaderValue("Sec-WebSocket-Key"))) &&
            (response.getHeaderValue("Upgrade") == "websocket") &&
            (response.getHeaderValue("Connection") == "Upgrade");
            //TOBECOMPLETED//extensions和protocol的判断。
}
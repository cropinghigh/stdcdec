#include <iostream>
#include <inmarsatc_decoder.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <map>
#include <cstring>

#define TOLERANCE 9

void printHelp() {
    std::cout << "Help: " << std::endl;
    std::cout << "stdc_decoder - open-source cli program to decode inmarsat-C symbols using inmarsatc library based on Scytale-C source code" << std::endl;
    std::cout << "Keys: " << std::endl;
    std::cout << "--help                                    - this help" << std::endl;
    std::cout << "--verbose                                 - print all frames in hex" << std::endl;
    std::cout << "--in-udp <port>                           - input symbols via udp(default port: 15003)" << std::endl;
    std::cout << "--out-udp <ip> <port>                     - send decoded frames via udp(default: 127.0.0.1:15004)" << std::endl;
    std::cout << "(one source and one out parameters should be selected)" << std::endl;
}

int parseArg(int argc, int* position, char* argv[], std::map<std::string, std::string>* params, bool recursive) {
    std::string arg1 = std::string(argv[*position]);
    //i would be using switch() here... but it's not available for strings, so...
    if(arg1 == "--help") {
        return 1;
    } else if(arg1 == "--verbose") {
        params->insert(std::pair<std::string, std::string>("decoderVerbose", "true"));
        return 0;
    } else if(arg1 == "--in-udp") {
        std::string arg2;
        arg2 = "15003";
        int nextpos = *position + 1;
        if(nextpos < argc and !recursive) {
            int parseRes = parseArg(argc, &nextpos, argv, params, true);
            if(parseRes == 2) {
                arg2 = std::string(argv[nextpos]);
            }
            *position = nextpos;
        }
        params->insert(std::pair<std::string, std::string>("decoderSource", "udp"));
        params->insert(std::pair<std::string, std::string>("decoderSourceUdpPort", arg2));
        return 0;
    } else if(arg1 == "--out-udp") {
        std::string arg2;
        std::string arg3;
        arg2 = "127.0.0.1";
        arg3 = "15004";
        int nextpos = *position + 1;
        if(nextpos < argc and !recursive) {
            int parseRes = parseArg(argc, &nextpos, argv, params, true);
            if(parseRes == 2) {
                arg2 = std::string(argv[nextpos]);
            }
            *position = nextpos;
            nextpos++;
            if(nextpos < argc) {
                parseRes = parseArg(argc, &nextpos, argv, params, true);
                if(parseRes == 2) {
                    arg3 = std::string(argv[nextpos]);
                }
                *position = nextpos;
            }
        }
        params->insert(std::pair<std::string, std::string>("decoderOutUdp", "true"));
        params->insert(std::pair<std::string, std::string>("decoderOutUdpIp", arg2));
        params->insert(std::pair<std::string, std::string>("decoderOutUdpPort", arg3));
        return 0;
    } else {
        return 2;
    }
}

void printDecodedFrameVerbose(inmarsatc::decoder::Decoder::decoder_result frame) {
    std::cout << "decoded frame:               " << std::endl;
    std::cout << "  len: " << std::dec << frame.length << std::endl;
    std::cout << "  frameNumber: " << std::dec << frame.frameNumber << std::endl;
    std::cout << "  isReversedPolarity: " << std::dec << frame.isReversedPolarity << std::endl;
    std::cout << "  isMidStreamReversePolarity: " << std::dec << frame.isMidStreamReversePolarity << std::endl;
    std::cout << "  isUncertain: " << std::dec << frame.isUncertain << std::endl;
    std::cout << "  BER: " << std::dec << frame.BER << std::endl;
    std::cout << "  data = {" << std::endl;
    for(int k = 0; k < DESCRAMBLER_FRAME_LENGTH; k++) {
        std::cout << std::hex << (uint16_t)frame.decodedFrame[k] << " ";
    }
    std::cout << std::endl << " }"  << std::endl;
}

inmarsatc::demodulator::Demodulator::demodulator_result receiveDemodDataViaUdp(int sockfd, sockaddr_in serveraddr) {
    inmarsatc::demodulator::Demodulator::demodulator_result ret;
    ret.meanMagnitude = 0;
    socklen_t len_useless = sizeof(serveraddr);
    recvfrom(sockfd, (char *)ret.bitsDemodulated, (sizeof(ret.bitsDemodulated)), MSG_WAITALL, ( struct sockaddr *) &serveraddr, &len_useless);
    return ret;
}

//serialization
template< typename T >
std::array< char, sizeof(T) >  to_bytes( const T& object ) {
    std::array< char, sizeof(T) > bytes ;

    const char* begin = reinterpret_cast< const char* >( std::addressof(object) ) ;
    const char* end = begin + sizeof(T) ;
    std::copy( begin, end, std::begin(bytes) ) ;

    return bytes ;
}

template< typename T >
T& from_bytes( const std::array< char, sizeof(T) >& bytes, T& object )
{
    // http://en.cppreference.com/w/cpp/types/is_trivially_copyable
    static_assert( std::is_trivially_copyable<T>::value, "not a TriviallyCopyable type" ) ;

    char* begin_object = reinterpret_cast< char* >( std::addressof(object) ) ;
    std::copy( std::begin(bytes), std::end(bytes), begin_object ) ;

    return object ;
}


void sendDecodedFrameViaUdp(inmarsatc::decoder::Decoder::decoder_result data, int sockfd, sockaddr_in serveraddr) {
    auto serialized = to_bytes(data);

    sendto(sockfd, serialized.data(), serialized.size(), 0, (const struct sockaddr *) &serveraddr, sizeof(serveraddr));
}

int main(int argc, char* argv[]) {
    std::map<std::string, std::string> params;
    if(argc < 2) {
        printHelp();
        return 1;
    }
    for(int i = 1; i < argc; i++) {
        int res = parseArg(argc, &i, argv, &params, false);
        if(res == 1 or res == 2) {
            std::cout << "Wrong args!" << std::endl;
            printHelp();
            return 1;
        }
    }
    bool isDecoderVerbose = params.find("decoderVerbose") != params.end() && params["decoderVerbose"] == "true";
    if(params.find("decoderSource") == params.end() || params.find("decoderOutUdp") == params.end() || params["decoderOutUdp"] != "true" || params.find("decoderOutUdpIp") == params.end() || params.find("decoderOutUdpPort") == params.end()) {
        std::cout << "Wrong/No source/out selected!" << std::endl;
        printHelp();
        return 1;
    }
    std::string decoderSource = params["decoderSource"];
    std::string udpIp = params["decoderOutUdpIp"];
    std::string udpPort = params["decoderOutUdpPort"];
    sockaddr_in clientaddr;
    memset(&clientaddr, 0, sizeof(clientaddr));
    // Filling server information
    clientaddr.sin_family = AF_INET;
    clientaddr.sin_port = htons(std::stoi(udpPort));
    clientaddr.sin_addr.s_addr=inet_addr(udpIp.c_str());
    inmarsatc::decoder::Decoder decoder(TOLERANCE);
    int sockfd;
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
        std::cout << "Socket creation failed!" << std::endl;
        return 1;
    }
    if(decoderSource == "udp") {
        if(params.find("decoderSourceUdpPort") == params.end()) {
            std::cout << "Udp port not specified!" << std::endl;
            return 1;
        }
        std::string udpPort = params["decoderSourceUdpPort"];
        int clisockfd;
        if ((clisockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
            std::cout << "Socket creation failed!" << std::endl;
            return 1;
        }
        // Filling server information
        sockaddr_in serveraddr, cliaddr;
        serveraddr.sin_family = AF_INET;
        serveraddr.sin_port = htons(std::stoi(udpPort));
        serveraddr.sin_addr.s_addr=INADDR_ANY;
        if (bind(clisockfd, (const struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0) {
                std::cout << "Binding to port failed!" << std::endl;
                return 1;
        }
        int received;
        while(true) {
            inmarsatc::demodulator::Demodulator::demodulator_result syms = receiveDemodDataViaUdp(clisockfd, serveraddr);
            std::vector<inmarsatc::decoder::Decoder::decoder_result> dec_res = decoder.decode(syms.bitsDemodulated);
            for(int i = 0; i < dec_res.size(); i++) {
                if(isDecoderVerbose) {
                    printDecodedFrameVerbose(dec_res[i]);
                }
                sendDecodedFrameViaUdp(dec_res[i], sockfd, clientaddr);
            }
        }
    }
}

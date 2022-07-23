#include <iostream>
#include <inmarsatc_parser.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <map>
#include <json.hpp> //https://github.com/nlohmann/json

using json = nlohmann::json;

void printHelp() {
    std::cout << "Help: " << std::endl;
    std::cout << "stdc_parse - open-source cli program to parse inmarsat-C frames using inmarsatc library based on Scytale-C source code" << std::endl;
    std::cout << "Keys: " << std::endl;
    std::cout << "--help                                    - this help" << std::endl;
    std::cout << "--verbose                                 - print all data for all parsed packets" << std::endl;
    std::cout << "--in-udp <port>                           - input decoded frames via udp(default port: 15004)" << std::endl;
    std::cout << "--out-udp <ip> <port>                     - output parsed packets via udp JSON(default: 127.0.0.1 15005)" << std::endl;
    std::cout << "--print-all-packets                       - parse data for any packets type(otherwise just message packets)" << std::endl;
    std::cout << "(one source should be selected)" << std::endl;
}

int parseArg(int argc, int* position, char* argv[], std::map<std::string, std::string>* params, bool recursive) {
    std::string arg1 = std::string(argv[*position]);
    //i would be using switch() here... but it's not available for strings, so...
    if(arg1 == "--help") {
        return 1;
    } else if(arg1 == "--verbose") {
        params->insert(std::pair<std::string, std::string>("frameparserVerbose", "true"));
        return 0;
    } else if(arg1 == "--in-udp") {
        std::string arg2;
        arg2 = "15004";
        int nextpos = *position + 1;
        if(nextpos < argc and !recursive) {
            int parseRes = parseArg(argc, &nextpos, argv, params, true);
            if(parseRes == 2) {
                arg2 = std::string(argv[nextpos]);
            }
            *position = nextpos;
        }
        params->insert(std::pair<std::string, std::string>("frameparserSource", "udp"));
        params->insert(std::pair<std::string, std::string>("frameparserSourceUdpPort", arg2));
        return 0;
    } else if(arg1 == "--out-udp") {
        std::string arg2;
        std::string arg3;
        arg2 = "127.0.0.1";
        arg3 = "15005";
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
        params->insert(std::pair<std::string, std::string>("frameparserOutUdp", "true"));
        params->insert(std::pair<std::string, std::string>("frameparserOutUdpIp", arg2));
        params->insert(std::pair<std::string, std::string>("frameparserOutUdpPort", arg3));
        return 0;
    } else if(arg1 == "--print-all-packets") {
        params->insert(std::pair<std::string, std::string>("frameparserPrintAllPackets", "true"));
        return 0;
    } else {
        return 2;
    }
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

void sendParserDataViaUdp(std::string data, int sockfd, sockaddr_in serveraddr) {
    sendto(sockfd, data.c_str(), data.size(), 0, (const struct sockaddr *) &serveraddr, sizeof(serveraddr));
}

inmarsatc::decoder::Decoder::decoder_result receiveDemodDataViaUdp(int sockfd, sockaddr_in serveraddr) {
    inmarsatc::decoder::Decoder::decoder_result ret;
    socklen_t len_useless = sizeof(serveraddr);
    char buf[sizeof(inmarsatc::decoder::Decoder::decoder_result)];
    recvfrom(sockfd, &buf, (sizeof(buf)), MSG_WAITALL, ( struct sockaddr *) &serveraddr, &len_useless);
    std::array<char, sizeof(inmarsatc::decoder::Decoder::decoder_result)> buf_std;
    std::copy(std::begin(buf), std::end(buf), std::begin(buf_std));
    from_bytes(buf_std, ret);
    return ret;
}

bool ifPacketIsMessage(inmarsatc::frameParser::FrameParser::frameParser_result pack_dec_res) {
    switch(pack_dec_res.decoding_result.packetDescriptor) {
        case 0xA3:
        case 0xA8:
            return pack_dec_res.decoding_result.packetVars.find("shortMessage") != pack_dec_res.decoding_result.packetVars.end();
        case 0xAA:
        case 0xB1:
        case 0xB2:
            return true;
        default:
            return false;
    }
}

void printFrameParserPacket(inmarsatc::frameParser::FrameParser::frameParser_result pack_dec_res, bool isFrameParserPrintAllPackets,  bool isFrameParserVerbose) {
    if(isFrameParserPrintAllPackets || ifPacketIsMessage(pack_dec_res)) {
        std::cout << "packet:                        " << std::endl;
        std::cout << "  type: " << pack_dec_res.decoding_result.packetVars["packetDescriptorText"] << std::dec << " (" << std::hex << (uint16_t)pack_dec_res.decoding_result.packetDescriptor << ")" << std::endl;
        if(isFrameParserVerbose) {
            std::cout << "  frameNumber: " << pack_dec_res.decoding_result.frameNumber << std::endl;
            std::time_t timestamp = std::chrono::high_resolution_clock::to_time_t(pack_dec_res.decoding_result.timestamp);
            std::cout << "  timestamp: " << std::ctime(&timestamp);
            std::cout << "  decodingStage: " << (pack_dec_res.decoding_result.decodingStage == PACKETDECODER_DECODING_STAGE_NONE ? "none" : (pack_dec_res.decoding_result.decodingStage == PACKETDECODER_DECODING_STAGE_COMPLETE ? "complete" : "partial")) << std::endl;
            std::cout << "  packetLength: " << pack_dec_res.decoding_result.packetLength << std::endl;
            bool payload = pack_dec_res.decoding_result.payload.presentation != -1;
            std::cout << "  payload: " << (payload ? "yes" : "no") << std::endl;
            if(payload) {
                std::cout << "      presentation: " << (pack_dec_res.decoding_result.payload.presentation == PACKETDECODER_PRESENTATION_BINARY ? "binary" : (pack_dec_res.decoding_result.payload.presentation == PACKETDECODER_PRESENTATION_IA5 ? "IA5" : (pack_dec_res.decoding_result.payload.presentation == PACKETDECODER_PRESENTATION_ITA2 ? "ITA2" : "unknown"))) << std::endl;
                std::cout << "      data: {" << std::endl << "          ";
                if(pack_dec_res.decoding_result.payload.presentation == PACKETDECODER_PRESENTATION_IA5) {
                    for(int i = 0; i < (int)pack_dec_res.decoding_result.payload.data8Bit.size(); i++) {
                        char chr = pack_dec_res.decoding_result.payload.data8Bit[i] & 0b01111111;
                        if((chr < 0x20 && chr != '\n' && chr != '\r')) {
                            std::cout << "(" << std::hex << (uint16_t)chr << std::dec << ")";
                        } else if(chr != '\n' && chr != '\r') {
                            std::cout << chr;
                        } else {
                            std::cout << std::endl << "         ";
                        }
                    }
                } else if(pack_dec_res.decoding_result.payload.presentation == PACKETDECODER_PRESENTATION_ITA2) {

                } else {
                    for(int i = 0; i < (int)pack_dec_res.decoding_result.payload.data8Bit.size(); i++) {
                        uint8_t data = pack_dec_res.decoding_result.payload.data8Bit[i];
                        std::cout << std::hex << (uint16_t)data << std::dec << " ";
                    }
                }
                std::cout << std::endl << "      }" << std::endl;
            }
            std::cout << "  packetVars:" << std::endl;
            for(std::map<std::string, std::string>::const_iterator it = pack_dec_res.decoding_result.packetVars.begin(); it != pack_dec_res.decoding_result.packetVars.end(); ++it) {
                std::string key = it->first;
                std::string data = it->second;
                if(key == "packetDescriptorText") {
                    continue;
                }
                std::cout << "      " << key << ": " << std::endl << "          ";
                for(int k = 0; k < (int)data.length(); k++) {
                    if(data[k] != '\n') {
                        std::cout << data[k];
                    } else {
                        std::cout << std::endl << "          ";
                    }
                }
                std::cout << std::endl;
            }
        } else {
            switch(pack_dec_res.decoding_result.packetDescriptor) {
                case 0x27:
                    {
                        std::cout << "  msgId: " << pack_dec_res.decoding_result.packetVars["mesId"] << " sat: " << pack_dec_res.decoding_result.packetVars["satName"] << " les: " << pack_dec_res.decoding_result.packetVars["lesName"] << " LCN: " << pack_dec_res.decoding_result.packetVars["logicalChannelNo"] << std::endl;
                    }
                    break;
                case 0x2A:
                    {
                        std::cout << "  msgId: " << pack_dec_res.decoding_result.packetVars["mesId"] << " sat: " << pack_dec_res.decoding_result.packetVars["satName"] << " les: " << pack_dec_res.decoding_result.packetVars["lesName"] << " LCN: " << pack_dec_res.decoding_result.packetVars["logicalChannelNo"] << std::endl;
                    }
                    break;
                case 0x08:
                    {
                        std::cout << "  sat: " << pack_dec_res.decoding_result.packetVars["satName"] << " les: " << pack_dec_res.decoding_result.packetVars["lesName"] << " LCN: " << pack_dec_res.decoding_result.packetVars["logicalChannelNo"] << " ULF: " << pack_dec_res.decoding_result.packetVars["uplinkChannelMhz"] << std::endl;
                    }
                    break;
                case 0x6C:
                    {
                        std::cout << "  ULF: " << pack_dec_res.decoding_result.packetVars["uplinkChannelMhz"] << std::endl;
                        std::cout << "  Services: ";
                        std::string services = pack_dec_res.decoding_result.packetVars["services"];
                        for(int k = 0; k < (int)services.length(); k++) {
                            if(services[k] != '\n') {
                                std::cout << services[k];
                            } else {
                                std::cout << " ";
                            }
                        }
                        std::cout << std::endl;
                        std::cout << "  Tdm slots: ";
                        std::string tdmSlots = pack_dec_res.decoding_result.packetVars["tdmSlots"];
                        for(int k = 0; k < (int)tdmSlots.length(); k++) {
                            if(tdmSlots[k] != '\n') {
                                std::cout << tdmSlots[k];
                            } else {
                                std::cout << " ";
                            }
                        }
                        std::cout << std::endl;
                    }
                    break;
                case 0x7D:
                    {
                        std::cout << "  netVer: " << pack_dec_res.decoding_result.packetVars["networkVersion"] << " sat: " << pack_dec_res.decoding_result.packetVars["satName"] << " les: " << pack_dec_res.decoding_result.packetVars["lesName"] << " sigCh: " << pack_dec_res.decoding_result.packetVars["signallingChannel"] << " count: " << pack_dec_res.decoding_result.packetVars["count"] << " chType: " << pack_dec_res.decoding_result.packetVars["channelTypeName"] << " sat: " << pack_dec_res.decoding_result.packetVars["satName"] << " local: " << pack_dec_res.decoding_result.packetVars["local"] << " randInt: " << pack_dec_res.decoding_result.packetVars["randomInterval"] << std::endl;
                        std::cout << "  Status: ";
                        std::string status = pack_dec_res.decoding_result.packetVars["status"];
                        for(int k = 0; k < (int)status.length(); k++) {
                            if(status[k] != '\n') {
                                std::cout << status[k];
                            } else {
                                std::cout << " ";
                            }
                        }
                        std::cout << std::endl;
                        std::cout << "  Services: ";
                        std::string services = pack_dec_res.decoding_result.packetVars["services"];
                        for(int k = 0; k < (int)services.length(); k++) {
                            if(services[k] != '\n') {
                                std::cout << services[k];
                            } else {
                                std::cout << " ";
                            }
                        }
                        std::cout << std::endl;
                    }
                    break;
                case 0x81:
                    {
                        std::cout << "  msgId: " << pack_dec_res.decoding_result.packetVars["mesId"] << " sat: " << pack_dec_res.decoding_result.packetVars["satName"] << " les: " << pack_dec_res.decoding_result.packetVars["lesName"] << " LCN: " << pack_dec_res.decoding_result.packetVars["logicalChannelNo"] << " dlFr: " << pack_dec_res.decoding_result.packetVars["downlinkChannelMhz"] << " pres: " << pack_dec_res.decoding_result.packetVars["presentation"] << std::endl;
                    }
                    break;
                case 0x83:
                    {
                        std::cout << "  msgId: " << pack_dec_res.decoding_result.packetVars["mesId"] << " sat: " << pack_dec_res.decoding_result.packetVars["satName"] << " les: " << pack_dec_res.decoding_result.packetVars["lesName"] << " status: " << pack_dec_res.decoding_result.packetVars["status_bits"] << " LCN: " << pack_dec_res.decoding_result.packetVars["logicalChannelNo"] << " frLen: " << pack_dec_res.decoding_result.packetVars["frameLength"] << " dur: " << pack_dec_res.decoding_result.packetVars["duration"] << " dlFr: " << pack_dec_res.decoding_result.packetVars["downlinkChannelMhz"] << " ulFr: " << pack_dec_res.decoding_result.packetVars["uplinkChannelMhz"] << " frOffs: " << pack_dec_res.decoding_result.packetVars["frameOffset"] << " PD1: " << pack_dec_res.decoding_result.packetVars["packetDescriptor1"] << std::endl;
                    }
                    break;
                case 0x91:
                    {
                        //not implemented
                    }
                    break;
                case 0x92:
                    {
                        std::cout << "  loginAckLen: " << pack_dec_res.decoding_result.packetVars["loginAckLength"] << " dlFr: " << pack_dec_res.decoding_result.packetVars["downlinkChannelMhz"] << " les: " << pack_dec_res.decoding_result.packetVars["les"] << " stStartHex: " << pack_dec_res.decoding_result.packetVars["stationStartHex"] << std::endl;
                        if(pack_dec_res.decoding_result.packetVars.find("stationCount") != pack_dec_res.decoding_result.packetVars.end()) {
                            std::cout << "  stationCnt: " << pack_dec_res.decoding_result.packetVars["stationCount"] << " Stations: ";
                            std::string stations = pack_dec_res.decoding_result.packetVars["stations"];
                            for(int k = 0; k < (int)stations.length(); k++) {
                                if(stations[k] != '\n') {
                                    std::cout << stations[k];
                                } else {
                                    std::cout << " ";
                                }
                            }
                            std::cout << std::endl;
                        }
                    }
                    break;
                case 0x9A:
                    {
                        //not implemented
                    }
                    break;
                case 0xA0:
                    {
                        //not implemented
                    }
                    break;
                case 0xA3:
                    {
                        std::cout << "  msgId: " << pack_dec_res.decoding_result.packetVars["mesId"] << " sat: " << pack_dec_res.decoding_result.packetVars["satName"] << " les: " << pack_dec_res.decoding_result.packetVars["lesName"]<< std::endl;
                        if(pack_dec_res.decoding_result.packetVars.find("shortMessage") != pack_dec_res.decoding_result.packetVars.end()) {
                            std::cout << "  Short message: " << std::endl << "      ";
                            std::string shortMessage = pack_dec_res.decoding_result.packetVars["shortMessage"];
                            for(int k = 0; k < (int)shortMessage.length(); k++) {
                                char chr = shortMessage[k] & 0x7F;
                                if((chr < 0x20 && chr != '\n' && chr != '\r')) {
                                    std::cout << "(" << std::hex << (uint16_t)chr << std::dec << ")";
                                } else if(chr != '\n'  && chr != '\r') {
                                    std::cout << chr;
                                } else {
                                    std::cout << std::endl << "     ";
                                }
                            }
                            std::cout << std::endl;
                        }
                    }
                    break;
                case 0xA8:
                    {
                        std::cout << "  msgId: " << pack_dec_res.decoding_result.packetVars["mesId"] << " sat: " << pack_dec_res.decoding_result.packetVars["satName"] << " les: " << pack_dec_res.decoding_result.packetVars["lesName"]<< std::endl;
                        if(pack_dec_res.decoding_result.packetVars.find("shortMessage") != pack_dec_res.decoding_result.packetVars.end()) {
                            std::cout << "  Short message: " << std::endl << "      ";
                            std::string shortMessage = pack_dec_res.decoding_result.packetVars["shortMessage"];
                            for(int k = 0; k < (int)shortMessage.length(); k++) {
                                char chr = shortMessage[k] & 0x7F;
                                if((chr < 0x20 && chr != '\n' && chr != '\r')) {
                                    std::cout << "(" << std::hex << (uint16_t)chr << std::dec << ")";
                                } else if(chr != '\n'  && chr != '\r') {
                                    std::cout << chr;
                                } else {
                                    std::cout << std::endl << "     ";
                                }
                            }
                            std::cout << std::endl;
                        }
                    }
                    break;
                case 0xAA:
                    {
                        std::cout << "  sat: " << pack_dec_res.decoding_result.packetVars["satName"] << " les: " << pack_dec_res.decoding_result.packetVars["lesName"] << " LCN: " << pack_dec_res.decoding_result.packetVars["logicalChannelNo"] << " packetNo: " << pack_dec_res.decoding_result.packetVars["packetNo"] << std::endl;
                        bool isBinary = pack_dec_res.decoding_result.payload.presentation == PACKETDECODER_PRESENTATION_BINARY;
                        std::cout << "  Message(" << (isBinary ? "hex" : "text") << "): " << std::endl << "     ";
                        if(pack_dec_res.decoding_result.payload.presentation == PACKETDECODER_PRESENTATION_IA5) {
                            for(int i = 0; i < (int)pack_dec_res.decoding_result.payload.data8Bit.size(); i++) {
                                char chr = pack_dec_res.decoding_result.payload.data8Bit[i] & 0x7F;
                                if((chr < 0x20 && chr != '\n' && chr != '\r')) {
                                    std::cout << "(" << std::hex << (uint16_t)chr << std::dec << ")";
                                } else if(chr != '\n' && chr != '\r') {
                                    std::cout << chr;
                                } else {
                                    std::cout << std::endl << "         ";
                                }
                            }
                        } else if(pack_dec_res.decoding_result.payload.presentation == PACKETDECODER_PRESENTATION_ITA2) {

                        } else {
                            for(int i = 0; i < (int)pack_dec_res.decoding_result.payload.data8Bit.size(); i++) {
                                uint8_t data = pack_dec_res.decoding_result.payload.data8Bit[i];
                                std::cout << std::hex << (uint16_t)data << std::dec << " ";
                            }
                        }
                        std::cout << std::endl;
                    }
                    break;
                case 0xAB:
                    {
                        std::cout << "  lesListLen: " << pack_dec_res.decoding_result.packetVars["lesListLength"] << " stStartHex: " << pack_dec_res.decoding_result.packetVars["stationStartHex"] << " stCnt: " << pack_dec_res.decoding_result.packetVars["stationCount"] << std::endl;
                        std::cout << " Stations: ";
                        std::string stations = pack_dec_res.decoding_result.packetVars["stations"];
                        for(int k = 0; k < (int)stations.length(); k++) {
                            if(stations[k] != '\n') {
                                std::cout << stations[k];
                            } else {
                                std::cout << " ";
                            }
                        }
                        std::cout << std::endl;
                    }
                    break;
                case 0xAC:
                    {
                        //not implemented
                    }
                    break;
                case 0xAD:
                    {
                        //not implemented
                    }
                    break;
                case 0xB1:
                    {
                        std::cout << "  msgType: " << pack_dec_res.decoding_result.packetVars["messageType"] << " svcCd&AddrName: " << pack_dec_res.decoding_result.packetVars["serviceCodeAndAddressName"] << " contin: " << pack_dec_res.decoding_result.packetVars["continuation"] << " prio: " << pack_dec_res.decoding_result.packetVars["priorityText"] << " rep: " << pack_dec_res.decoding_result.packetVars["repetition"] << " msgId: " << pack_dec_res.decoding_result.packetVars["messageId"] << " packetNo: " << pack_dec_res.decoding_result.packetVars["packetNo"] << " isNewPayl: " << pack_dec_res.decoding_result.packetVars["isNewPayload"] << " addrHex: " << pack_dec_res.decoding_result.packetVars["addressHex"] << std::endl;
                        bool isBinary = pack_dec_res.decoding_result.payload.presentation == PACKETDECODER_PRESENTATION_BINARY;
                        std::cout << "  Payload(" << (isBinary ? "hex" : "text") << "): " << std::endl << "     ";
                        if(!isBinary) {
                            for(int k = 0; k < (int)pack_dec_res.decoding_result.payload.data8Bit.size(); k++) {
                                char chr = pack_dec_res.decoding_result.payload.data8Bit[k] & 0x7F;
                                if((chr < 0x20 && chr != '\n' && chr != '\r')) {
                                    std::cout << "(" << std::hex << (uint16_t)chr << std::dec << ")";
                                } else if(chr != '\n'  && chr != '\r') {
                                    std::cout << chr;
                                } else {
                                    std::cout << std::endl << "     ";
                                }
                            }
                        } else {
                            for(int i = 0; i < (int)pack_dec_res.decoding_result.payload.data8Bit.size(); i++) {
                                uint8_t data = pack_dec_res.decoding_result.payload.data8Bit[i];
                                std::cout << std::hex << (uint16_t)data << std::dec << " ";
                            }
                        }
                        std::cout << std::endl;
                    }
                    break;
                case 0xB2:
                    {
                        std::cout << "  msgType: " << pack_dec_res.decoding_result.packetVars["messageType"] << " svcCd&AddrName: " << pack_dec_res.decoding_result.packetVars["serviceCodeAndAddressName"] << " contin: " << pack_dec_res.decoding_result.packetVars["continuation"] << " prio: " << pack_dec_res.decoding_result.packetVars["priorityText"] << " rep: " << pack_dec_res.decoding_result.packetVars["repetition"] << " msgId: " << pack_dec_res.decoding_result.packetVars["messageId"] << " packetNo: " << pack_dec_res.decoding_result.packetVars["packetNo"] << " isNewPayl: " << pack_dec_res.decoding_result.packetVars["isNewPayload"] << " addrHex: " << pack_dec_res.decoding_result.packetVars["addressHex"] << std::endl;
                        bool isBinary = pack_dec_res.decoding_result.payload.presentation == PACKETDECODER_PRESENTATION_BINARY;
                        std::cout << "  Payload(" << (isBinary ? "hex" : "text") << "): " << std::endl << "     ";
                        if(!isBinary) {
                            for(int k = 0; k < (int)pack_dec_res.decoding_result.payload.data8Bit.size(); k++) {
                                char chr = pack_dec_res.decoding_result.payload.data8Bit[k] & 0x7F;
                                if((chr < 0x20 && chr != '\n' && chr != '\r')) {
                                    std::cout << "(" << std::hex << (uint16_t)chr << std::dec << ")";
                                } else if(chr != '\n'  && chr != '\r') {
                                    std::cout << chr;
                                } else {
                                    std::cout << std::endl << "     ";
                                }
                            }
                        } else {
                            for(int i = 0; i < (int)pack_dec_res.decoding_result.payload.data8Bit.size(); i++) {
                                uint8_t data = pack_dec_res.decoding_result.payload.data8Bit[i];
                                std::cout << std::hex << (uint16_t)data << std::dec << " ";
                            }
                        }
                        std::cout << std::endl;
                    }
                    break;
                case 0xBD:
                    {
                        //nothing
                    }
                    break;
                case 0xBE:
                    {
                        //nothing
                    }
                    break;
                default:
                    break;
            }
        }
        std::cout << std::endl;
    }
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
    bool isFrameparserVerbose = params.find("frameparserVerbose") != params.end() && params["frameparserVerbose"] == "true";
    bool isFrameparserPrintAllPackets = params.find("frameparserPrintAllPackets") != params.end() && params["frameparserPrintAllPackets"] == "true";
    bool isFrameparserOutUdp = params.find("frameparserOutUdp") != params.end() && params["frameparserOutUdp"] == "true";
    sockaddr_in clientaddr;
    int sockfd;
    if(isFrameparserOutUdp) {
        std::string udpIp = params["frameparserOutUdpIp"];
        std::string udpPort = params["frameparserOutUdpPort"];
        memset(&clientaddr, 0, sizeof(clientaddr));
        // Filling server information
        clientaddr.sin_family = AF_INET;
        clientaddr.sin_port = htons(std::stoi(udpPort));
        clientaddr.sin_addr.s_addr=inet_addr(udpIp.c_str());
        if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
            std::cout << "Socket creation failed!" << std::endl;
            return 1;
        }
    }
    if(params.find("frameparserSource") == params.end()) {
        std::cout << "Wrong/No source selected!" << std::endl;
        printHelp();
        return 1;
    }
    std::string frameparserSource = params["frameparserSource"];
    inmarsatc::frameParser::FrameParser parser;
    if(frameparserSource == "udp") {
        if(params.find("frameparserSourceUdpPort") == params.end()) {
            std::cout << "Udp port not specified!" << std::endl;
            return 1;
        }
        std::string udpPort = params["frameparserSourceUdpPort"];
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
            inmarsatc::decoder::Decoder::decoder_result frame = receiveDemodDataViaUdp(clisockfd, serveraddr);
            std::vector<inmarsatc::frameParser::FrameParser::frameParser_result> pack_dec_res_vec = parser.parseFrame(frame);
            for(int k = 0; k < (int)pack_dec_res_vec.size(); k++) {
                inmarsatc::frameParser::FrameParser::frameParser_result pack_dec_res = pack_dec_res_vec[k];
                if(!pack_dec_res.decoding_result.isDecodedPacket || !pack_dec_res.decoding_result.isCrc) {
                    continue;
                }
                printFrameParserPacket(pack_dec_res, isFrameparserPrintAllPackets, isFrameparserVerbose);
                if(isFrameparserOutUdp) {
                    if((isFrameparserPrintAllPackets || (ifPacketIsMessage(pack_dec_res))) && pack_dec_res.decoding_result.isDecodedPacket) {
                        json j;
                        j["frameNumber"] = pack_dec_res.decoding_result.frameNumber;
                        j["timestamp"] = pack_dec_res.decoding_result.timestamp.time_since_epoch().count();
                        j["packetDescriptor"] = pack_dec_res.decoding_result.packetDescriptor;
                        j["packetLength"] = pack_dec_res.decoding_result.packetLength;
                        j["decodingStage"] = pack_dec_res.decoding_result.decodingStage == PACKETDECODER_DECODING_STAGE_COMPLETE ? "Complete" : (pack_dec_res.decoding_result.decodingStage == PACKETDECODER_DECODING_STAGE_PARTIAL ? "Partial" : "None");
                        j["payload"]["presentation"] = pack_dec_res.decoding_result.payload.presentation;
                        std::ostringstream os;
                        for(int i = 0; i < (int)pack_dec_res.decoding_result.payload.data8Bit.size(); i++) {
                            char chr = pack_dec_res.decoding_result.payload.data8Bit[i] & 0b01111111;
                            if((chr < 0x20 && chr != '\n' && chr != '\r')) {
                                os << "(" << std::hex << (uint16_t)chr << std::dec << ")";
                            } else if(chr != '\n' && chr != '\r') {
                                os << chr;
                            } else {
                                os << std::endl;
                            }
                        }
                        std::string payload_data = os.str();
                        j["payload"]["data"] = payload_data;
                        j["packetVars"] = pack_dec_res.decoding_result.packetVars;
                        std::string data = j.dump(-1, ' ', false, json::error_handler_t::ignore);
                        sendParserDataViaUdp(data, sockfd, clientaddr);
                    }
                }
            }
        }
    }
}

#include <iostream>
#include <inmarsatc.h>
#include <audiofile.h>
#include <alsa/asoundlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define BUFSIZE 2048
#define TOLERANCE 9

void printHelp() {
    std::cout << "Help: " << std::endl;
    std::cout << "stdcdec - open-source cli program to decode inmarsat-C signals using inmarsatc library based on Scytale-C source code" << std::endl;
    std::cout << "Keys: " << std::endl;
    std::cout << "--help                                    - this help" << std::endl;
    std::cout << "--demod                                   - enable demodulator" << std::endl;
    std::cout << "--demod-lo-freq <freq>                    - set demodulator low frequency. default: 500" << std::endl;
    std::cout << "--demod-hi-freq <freq>                    - set demodulator high frequency. default: 4500" << std::endl;
    std::cout << "--demod-cent-freq <freq>                  - set demodulator initial center frequency. default: 2600" << std::endl;
    std::cout << "--demod-stats                             - print demodulator statistics(frequency, etc...)" << std::endl;
    std::cout << "--demod-verbose                           - print demodulated frames as hex" << std::endl;
    std::cout << "--demod-source-file <file-path>           - select audiofile source for demodulator" << std::endl;
    std::cout << "--demod-source-udp <port>                 - select udp source for demodulator(compatible with gqrx). default port: 7355" << std::endl;
    std::cout << "--demod-source-alsa <device>              - select alsa source for demodulator. default device: 'default'" << std::endl;
    std::cout << "--demod-out-udp <ip> <port>               - send demodulated frames via udp to specified ip:port. default: 127.0.0.1:15003" << std::endl;
    std::cout << "--frameparser                             - enable frame parser(if demodulator enabled, it will be automatically selected as source for frame parser)" << std::endl;
    std::cout << "--frameparser-source-udp <port>           - select udp source for frame parser. default port: 15003" << std::endl;
    std::cout << "--frameparser-print-all-packets           - frame parser will print data for all decoded packets(otherwise just message packets)" << std::endl;
    std::cout << "--frameparser-verbose                     - frame parser will print all data for decoded packets" << std::endl;
}

int parseArg(int argc, int* position, char* argv[], std::map<std::string, std::string>* params, bool recursive) {
    std::string arg1 = std::string(argv[*position]);
    //i would be using switch() here... but it's not available for strings, so...
    if(arg1 == "--help") {
        return 1;
    } else if(arg1 == "--demod") {
        params->insert(std::pair<std::string, std::string>("enableDemod", "true"));
        return 0;
    } else if(arg1 == "--demod-stats") {
        params->insert(std::pair<std::string, std::string>("demodStats", "true"));
        return 0;
    } else if(arg1 == "--demod-verbose") {
        params->insert(std::pair<std::string, std::string>("demodVerbose", "true"));
        return 0;
    } else if(arg1 == "--demod-lo-freq") {
        int nextpos = *position + 1;
        if(nextpos > argc or recursive) {
            return 1;
        }
        int parseRes = parseArg(argc, &nextpos, argv, params, true);
        if(parseRes != 2) {
            return 1;
        }
        *position = nextpos;
        std::string arg2 = std::string(argv[nextpos]);
        params->insert(std::pair<std::string, std::string>("demodLoFreq", arg2));
        return 0;
    } else if(arg1 == "--demod-hi-freq") {
        int nextpos = *position + 1;
        if(nextpos > argc or recursive) {
            return 1;
        }
        int parseRes = parseArg(argc, &nextpos, argv, params, true);
        if(parseRes != 2) {
            return 1;
        }
        *position = nextpos;
        std::string arg2 = std::string(argv[nextpos]);
        params->insert(std::pair<std::string, std::string>("demodHiFreq", arg2));
        return 0;
    } else if(arg1 == "--demod-cent-freq") {
        int nextpos = *position + 1;
        if(nextpos > argc or recursive) {
            return 1;
        }
        int parseRes = parseArg(argc, &nextpos, argv, params, true);
        if(parseRes != 2) {
            return 1;
        }
        *position = nextpos;
        std::string arg2 = std::string(argv[nextpos]);
        params->insert(std::pair<std::string, std::string>("demodCentFreq", arg2));
        return 0;
    } else if(arg1 == "--demod-source-file") {
        int nextpos = *position + 1;
        if(nextpos > argc or recursive) {
            return 1;
        }
        int parseRes = parseArg(argc, &nextpos, argv, params, true);
        if(parseRes != 2) {
            return 1;
        }
        *position = nextpos;
        std::string arg2 = std::string(argv[nextpos]);
        params->insert(std::pair<std::string, std::string>("demodSource", "file"));
        params->insert(std::pair<std::string, std::string>("demodSourceFilepath", arg2));
        return 0;
    } else if(arg1 == "--demod-source-udp") {
        std::string arg2;
        arg2 = "7355";
        int nextpos = *position + 1;
        if(nextpos < argc and !recursive) {
            int parseRes = parseArg(argc, &nextpos, argv, params, true);
            if(parseRes == 2) {
                arg2 = std::string(argv[nextpos]);
            }
            *position = nextpos;
        }
        params->insert(std::pair<std::string, std::string>("demodSource", "udp"));
        params->insert(std::pair<std::string, std::string>("demodSourceUdpPort", arg2));
        return 0;
    } else if(arg1 == "--demod-source-alsa") {
        std::string arg2;
        arg2 = "default";
        int nextpos = *position + 1;
        if(nextpos < argc and !recursive) {
            int parseRes = parseArg(argc, &nextpos, argv, params, true);
            if(parseRes == 2) {
                arg2 = std::string(argv[nextpos]);
            }
            *position = nextpos;
        }
        params->insert(std::pair<std::string, std::string>("demodSource", "alsa"));
        params->insert(std::pair<std::string, std::string>("demodSourceAlsaDev", arg2));
        return 0;
    } else if(arg1 == "--demod-out-udp") {
        std::string arg2;
        std::string arg3;
        arg2 = "127.0.0.1";
        arg3 = "15003";
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
        params->insert(std::pair<std::string, std::string>("demodOutUdp", "true"));
        params->insert(std::pair<std::string, std::string>("demodOutUdpIp", arg2));
        params->insert(std::pair<std::string, std::string>("demodOutUdpPort", arg3));
        return 0;
    } else if(arg1 == "--frameparser") {
        params->insert(std::pair<std::string, std::string>("enableFrameParser", "true"));
        if(params->find("enableDemod") != params->end() && params->at("enableDemod") == "true") {
            params->insert(std::pair<std::string, std::string>(("frameParserSource"), "demodulator"));
        }
        return 0;
    } else if(arg1 == "--frameparser-source-udp") {
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
        params->insert(std::pair<std::string, std::string>("frameParserSource", "udp"));
        params->insert(std::pair<std::string, std::string>("frameParserSourceUdpPort", arg2));
        return 0;
    } else if(arg1 == "--frameparser-print-all-packets") {
        params->insert(std::pair<std::string, std::string>("frameParserPrintAllPackets", "true"));
        return 0;
    } else if(arg1 == "--frameparser-verbose") {
        params->insert(std::pair<std::string, std::string>("frameParserVerbose", "true"));
        return 0;
    } else {
        return 2;
    }
}

void sendDemodDataViaUdp(inmarsatc::decoder::Decoder::decoder_result res, int sockfd, sockaddr_in serveraddr) {
    char buf[res.length + sizeof(res.frameNumber)];
    int i;
    for(i = 0;i < res.length;i++) {
        buf[i] = res.decodedFrame[i];
    }
    buf[i++] = (res.frameNumber >> (24));
    buf[i++] = (res.frameNumber >> (16));
    buf[i++] = (res.frameNumber >> (8));
    buf[i] = (res.frameNumber >> (3));
    sendto(sockfd, (const char *)buf, sizeof(buf), 0, (const struct sockaddr *) &serveraddr, sizeof(serveraddr));
}

inmarsatc::decoder::Decoder::decoder_result receiveDemodDataViaUdp(int sockfd, sockaddr_in serveraddr) {
    inmarsatc::decoder::Decoder::decoder_result ret;
    //TODO
    return ret;
}

void printDemodFrameVerbose(inmarsatc::decoder::Decoder::decoder_result frame) {
    std::cout << "demodulated frame:               " << std::endl;
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

bool ifPacketIsMessage(inmarsatc::frameParser::FrameParser::frameParser_result pack_dec_res) {
    switch(pack_dec_res.decoding_result.packetDescriptor) {
        case 0xA3:
        case 0xA8:
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
        std::cout << "packet:                " << std::endl;
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
                if(pack_dec_res.decoding_result.payload.presentation != PACKETDECODER_PRESENTATION_BINARY) {
                    for(int i = 0; i < (int)pack_dec_res.decoding_result.payload.data8Bit.size(); i++) {
                        char chr = pack_dec_res.decoding_result.payload.data8Bit[i];
                        if(chr != '\n') {
                            std::cout << chr;
                        } else {
                            std::cout << std::endl << "         ";
                        }
                    }
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
                        if(pack_dec_res.decoding_result.packetVars.find("stationCount") == pack_dec_res.decoding_result.packetVars.end()) {
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
                        if(pack_dec_res.decoding_result.packetVars.find("shortMessage") == pack_dec_res.decoding_result.packetVars.end()) {
                            std::cout << "  Short message: " << std::endl << "      ";
                            std::string shortMessage = pack_dec_res.decoding_result.packetVars["shortMessage"];
                            for(int k = 0; k < (int)shortMessage.length(); k++) {
                                if(shortMessage[k] != '\n') {
                                    std::cout << shortMessage[k];
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
                        if(pack_dec_res.decoding_result.packetVars.find("shortMessage") == pack_dec_res.decoding_result.packetVars.end()) {
                            std::cout << "  Short message: " << std::endl << "      ";
                            std::string shortMessage = pack_dec_res.decoding_result.packetVars["shortMessage"];
                            for(int k = 0; k < (int)shortMessage.length(); k++) {
                                if(shortMessage[k] != '\n') {
                                    std::cout << shortMessage[k];
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
                        if(!isBinary) {
                            for(int k = 0; k < (int)pack_dec_res.decoding_result.payload.data8Bit.size(); k++) {
                                char c = pack_dec_res.decoding_result.payload.data8Bit[k];
                                if(c != '\n') {
                                    std::cout << c;
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
                                char c = pack_dec_res.decoding_result.payload.data8Bit[k];
                                if(c != '\n') {
                                    std::cout << c;
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
                                char c = pack_dec_res.decoding_result.payload.data8Bit[k];
                                if(c != '\n') {
                                    std::cout << c;
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
    bool isDemodEnabled = params.find("enableDemod") != params.end() && params["enableDemod"] == "true";
    bool isframeParserEnabled = params.find("enableFrameParser") != params.end() && params["enableFrameParser"] == "true";
    bool isDemodStats = params.find("demodStats") != params.end() && params["demodStats"] == "true";
    bool isDemodVerbose = params.find("demodVerbose") != params.end() && params["demodVerbose"] == "true";
    bool isFrameParserPrintAllPackets = params.find("frameParserPrintAllPackets") != params.end() && params["frameParserPrintAllPackets"] == "true";
    bool isFrameParserVerbose = params.find("frameParserVerbose") != params.end() && params["frameParserVerbose"] == "true";
    if(!isDemodEnabled && !isframeParserEnabled) {
        std::cout << "Enable demodulator or frame parser!" << std::endl;
        printHelp();
        return 1;
    }
    if(isDemodEnabled && !isframeParserEnabled) {
        if(params.find("demodSource") == params.end() || params.find("demodOutUdp") == params.end() || params["demodOutUdp"] != "true" || params.find("demodOutUdpIp") == params.end() || params.find("demodOutUdpPort") == params.end()) {
            std::cout << "If you want to use demodulator without frame parer, you have to use UDP out for demodulator!" << std::endl;
            printHelp();
            return 1;
        }
        std::string demodSource = params["demodSource"];
        std::string udpIp = params["demodOutUdpIp"];
        std::string udpPort = params["demodOutUdpPort"]; //if we don't enabled frame parser, only we can do is send through udp
        sockaddr_in clientaddr;
        memset(&clientaddr, 0, sizeof(clientaddr));
        // Filling server information
        clientaddr.sin_family = AF_INET;
        clientaddr.sin_port = htons(std::stoi(udpPort));
        clientaddr.sin_addr.s_addr=inet_addr(udpIp.c_str());
        inmarsatc::demodulator::Demodulator demod;
        if(params.find("demodLoFreq") != params.end()) {
            int loFreq = std::atoi(params["demodLoFreq"].c_str());
            demod.setLowFreq(loFreq);
        }
        if(params.find("demodHiFreq") != params.end()) {
            int hiFreq = std::atoi(params["demodHiFreq"].c_str());
            demod.setHighFreq(hiFreq);
        }
        if(params.find("demodCentFreq") != params.end()) {
            int centFreq = std::atoi(params["demodCentFreq"].c_str());
            demod.setCenterFreq(centFreq);
        }
        inmarsatc::decoder::Decoder decoder(TOLERANCE);
        int sockfd;
        if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
            std::cout << "Socket creation failed!" << std::endl;
            return 1;
        }
        if(demodSource == "file") {
            if(params.find("demodSourceFilepath") == params.end()) {
                std::cout << "File path not specified!" << std::endl;
                return 1;
            }
            std::string filePath = params["demodSourceFilepath"];
            AFfilehandle file = afOpenFile(filePath.c_str(), "r", AF_NULL_FILESETUP);
            int channels = afGetChannels(file, AF_DEFAULT_TRACK);
            double rate = afGetRate(file, AF_DEFAULT_TRACK);
            afSetVirtualSampleFormat(file, AF_DEFAULT_TRACK, AF_SAMPFMT_TWOSCOMP, 16);
            if(channels != 1 or rate != 48000) {
                std::cout << "Wrong file format! It should be 48k, 1 channel." << std::endl;
                return 1;
            }
            int16_t buf[BUFSIZE];
            AFframecount framesRead;
            while(true) {
                framesRead = afReadFrames(file, AF_DEFAULT_TRACK, buf, BUFSIZE);
                if(framesRead <= 0) {
                    return 0;
                }
                std::complex<double> cbuf[framesRead];
                for(int i = 0; i < framesRead; i+= 1) {
                    double val = buf[i];
                    cbuf[i] = std::complex<double>(val,val);
                }
                std::vector<inmarsatc::demodulator::Demodulator::demodulator_result> res = demod.demodulate(cbuf, framesRead);
                if(isDemodStats) {
                    std::cout << "freq = " << demod.getCenterFreq() << " sync = " << (demod.getIsInSync() ? "true" : "false") << "     \r" << std::flush;
                }
                if(res.size() > 0) {
                    for(int d = 0; d < (int)res.size(); d++) {
                        std::vector<inmarsatc::decoder::Decoder::decoder_result> dec_res = decoder.decode(res[d].bitsDemodulated);
                        for(int i = 0; i < (int)dec_res.size(); i++) {
                            if(isDemodVerbose) {
                                printDemodFrameVerbose(dec_res[i]);
                            }
                            sendDemodDataViaUdp(dec_res[i], sockfd, clientaddr);
                        }
                    }
                }
            }
        } else if(demodSource == "udp") {
            if(params.find("demodSourceUdpPort") == params.end()) {
                std::cout << "Udp port not specified!" << std::endl;
                return 1;
            }
            std::string udpPort = params["demodSourceUdpPort"];
            int sockfd;
            if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
                std::cout << "Socket creation failed!" << std::endl;
                return 1;
            }
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
            if (bind(sockfd, (const struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0) {
                    std::cout << "Binding to port failed!" << std::endl;
                    return 1;
            }
            int16_t buf[BUFSIZE];
            int received;
            socklen_t len_useless = sizeof(cliaddr);
            while(true) {
                received = recvfrom(sockfd, (char *)buf, (BUFSIZE*2), MSG_WAITALL, ( struct sockaddr *) &cliaddr, &len_useless);
                received = received / 2;//char to int16_t
                if(received <= 0) {
                    return 0;
                }
                std::complex<double> cbuf[received];
                for(int i = 0; i < received; i+= 1) {
                    double val = buf[i];
                    cbuf[i] = std::complex<double>(val,val);
                }
                std::vector<inmarsatc::demodulator::Demodulator::demodulator_result> res = demod.demodulate(cbuf, received);
                if(isDemodStats) {
                    std::cout << "freq = " << demod.getCenterFreq() << " sync = " << (demod.getIsInSync() ? "true" : "false") << "     \r" << std::flush;
                }
                if(res.size() > 0) {
                    for(int d = 0; d < (int)res.size(); d++) {
                        std::vector<inmarsatc::decoder::Decoder::decoder_result> dec_res = decoder.decode(res[d].bitsDemodulated);
                        for(int i = 0; i < (int)dec_res.size(); i++) {
                            if(isDemodVerbose) {
                                printDemodFrameVerbose(dec_res[i]);
                            }
                            sendDemodDataViaUdp(dec_res[i], clisockfd, clientaddr);
                        }
                    }
                }
            }
        } else if(demodSource == "alsa") {
            if(params.find("demodSourceAlsaDev") == params.end()) {
                std::cout << "Udp port not specified!" << std::endl;
                return 1;
            }
            std::string alsaDev = params["demodSourceAlsaDev"];
            int err;
            unsigned int rate = 48000;
            snd_pcm_t *capture_handle;
            snd_pcm_hw_params_t *hw_params;
            snd_pcm_format_t format = SND_PCM_FORMAT_S16_LE;
            if ((err = snd_pcm_open (&capture_handle, alsaDev.c_str(), SND_PCM_STREAM_CAPTURE, 0)) < 0) {
                fprintf (stderr, "cannot open audio device %s (%s)\n", "", snd_strerror (err));
                exit (1);
            }
            if ((err = snd_pcm_hw_params_malloc (&hw_params)) < 0) {
                fprintf (stderr, "cannot allocate hardware parameter structure (%s)\n", snd_strerror (err));
                exit (1);
            }
            if ((err = snd_pcm_hw_params_any (capture_handle, hw_params)) < 0) {
                fprintf (stderr, "cannot initialize hardware parameter structure (%s)\n", snd_strerror (err));
                exit (1);
            }
            if ((err = snd_pcm_hw_params_set_access (capture_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
                fprintf (stderr, "cannot set access type (%s)\n", snd_strerror (err));
                exit (1);
            }
            if ((err = snd_pcm_hw_params_set_format (capture_handle, hw_params, format)) < 0) {
                fprintf (stderr, "cannot set sample format (%s)\n", snd_strerror (err));
                exit (1);
            }
            if ((err = snd_pcm_hw_params_set_rate_near (capture_handle, hw_params, &rate, 0)) < 0) {
                fprintf (stderr, "cannot set sample rate (%s)\n", snd_strerror (err));
                exit (1);
            }
            if ((err = snd_pcm_hw_params_set_channels (capture_handle, hw_params, 1)) < 0) {
                fprintf (stderr, "cannot set channel count (%s)\n", snd_strerror (err));
                exit (1);
            }
            if ((err = snd_pcm_hw_params (capture_handle, hw_params)) < 0) {
                fprintf (stderr, "cannot set parameters (%s)\n", snd_strerror (err));
                exit (1);
            }
            snd_pcm_hw_params_free (hw_params);
            if ((err = snd_pcm_prepare (capture_handle)) < 0) {
                fprintf (stderr, "cannot prepare audio interface for use (%s)\n", snd_strerror (err));
                exit (1);
            }
            int framesRead;
            int16_t buf[BUFSIZE];
            while(true) {
                framesRead = (snd_pcm_readi (capture_handle, buf, BUFSIZE));
                if(framesRead <= 0) {
                    break;
                }
                std::complex<double> cbuf[framesRead];
                for(int i = 0; i < framesRead; i+= 1) {
                    double val = buf[i];
                    cbuf[i] = std::complex<double>(val,val);
                }
                std::vector<inmarsatc::demodulator::Demodulator::demodulator_result> res = demod.demodulate(cbuf, framesRead);
                if(isDemodStats) {
                    std::cout << "freq = " << demod.getCenterFreq() << " sync = " << (demod.getIsInSync() ? "true" : "false") << "     \r" << std::flush;
                }
                if(res.size() > 0) {
                    for(int d = 0; d < (int)res.size(); d++) {
                        std::vector<inmarsatc::decoder::Decoder::decoder_result> dec_res = decoder.decode(res[d].bitsDemodulated);
                        for(int i = 0; i < (int)dec_res.size(); i++) {
                            if(isDemodVerbose) {
                                printDemodFrameVerbose(dec_res[i]);
                            }
                            sendDemodDataViaUdp(dec_res[i], sockfd, clientaddr);
                        }
                    }
                }
            }
            snd_pcm_close (capture_handle);
        } else {
            std::cout << "Wrong or none demodulator source!" << std::endl;
            return 1;
        }
    } else if(isDemodEnabled && isframeParserEnabled) {
        if(params.find("demodSource") == params.end()) {
            std::cout << "Demodulator source not selected!" << std::endl;
            printHelp();
            return 1;
        }
        inmarsatc::demodulator::Demodulator demod;
        if(params.find("demodLoFreq") != params.end()) {
            int loFreq = std::atoi(params["demodLoFreq"].c_str());
            demod.setLowFreq(loFreq);
        }
        if(params.find("demodHiFreq") != params.end()) {
            int hiFreq = std::atoi(params["demodHiFreq"].c_str());
            demod.setHighFreq(hiFreq);
        }
        if(params.find("demodCentFreq") != params.end()) {
            int centFreq = std::atoi(params["demodCentFreq"].c_str());
            demod.setCenterFreq(centFreq);
        }
        inmarsatc::decoder::Decoder decoder(TOLERANCE);
        inmarsatc::frameParser::FrameParser frameParser;
        int demodUdpOutSockFd;
        sockaddr_in demodUdpOutClientAddr;
        bool isDemodUdpOutEnabled = params.find("demodOutUdp") != params.end() && params["demodOutUdp"] == "true";
        if(isDemodUdpOutEnabled) {
            if(params.find("demodOutUdpIp") == params.end() || params.find("demodOutUdpPort") == params.end()) {
                std::cout << "Demodulator ip/port not specified(internal error)!" << std::endl;
                return 1;
            }
            std::string udpIp = params["demodOutUdpIp"];
            std::string udpPort = params["demodOutUdpPort"];
            memset(&demodUdpOutClientAddr, 0, sizeof(demodUdpOutClientAddr));
            // Filling server information
            demodUdpOutClientAddr.sin_family = AF_INET;
            demodUdpOutClientAddr.sin_port = htons(std::stoi(udpPort));
            demodUdpOutClientAddr.sin_addr.s_addr=inet_addr(udpIp.c_str());
            if ((demodUdpOutSockFd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
                std::cout << "Socket creation failed!" << std::endl;
                return 1;
            }
        }
        std::string demodSource = params["demodSource"];
        if(demodSource == "file") {
            if(params.find("demodSourceFilepath") == params.end()) {
                std::cout << "File path not specified!" << std::endl;
                return 1;
            }
            std::string filePath = params["demodSourceFilepath"];
            AFfilehandle file = afOpenFile(filePath.c_str(), "r", AF_NULL_FILESETUP);
            int channels = afGetChannels(file, AF_DEFAULT_TRACK);
            double rate = afGetRate(file, AF_DEFAULT_TRACK);
            afSetVirtualSampleFormat(file, AF_DEFAULT_TRACK, AF_SAMPFMT_TWOSCOMP, 16);
            if(channels != 1 or rate != 48000) {
                std::cout << "Wrong file format! It should be 48k, 1 channel." << std::endl;
                return 1;
            }
            int16_t buf[BUFSIZE];
            AFframecount framesRead;
            while(true) {
                framesRead = afReadFrames(file, AF_DEFAULT_TRACK, buf, BUFSIZE);
                if(framesRead <= 0) {
                    return 0;
                }
                std::complex<double> cbuf[framesRead];
                for(int i = 0; i < framesRead; i+= 1) {
                    double val = buf[i];
                    cbuf[i] = std::complex<double>(val,val);
                }
                std::vector<inmarsatc::demodulator::Demodulator::demodulator_result> res = demod.demodulate(cbuf, framesRead);
                if(isDemodStats) {
                    std::cout << "freq = " << demod.getCenterFreq() << " sync = " << (demod.getIsInSync() ? "true" : "false") << "     \r" << std::flush;
                }
                if(res.size() > 0) {
                    for(int d = 0; d < (int)res.size(); d++) {
                        std::vector<inmarsatc::decoder::Decoder::decoder_result> dec_res = decoder.decode(res[d].bitsDemodulated);
                        for(int i = 0; i < (int)dec_res.size(); i++) {
                            if(isDemodVerbose) {
                                printDemodFrameVerbose(dec_res[i]);
                            }
                            if(isDemodUdpOutEnabled) {
                                sendDemodDataViaUdp(dec_res[i], demodUdpOutSockFd, demodUdpOutClientAddr);
                            }
                            std::vector<inmarsatc::frameParser::FrameParser::frameParser_result> pack_dec_res_vec = frameParser.parseFrame(dec_res[i]);
                            for(int k = 0; k < (int)pack_dec_res_vec.size(); k++) {
                                inmarsatc::frameParser::FrameParser::frameParser_result pack_dec_res = pack_dec_res_vec[k];
                                if(!pack_dec_res.decoding_result.isDecodedPacket || !pack_dec_res.decoding_result.isCrc) {
                                    continue;
                                }
                                printFrameParserPacket(pack_dec_res, isFrameParserPrintAllPackets, isFrameParserVerbose);
                            }
                        }
                    }
                }
            }
        } else if(demodSource == "udp") {
            if(params.find("demodSourceUdpPort") == params.end()) {
                std::cout << "Udp port not specified!" << std::endl;
                return 1;
            }
            std::string udpPort = params["demodSourceUdpPort"];
            int sockfd;
            if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
                std::cout << "Socket creation failed!" << std::endl;
                return 1;
            }
            // Filling server information
            sockaddr_in serveraddr, cliaddr;
            serveraddr.sin_family = AF_INET;
            serveraddr.sin_port = htons(std::stoi(udpPort));
            serveraddr.sin_addr.s_addr=INADDR_ANY;
            if (bind(sockfd, (const struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0) {
                    std::cout << "Binding to port failed!" << std::endl;
                    return 1;
            }
            int16_t buf[BUFSIZE];
            int received;
            socklen_t len_useless = sizeof(cliaddr);
            while(true) {
                received = recvfrom(sockfd, (char *)buf, (BUFSIZE*2), MSG_WAITALL, ( struct sockaddr *) &cliaddr, &len_useless);
                received = received / 2;//char to int16_t
                if(received <= 0) {
                    return 0;
                }
                std::complex<double> cbuf[received];
                for(int i = 0; i < received; i+= 1) {
                    double val = buf[i];
                    cbuf[i] = std::complex<double>(val,val);
                }
                std::vector<inmarsatc::demodulator::Demodulator::demodulator_result> res = demod.demodulate(cbuf, received);
                if(isDemodStats) {
                    std::cout << "freq = " << demod.getCenterFreq() << " sync = " << (demod.getIsInSync() ? "true" : "false") << "     \r" << std::flush;
                }
                if(res.size() > 0) {
                    for(int d = 0; d < (int)res.size(); d++) {
                        std::vector<inmarsatc::decoder::Decoder::decoder_result> dec_res = decoder.decode(res[d].bitsDemodulated);
                        for(int i = 0; i < (int)dec_res.size(); i++) {
                            if(isDemodVerbose) {
                                printDemodFrameVerbose(dec_res[i]);
                            }
                            if(isDemodUdpOutEnabled) {
                                sendDemodDataViaUdp(dec_res[i], demodUdpOutSockFd, demodUdpOutClientAddr);
                            }
                            std::vector<inmarsatc::frameParser::FrameParser::frameParser_result> pack_dec_res_vec = frameParser.parseFrame(dec_res[i]);
                            for(int k = 0; k < (int)pack_dec_res_vec.size(); k++) {
                                inmarsatc::frameParser::FrameParser::frameParser_result pack_dec_res = pack_dec_res_vec[k];
                                if(!pack_dec_res.decoding_result.isDecodedPacket || !pack_dec_res.decoding_result.isCrc) {
                                    continue;
                                }
                                printFrameParserPacket(pack_dec_res, isFrameParserPrintAllPackets, isFrameParserVerbose);
                            }
                        }
                    }
                }
            }
        } else if(demodSource == "alsa") {
            if(params.find("demodSourceAlsaDev") == params.end()) {
                std::cout << "Udp port not specified!" << std::endl;
                return 1;
            }
            std::string alsaDev = params["demodSourceAlsaDev"];
            int err;
            unsigned int rate = 48000;
            snd_pcm_t *capture_handle;
            snd_pcm_hw_params_t *hw_params;
            snd_pcm_format_t format = SND_PCM_FORMAT_S16_LE;
            if ((err = snd_pcm_open (&capture_handle, alsaDev.c_str(), SND_PCM_STREAM_CAPTURE, 0)) < 0) {
                fprintf (stderr, "cannot open audio device %s (%s)\n", "", snd_strerror (err));
                exit (1);
            }
            if ((err = snd_pcm_hw_params_malloc (&hw_params)) < 0) {
                fprintf (stderr, "cannot allocate hardware parameter structure (%s)\n", snd_strerror (err));
                exit (1);
            }
            if ((err = snd_pcm_hw_params_any (capture_handle, hw_params)) < 0) {
                fprintf (stderr, "cannot initialize hardware parameter structure (%s)\n", snd_strerror (err));
                exit (1);
            }
            if ((err = snd_pcm_hw_params_set_access (capture_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
                fprintf (stderr, "cannot set access type (%s)\n", snd_strerror (err));
                exit (1);
            }
            if ((err = snd_pcm_hw_params_set_format (capture_handle, hw_params, format)) < 0) {
                fprintf (stderr, "cannot set sample format (%s)\n", snd_strerror (err));
                exit (1);
            }
            if ((err = snd_pcm_hw_params_set_rate_near (capture_handle, hw_params, &rate, 0)) < 0) {
                fprintf (stderr, "cannot set sample rate (%s)\n", snd_strerror (err));
                exit (1);
            }
            if ((err = snd_pcm_hw_params_set_channels (capture_handle, hw_params, 1)) < 0) {
                fprintf (stderr, "cannot set channel count (%s)\n", snd_strerror (err));
                exit (1);
            }
            if ((err = snd_pcm_hw_params (capture_handle, hw_params)) < 0) {
                fprintf (stderr, "cannot set parameters (%s)\n", snd_strerror (err));
                exit (1);
            }
            snd_pcm_hw_params_free (hw_params);
            if ((err = snd_pcm_prepare (capture_handle)) < 0) {
                fprintf (stderr, "cannot prepare audio interface for use (%s)\n", snd_strerror (err));
                exit (1);
            }
            int framesRead;
            int16_t buf[BUFSIZE];
            while(true) {
                framesRead = (snd_pcm_readi (capture_handle, buf, BUFSIZE));
                if(framesRead <= 0) {
                    break;
                }
                std::complex<double> cbuf[framesRead];
                for(int i = 0; i < framesRead; i+= 1) {
                    double val = buf[i];
                    cbuf[i] = std::complex<double>(val,val);
                }
                std::vector<inmarsatc::demodulator::Demodulator::demodulator_result> res = demod.demodulate(cbuf, framesRead);
                if(isDemodStats) {
                    std::cout << "freq = " << demod.getCenterFreq() << " sync = " << (demod.getIsInSync() ? "true" : "false") << "     \r" << std::flush;
                }
                if(res.size() > 0) {
                    for(int d = 0; d < (int)res.size(); d++) {
                        std::vector<inmarsatc::decoder::Decoder::decoder_result> dec_res = decoder.decode(res[d].bitsDemodulated);
                        for(int i = 0; i < (int)dec_res.size(); i++) {
                            if(isDemodVerbose) {
                                printDemodFrameVerbose(dec_res[i]);
                            }
                            if(isDemodUdpOutEnabled) {
                                sendDemodDataViaUdp(dec_res[i], demodUdpOutSockFd, demodUdpOutClientAddr);
                            }
                            std::vector<inmarsatc::frameParser::FrameParser::frameParser_result> pack_dec_res_vec = frameParser.parseFrame(dec_res[i]);
                            for(int k = 0; k < (int)pack_dec_res_vec.size(); k++) {
                                inmarsatc::frameParser::FrameParser::frameParser_result pack_dec_res = pack_dec_res_vec[k];
                                if(!pack_dec_res.decoding_result.isDecodedPacket || !pack_dec_res.decoding_result.isCrc) {
                                    continue;
                                }
                                printFrameParserPacket(pack_dec_res, isFrameParserPrintAllPackets, isFrameParserVerbose);
                            }
                        }
                    }
                }
            }
            snd_pcm_close (capture_handle);
        } else {
            std::cout << "Wrong or none demodulator source!" << std::endl;
            return 1;
        }
    } else { //!isDemodEnabled && isframeParserEnabled
        std::cout << "Sorry, not implemented at the moment :(" << std::endl;
        return 1;
    }
    return 0;
}

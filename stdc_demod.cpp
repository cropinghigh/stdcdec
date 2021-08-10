#include <iostream>
#include <inmarsatc_demodulator.h>
#include <audiofile.h>
#include <alsa/asoundlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <map>

#define BUFSIZE 2048

void printHelp() {
    std::cout << "Help: " << std::endl;
    std::cout << "stdc_demod - open-source cli program to demodulate inmarsat-C signals using inmarsatc library based on Scytale-C source code" << std::endl;
    std::cout << "Keys: " << std::endl;
    std::cout << "--help                                    - this help" << std::endl;
    std::cout << "--lo-freq <freq>                          - set demodulator low frequency. default: 500" << std::endl;
    std::cout << "--hi-freq <freq>                          - set demodulator high frequency. default: 4500" << std::endl;
    std::cout << "--cent-freq <freq>                        - set demodulator initial center frequency. default: 2600" << std::endl;
    std::cout << "--stats                                   - print demodulator statistics(frequency, etc...)" << std::endl;
    std::cout << "--source-file <file-path>                 - select audiofile source for demodulator" << std::endl;
    std::cout << "--source-udp <port>                       - select udp source for demodulator(compatible with gqrx). default port: 7355" << std::endl;
    std::cout << "--source-alsa <device>                    - select alsa source for demodulator. default device: 'default'" << std::endl;
    std::cout << "--out-udp <ip> <port>                     - send demodulated symbols via udp to specified ip:port. default: 127.0.0.1:15003" << std::endl;
    std::cout << "(one source and one out parameters should be selected)" << std::endl;
}

int parseArg(int argc, int* position, char* argv[], std::map<std::string, std::string>* params, bool recursive) {
    std::string arg1 = std::string(argv[*position]);
    //i would be using switch() here... but it's not available for strings, so...
    if(arg1 == "--help") {
        return 1;
    } else if(arg1 == "--stats") {
        params->insert(std::pair<std::string, std::string>("demodStats", "true"));
        return 0;
    } else if(arg1 == "--lo-freq") {
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
    } else if(arg1 == "--hi-freq") {
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
    } else if(arg1 == "--cent-freq") {
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
    } else if(arg1 == "--source-file") {
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
    } else if(arg1 == "--source-udp") {
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
    } else if(arg1 == "--source-alsa") {
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
    } else if(arg1 == "--out-udp") {
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
    } else {
        return 2;
    }
}

void sendDemodSymbolsViaUdp(uint8_t data[DEMODULATOR_SYMBOLSPERCHUNK], int sockfd, sockaddr_in serveraddr) {
    sendto(sockfd, (const char *)data, DEMODULATOR_SYMBOLSPERCHUNK, 0, (const struct sockaddr *) &serveraddr, sizeof(serveraddr));
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
    bool isDemodStats = params.find("demodStats") != params.end() && params["demodStats"] == "true";
    if(params.find("demodSource") == params.end() || params.find("demodOutUdp") == params.end() || params["demodOutUdp"] != "true" || params.find("demodOutUdpIp") == params.end() || params.find("demodOutUdpPort") == params.end()) {
        std::cout << "Wrong/No source/out selected!" << std::endl;
        printHelp();
        return 1;
    }
    std::string demodSource = params["demodSource"];
    std::string udpIp = params["demodOutUdpIp"];
    std::string udpPort = params["demodOutUdpPort"];
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
                    sendDemodSymbolsViaUdp(res[d].bitsDemodulated, sockfd, clientaddr);
                }
            }
        }
    } else if(demodSource == "udp") {
        if(params.find("demodSourceUdpPort") == params.end()) {
            std::cout << "Udp port not specified!" << std::endl;
            return 1;
        }
        std::string udpPort = params["demodSourceUdpPort"];
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
        int16_t buf[BUFSIZE];
        int received;
        socklen_t len_useless = sizeof(cliaddr);
        while(true) {
            received = recvfrom(clisockfd, (char *)buf, (BUFSIZE*2), MSG_WAITALL, ( struct sockaddr *) &cliaddr, &len_useless);
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
                    sendDemodSymbolsViaUdp(res[d].bitsDemodulated, sockfd, clientaddr);
                }
            }
        }
    } else if(demodSource == "alsa") {
        if(params.find("demodSourceAlsaDev") == params.end()) {
            std::cout << "Alsa device not specified!" << std::endl;
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
                    sendDemodSymbolsViaUdp(res[d].bitsDemodulated, sockfd, clientaddr);
                }
            }
        }
        snd_pcm_close (capture_handle);
    } else {
        std::cout << "Wrong or none demodulator source!" << std::endl;
        return 1;
    }
}

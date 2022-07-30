# stdcdec
Set of programs to receive Inmarsat-C signals

Related projects:

    inmarsatc: library with all functions to receive Inmarsat-C signals
    https://github.com/cropinghigh/inmarsatc

    qstdcdec: qt version of stdcdec_parser
    https://github.com/cropinghigh/qstdcdec

    sdrpp-inmarsatc-demodulator: SDR++ module, which replaces stdcdec_demod and stdcdec_decoder
    https://github.com/cropinghigh/sdrpp-inmarsatc-demodulator

Building:

  0.  If you have an arch-like system, just install stdcdec-git with all dependencies

  1.  Install inmarsatc library(libinmarsatc-git for arch-like systems)

  2.  Build

          mkdir build
          cd build
          cmake ..
          make
          sudo make install

Usage:

  1.  Run stdc_demod to demodulate symbols from audio stream(you need additional program to move RF signal to audio zone, like SDR++ or gqrx, Inmarsat-C signal are usually around 1539MHz).

      Available arguments:

          --lo-freq <freq>           - set the minimum audio frequency in Hz where demodulator will search for the signal, default=500Hz
          --hi-freq <freq>           - set the maximum audio frequency in Hz where demodulator will search for the signal, default=4500Hz
          --cent-freq <freq>         - set the initial audio center frequency in Hz to tune demodulator to, default=2600Hz; Because demodulator is not very good, it requires to be set quite precisely and a bit higher than actual signal center frequency(~100 Hz)
          --stats                    - demodulator will print statistics(frequency and lock status). Useful for tuning
          --source-file <file path>  - use the audio file as the source for the demodulator
          --source-udp <port>        - receive audio samples via udp. Compatible with gqrx, default argument=7355
          --source-alsa <device>     - read audio samples from specified alsa device, default argument=default
          --out-udp <ip> <port>      - send demodulated symbols to specified ip and port, default arguments=127.0.0.1 15003

      Note that exactly one source and one out arguments should be used.

  2.  Run stdc_decoder to decode symbols to get the frames

      Available arguments:

          --verbose              - print all frames to the stdout, useful for tuning
          --in-udp <port>        - receive demodulated symbols via udp, default argument=15003
          --out-udp <ip> <port>  - send decoded frames to specified ip and port, default arguments=127.0.0.1 15004

      Note that exactly one source and one out arguments should be used.

  3.  Run stdc_parser to extract packets and messages from the frames

      Available arguments:

          --verbose              - print all data for all parsed packets
          --print-all-packets    - print all packets, not only messages
          --in-udp <port>        - receive decoded frames via udp, default argument=15003(should be changed to 15004)
          --out-udp <ip> <port>  - send parsed packets to specified ip and port in JSON, default arguments=127.0.0.1 15005

      Note that exactly one source argument should be used


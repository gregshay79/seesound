// ---------------------------------------------------------------------
// pkt.h  --  1200/300/2400 baud AX.25
//
//
// This file is a proposed part of fldigi.  Adapted very liberally from 
// rtty.h, with many thanks to John Hansen, W2FS, who wrote 
// 'dcc.doc' and 'dcc2.doc', GNU Octave, GNU Radio Companion, and finally
// Bartek Kania (bk.gnarf.org) whose 'aprs.c' expository coding style helped
// shape this implementation.
//
// Copyright (C) 2010
//    Dave Freese, W1HKJ
//    Chris Sylvain, KB3CS
//
// fldigi is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// fldigi is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with fldigi; if not, write to the Free Software
// Foundation, Inc.
// 59 Temple Place
// Suite 330
// Boston, MA  02111-1307  USA
// ---------------------------------------------------------------------

#ifndef PKT_H
#define PKT_H

#include "complex.h"
#include "modem.h"
#include "globals.h"
//#include "debug.h"
//#include "filters.h"
//#include "fftfilt.h"
#include "digiscope.h"
#include "waterfall.h"
#include "trx.h"
#include "nco.h"

#define	PKT_SampleRate 12000

#define PKT_MinBaudRate  300
#define PKT_MaxSymbolLen (PKT_SampleRate / PKT_MinBaudRate)

#define PKT_DetectLen	 16
#define PKT_SyncDispLen	 24
#define PKT_IdleLen	120

#define PKT_MinSignalPwr 0.1

// is adjval too small? maybe 0.25 ? too large?
#define PKT_PLLAdjVal	0.1

// special and unique start+end of AX.25 frame symbol
#define PKT_Flag	0x7e
// number of mark bits prior to start of frame at 1200 baud
#define PKT_MarkBits	320
// number of flags used indicate start or end of frame at 1200 baud
#define PKT_StartFlags	24
#define PKT_EndFlags	12

// 70 bytes addr + 256 payload + 2 FCS + 1 Control + 1 Protocol ID
#define MAXOCTETS 340
// 136 bits minimum (including start and end flags) - AX.25 v2.2 section 3.9
//  == 15 octets.  we count only one of the two flags, though.
#define MINOCTETS  14

enum PKT_RX_STATE {
    PKT_RX_STATE_IDLE = 0,
    PKT_RX_STATE_DROP,
    PKT_RX_STATE_START,
    PKT_RX_STATE_DATA,
    PKT_RX_STATE_STOP
};

enum PKT_MicE_field {
    Null = 0x00,
    Space = 0x20,
    Zero = 0x30,
    One,
    Two,
    Three,
    Four,
    Five,
    Six,
    Seven,
    Eight,
    Nine,
    P0 = 0x40,
    P100,
    North = 0x50,
    East,
    South,
    West,
    Invalid = 0xFF
};

struct PKT_PHG_table {
    const char *s;
    unsigned char l;
};

class pkt : public modem {
 public:
    static const double CENTER[];
    static const double SHIFT[];
    static const int	BAUD[];
    static const int	BITS[];

 private:
    int symbollen;

    int pkt_stoplen, pkt_detectlen;
    int pkt_syncdisplen, pkt_idlelen;
    int pkt_startlen;

    double	pkt_shift;
    int		pkt_baud;
    int		pkt_nbits;
    double	pkt_ctrfreq;
    double	pkt_squelch;
    double	pkt_BW;
    bool 	pkt_reverse;

    int		scounter; // audio sample counter
    double	lo_signal_gain, hi_signal_gain;
    NCO		*nco_lo, *nco_hi, *nco_mid;

    PKT_RX_STATE rxstate;

    double	idle_signal_pwr, *idle_signal_buf;
    int		idle_signal_buf_ptr;

    void	idle_signal_power(double sample);

    cmplx	lo_signal_energy, *lo_signal_buf;
    cmplx	hi_signal_energy, *hi_signal_buf;
    cmplx	mid_signal_energy, *mid_signal_buf;
    double	lo_signal_corr, hi_signal_corr, mid_signal_corr;
    int		correlate_buf_ptr;

    double	signal_gain, signal_pwr, *signal_buf;
    int		signal_buf_ptr;

    double *pipe, *dsppipe;  // SyncScope
    int pipeptr;

    // SCBLOCKSIZE * 2 = 1024 ( == MAX_ZLEN )
    cmplx QI[MAX_ZLEN];  // PhaseScope
    int QIptr;

    double yt_avg;
    bool   clear_zdata;

    double      signal_power, noise_power, power_ratio, snr_avg;

    double	corr_power(cmplx v);
    void	correlate(double sample);

    int		detect_drop;
    void 	detect_signal();

    double	lo_sync, hi_sync, mid_symbol;
    int		lo_sync_ptr, hi_sync_ptr;
    bool	prev_symbol, pll_symbol;
    void	do_sync();

    int		seq_ones; // number of sequential ones in data

    int		select_val;
    void	set_pkt_modem_params(int i);

    void	rx_data();
    void	rx(bool bit);

    void Metric();

    unsigned char bitreverse(unsigned char in, int n);

    unsigned char rxbuf[MAXOCTETS+4], *cbuf;

    unsigned int computeFCS(unsigned char *h, unsigned char *t);
    bool	checkFCS(unsigned char *cp);

    void do_put_rx_char(unsigned char *cp);

    void clear_syncscope();
    void update_syncscope();


    //    inline cmplx mixer(cmplx in);

    // MicE encodings:
    //   3 char groups,
    //     max 12 char values per group,
    //       5 encodings per char value
    static PKT_MicE_field	MicE_table[][12][5];
    void	expand_MicE(unsigned char *I, unsigned char *E);

    static PKT_PHG_table	PHG_table[];
    void	expand_PHG(unsigned char *I);

    void	expand_Cmp(unsigned char *I);

// transmit	

    int		preamble, pretone, postamble;
    double	lo_txgain, hi_txgain;
    NCO		*lo_tone, *hi_tone;

    void	send_symbol(bool bit);

    int		tx_char_count, nr_ones;
    bool	currbit, nostuff, did_pkt_head;
    void	send_char(unsigned char c);

    unsigned char txbuf[MAXOCTETS+4], *tx_cbuf;
    void	send_msg(unsigned char c);

 public:
    pkt(trx_mode mode);
    ~pkt();
    void init();
    void rx_init();
    void tx_init(SoundBase *sc);
    void restart();
    int rx_process(const double *buf, int len);
    int tx_process();

    void set_freq(double);
};

#endif

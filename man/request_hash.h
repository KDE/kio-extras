#ifndef RequestHash_H
#define RequestHash_H

/* This file is part of the KDE libraries
   Copyright (C) 2011 Martin Koller <kollix@aon.at>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

// Some known missing requests from mdoc(7):
// - start or end of quotings

// Some of the requests are from mdoc.
// On Linux see the man pages mdoc(7), mdoc.samples(7) and groff_mdoc(7)
// See also the online man pages of FreeBSD: mdoc(7)

enum RequestNum
{
  REQ_UNKNOWN = -1,
  REQ_ab,
  REQ_di,
  REQ_ds,
  REQ_as,
  REQ_br,
  REQ_c2,
  REQ_cc,
  REQ_ce,
  REQ_ec,
  REQ_eo,
  REQ_ex,
  REQ_fc,
  REQ_fi,
  REQ_ft,  // groff(7) "FonT"
  REQ_el,
  REQ_ie,
  REQ_if,
  REQ_ig,
  REQ_nf,
  REQ_ps,
  REQ_sp,
  REQ_so,
  REQ_ta,
  REQ_ti,
  REQ_tm,
  REQ_B,
  REQ_I,
  REQ_Fd,
  REQ_Fn,
  REQ_Fo,
  REQ_Fc,
  REQ_OP,
  REQ_Ft,
  REQ_Fa,
  REQ_BR,
  REQ_BI,
  REQ_IB,
  REQ_IR,
  REQ_RB,
  REQ_RI,
  REQ_DT,
  REQ_IP, // man(7) "Indent Paragraph"
  REQ_TP,
  REQ_IX,
  REQ_P,
  REQ_LP,
  REQ_PP,
  REQ_HP,
  REQ_PD,
  REQ_Rs,
  REQ_RS,
  REQ_Re,
  REQ_RE,
  REQ_SB,
  REQ_SM,
  REQ_Ss,
  REQ_SS,
  REQ_Sh,
  REQ_SH, // man(7) "Sub Header"
  REQ_Sx,
  REQ_TS,
  REQ_Dt,
  REQ_TH,
  REQ_TX,
  REQ_rm,
  REQ_rn,
  REQ_nx,
  REQ_in,
  REQ_nr, // groff(7) "Number Register"
  REQ_am,
  REQ_de,
  REQ_de1, // groff(7) Same as .de but with compatibility mode switched off during macro expansion.
  REQ_Bl, // mdoc(7) "Begin List"
  REQ_El, // mdoc(7) "End List"
  REQ_It, // mdoc(7) "ITem"
  REQ_Bk,
  REQ_Ek,
  REQ_Dd,
  REQ_Os, // mdoc(7)
  REQ_Bt,
  REQ_At, // mdoc(7) "AT&t" (not parsable, not callable)
  REQ_Fx, // mdoc(7) "Freebsd" (not parsable, not callable)
  REQ_Nx,
  REQ_Ox,
  REQ_Bx, // mdoc(7) "Bsd"
  REQ_Ux, // mdoc(7) "UniX"
  REQ_Dl,
  REQ_Bd,
  REQ_Ed,
  REQ_Be,
  REQ_Xr, // mdoc(7) "eXternal Reference"
  REQ_Fl, // mdoc(7) "FLag"
  REQ_Pa,
  REQ_Pf,
  REQ_Pp,
  REQ_Dq, // mdoc(7) "Double Quote"
  REQ_Op,
  REQ_Oo,
  REQ_Oc,
  REQ_Pq, // mdoc(7) "Parenthese Quote"
  REQ_Ql,
  REQ_Sq, // mdoc(7) "Single Quote"
  REQ_Ar,
  REQ_Ad,
  REQ_Em, // mdoc(7) "EMphasis"
  REQ_Va,
  REQ_Nd,
  REQ_Nm,
  REQ_Cd,
  REQ_Cm,
  REQ_Ic,
  REQ_Ms,
  REQ_Or,
  REQ_Sy,
  REQ_Dv,
  REQ_Ev,
  REQ_Fr,
  REQ_Li,
  REQ_No,
  REQ_Ns,
  REQ_Tn,
  REQ_nN,
  REQ_perc_A,
  REQ_perc_D,
  REQ_perc_N,
  REQ_perc_O,
  REQ_perc_P,
  REQ_perc_Q,
  REQ_perc_V,
  REQ_perc_B,
  REQ_perc_J,
  REQ_perc_R,
  REQ_perc_T,
  REQ_An, // mdoc(7) "Author Name"
  REQ_Aq, // mdoc(7) "Angle bracket Quote"
  REQ_Bq, // mdoc(7) "Bracket Quote"
  REQ_Qq, // mdoc(7)  "straight double Quote"
  REQ_tr, // translate
  REQ_troff, // groff(7) "TROFF mode"
  REQ_nroff, // groff(7) "NROFF mode"
  REQ_als, // groff(7) "ALias String"
  REQ_rr, // groff(7) "Remove number Register"
  REQ_rnn, // groff(7) "ReName Number register"
  REQ_aln, // groff(7) "ALias Number register"
  REQ_shift, // groff(7) "SHIFT parameter"
  REQ_while, // groff(7) "WHILE loop"
  REQ_do, // groff(7) "DO command"
  REQ_Dx, // mdoc(7) "DragonFly" macro
  REQ_Ta, // mdoc(7) "Ta" inside .It macro
  REQ_break, // groff(7) "Break out of a while loop"
  REQ_nop, // groff(7) .nop macro
  REQ_URL, // man(7) .URL macro
  REQ_Sm,  // mdoc(7) space mode
  REQ_Xo,  // mdoc(7) extended argument list open
  REQ_Xc,  // mdoc(7) extended argument list close
};

class RequestHash
{
  public:
    // return the RequestNum for given first len bytes from str or REQ_UNKNOWN
    // if an undefined key is searched
    static RequestNum getRequest(const char *str, int len);
};

#endif

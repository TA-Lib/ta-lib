/*  ----------------------------------------------------------------<Prolog>-
    Name:       sflcryp.h
    Title:      Encryption and decryption functions
    Package:    Standard Function Library (SFL)

    Written:    1996/01/23  iMatix SFL project team <sfl@imatix.com>
    Revised:    1997/09/08

    Copyright:  Copyright (c) 1996-2000 iMatix Corporation

    Synopsis:   The encryption/decryption functions were based on the
                cryptosystem library by Andrew Brown <asb@cs.nott.ac.uk>,
                cleaned-up for portability.  Thanks for a great package.

                IDEA is registered as the international patent WO 91/18459
                "Device for Converting a Digital Block and the Use thereof".
                For commercial use of IDEA, you should contact:
                  ASCOM TECH AG
                  Freiburgstrasse 370
                  CH-3018 Bern, Switzerland

    License:    This is free software; you can redistribute it and/or modify
                it under the terms of the SFL License Agreement as provided
                in the file LICENSE.TXT.  This software is distributed in
                the hope that it will be useful, but without any warranty.

    Notes:
    Description of IDEA cipher
    --------------------------
    The IDEA cipher operates on 64 bit (8 byte) blocks, using a 128 bit (16
    byte) key. IDEA has found itself famous through its inclusion in the
    well-known PGP package. The following is from the introduction to chapter
    3 of the thesis that presented the cipher.

    The block cipher IDEA (International Data Encryption Algorithm) is based
    on the new design concept of "mixing operations from different algebraic
    groups". The required "confusion" was achieved by successively using three
    "incompatible" group operations on pairs of 16-bit subblocks and the cipher
    structure was chosen to provide the necessary "diffusion". The cipher
    structure was further chosen to facilitate both hardware and software
    implementations. The IDEA cipher is an improved version of PES and was
    developed to increase security against differential cryptanalysis.

    Description of MDC cipher
    -------------------------
    This is a method for turning a hash function, here MD5, into a fast
    secret-key encryption. Based on a suggestion by Phil Karn in sci.crypt, 13
    Feb 1992. See also his comments from sci.crypt, 23 Mar 1992. The method is
    a variant of that described in Zheng, Matsumoto and Imai, Crypto 89. See
    also, "A New Class of Cryptosystems Based on Interconnection Networks" by
    michaelp@terpsichore.informatic.rwth-aachen.de

    Description of DES cipher
    -------------------------
    DES is the well known U.S. Data Encryption Standard cipher.
    DES encrypts data in 64 bit blocks, using a 64 bit key -- of which
    56 bits are used in the encipherment process.
------------------------------------------------------------------</Prolog>-*/

#ifndef SFLCRYP_INCLUDED               /*  Allow multiple inclusions        */
#define SFLCRYP_INCLUDED


/*  Definitions of the encryption algorithms we support                      */

#define CRYPT_IDEA      0               /*  IDEA algorithm                   */
#define CRYPT_MDC       1               /*  MDC algorithm                    */
#define CRYPT_DES       2               /*  DES algorithm                    */
#define CRYPT_XOR       3               /*  A basic XOR algorithm            */
#define CRYPT_TOP       4               /*  We support 4 algorithms          */

/*  We define some tables that key off the encryption algorithm              */
#if 0
!!! TA-Lib removed
#if (defined (DEFINE_CRYPT_TABLES))
static int
    crypt_block_size [] = {             /*  Block size for each algorithm    */
       8, 32, 8, 16
    };
#define CRYPT_MAX_BLOCK_SIZE  32        /*  Largest block size, in bytes     */
#endif
#endif

/*  Function prototypes                                                      */

#ifdef __cplusplus
extern "C" {
#endif

Bool  crypt_encode  (byte *buffer, word buffer_size, int algorithm, 
                     const byte *key);
Bool  crypt_decode  (byte *buffer, word buffer_size, int algorithm, 
                     const byte *key);
qbyte calculate_crc (byte *block, size_t length);

qbyte calculate_running_crc( qbyte *runningData, byte *block, size_t length);


#ifdef __cplusplus
}
#endif

#endif

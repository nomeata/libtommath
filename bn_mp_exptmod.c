#include "tommath_private.h"
#ifdef BN_MP_EXPTMOD_C
/* LibTomMath, multiple-precision integer library -- Tom St Denis */
/* SPDX-License-Identifier: Unlicense */

/* this is a shell function that calls either the normal or Montgomery
 * exptmod functions.  Originally the call to the montgomery code was
 * embedded in the normal function but that wasted alot of stack space
 * for nothing (since 99% of the time the Montgomery code would be called)
 */
int mp_exptmod(const mp_int *G, const mp_int *X, const mp_int *P, mp_int *Y)
{
   int dr;

   /* modulus P must be positive */
   if (P->sign == MP_NEG) {
      return MP_VAL;
   }

   /* if exponent X is negative we have to recurse */
   if (MP_ENABLED(MP_INVMOD) && X->sign == MP_NEG) {
      mp_int tmpG, tmpX;
      int err;

      /* first compute 1/G mod P */
      if ((err = mp_init(&tmpG)) != MP_OKAY) {
         return err;
      }
      if ((err = mp_invmod(G, P, &tmpG)) != MP_OKAY) {
         mp_clear(&tmpG);
         return err;
      }

      /* now get |X| */
      if ((err = mp_init(&tmpX)) != MP_OKAY) {
         mp_clear(&tmpG);
         return err;
      }
      if ((err = mp_abs(X, &tmpX)) != MP_OKAY) {
         mp_clear_multi(&tmpG, &tmpX, NULL);
         return err;
      }

      /* and now compute (1/G)**|X| instead of G**X [X < 0] */
      err = mp_exptmod(&tmpG, &tmpX, P, Y);
      mp_clear_multi(&tmpG, &tmpX, NULL);
      return err;
   } else {
      /* no invmod */
      return MP_VAL;
   }

   /* modified diminished radix reduction */
   if (MP_ENABLED(MP_REDUCE_IS_2K_L) && MP_ENABLED(MP_REDUCE_2K_L) && MP_ENABLED(S_MP_EXPTMOD) &&
       mp_reduce_is_2k_l(P) == MP_YES) {
      return s_mp_exptmod(G, X, P, Y, 1);
   }

   /* is it a DR modulus? default to no */
   dr = MP_ENABLED(MP_DR_IS_MODULUS) ? mp_dr_is_modulus(P) : 0;

   /* if not, is it a unrestricted DR modulus? */
   if (MP_ENABLED(MP_REDUCE_IS_2K) && dr == 0) {
      dr = mp_reduce_is_2k(P) << 1;
   }

   /* if the modulus is odd or dr != 0 use the montgomery method */
   if (MP_ENABLED(MP_EXPTMOD_FAST) && (IS_ODD(P) || (dr !=  0))) {
      return mp_exptmod_fast(G, X, P, Y, dr);
   } else if (MP_ENABLED(S_MP_EXPTMOD)) {
      /* otherwise use the generic Barrett reduction technique */
      return s_mp_exptmod(G, X, P, Y, 0);
   } else {
      /* no exptmod for evens */
      return MP_VAL;
   }
}

#endif

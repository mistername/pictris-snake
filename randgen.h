/* 
 * File:   randgen.h
 * Author: jeroen.bol
 *
 * Created on 18 maart 2015, 9:07
 */

#ifndef __RANDGEN_H__
#define	__RANDGEN_H__

// the following select which algorithm will be used
#define RANDWIKI
#undef  RANDBYTE
#undef  RANDGEN
#define RANDGENPLUS

#include <stdint.h>

#if defined RANDBYTE || defined RANDWIKI
#define RND_NUM_T uint8_t
#define RND_NAM_T uint8_t
#elif defined RANDGEN
#endif

//void add_counter(uint8_t);
RND_NUM_T   rnd_get_num(void);
void        rnd_initialize(uint8_t);

#endif	/* __RANDGEN_H__ */


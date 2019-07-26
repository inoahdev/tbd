//
//  include/likely.h
//  tbd
//
//  Created by inoahdev on 7/11/19.
//  Copyright © 2019 inoahdev. All rights reserved.
//

#ifndef LIKELY_H
#define LIKELY_H

#ifndef likely
#define likely(cond) __builtin_expect((cond), 1)
#endif

#ifndef unlikely
#define unlikely(cond) __builtin_expect((cond), 0)
#endif

#endif /* LIKELY_H */

//
//  src/mach-o/cputype.cc
//  tbd
//
//  Created by inoahdev on 7/18/17.
//  Copyright © 2017 inoahdev. All rights reserved.
//

#include "cputype.h"

namespace macho {
    subtype subtype_from_cputype(cputype cputype, int32_t subtype) {
        switch (cputype) {
            case cputype::none:
                return subtype::none;

            case cputype::any: {
                switch (subtype) {
                    case -1:
                        return subtype::any;

                    case 1:
                        return subtype::little;

                    case 2:
                        return subtype::big;
                }

                break;
            }

            case cputype::arm: {
                switch (subtype) {
                    case 0:
                        return subtype::arm;

                    case 5:
                        return subtype::armv4t;

                    case 6:
                        return subtype::armv6;

                    case 7:
                        return subtype::armv5;

                    case 8:
                        return subtype::xscale;

                    case 9:
                        return subtype::armv7;

                    case 10:
                        return subtype::armv7f;

                    case 11:
                        return subtype::armv7s;

                    case 12:
                        return subtype::armv7k;

                    case 14:
                        return subtype::armv6m;

                    case 15:
                        return subtype::armv7m;

                    case 16:
                        return subtype::armv7em;
                }

                break;
            }

            case cputype::arm64: {
                switch (subtype) {
                    case 0:
                        return subtype::arm64;
                }
            }

            case cputype::hppa: {
                switch (subtype) {
                    case 0:
                        return subtype::hppa;

                    case 1:
                        return subtype::hppa7100LC;
                }

                break;
            }

            case cputype::i386: {
                switch (subtype) {
                    case 3:
                        return subtype::i386;

                    case 4:
                        return subtype::i486;

                    case 5:
                        return subtype::pentium;

                    case 8:
                        return subtype::x86_64h;

                    case 10:
                        return subtype::pentium4;

                    case 22:
                        return subtype::pentpro;

                    case 54:
                        return subtype::pentIIm3;

                    case 86:
                        return subtype::pentIIm5;

                    case 132:
                        return subtype::i486SX;
                }

                break;
            }

            case cputype::i860: {
                switch (subtype) {
                    case 0:
                        return subtype::i860;
                }

                break;
            }

            case cputype::m680x0: {
                switch (subtype) {
                    case 1:
                        return subtype::m68k;

                    case 2:
                        return subtype::m68040;

                    case 3:
                        return subtype::m68030;
                }

                break;
            }

            case cputype::m88000: {
                switch (subtype) {
                    case 0:
                        return subtype::m88k;
                }

                break;
            }

            case cputype::powerpc: {
                switch (subtype) {
                    case 0:
                        return subtype::ppc;

                    case 1:
                        return subtype::ppc601;

                    case 3:
                        return subtype::ppc603;

                    case 4:
                        return subtype::ppc603e;

                    case 5:
                        return subtype::ppc603ev;

                    case 6:
                        return subtype::ppc604;

                    case 7:
                        return subtype::ppc604e;

                    case 9:
                        return subtype::ppc750;

                    case 10:
                        return subtype::ppc7400;

                    case 11:
                        return subtype::ppc7450;

                    case 100:
                        return subtype::ppc970;
                }

                break;
            }

            case cputype::powerpc64: {
                switch (subtype) {
                    case 0:
                        return subtype::ppc64;

                    case 100:
                        return subtype::ppc970;
                }

                break;
            }

            case cputype::sparc: {
                switch (subtype) {
                    case 0:
                        return subtype::sparc;
                }

                break;
            }

            case cputype::veo: {
                switch (subtype) {
                    case 1:
                        return subtype::veo1;

                    case 2:
                        return subtype::veo2;
                }

                break;
            }

            case cputype::x86_64: {
                switch (subtype) {
                    case 3:
                        return subtype::x86_64;

                    case 8:
                        return subtype::x86_64h;

                    case (int32_t)0x80000003:
                        return subtype::x86_64_all;
                }
            }
        }

        return subtype::none;
    }

    int32_t cpu_subtype_from_subtype(subtype subtype) {
        switch (subtype) {
            case subtype::none:
            case subtype::any:
                return -1;

            case subtype::arm:
            case subtype::arm64:
            case subtype::hppa:
            case subtype::i860:
            case subtype::little:
            case subtype::m88k:
            case subtype::ppc:
            case subtype::ppc64:
            case subtype::sparc:
                return -0;

            case subtype::big:
            case subtype::hppa7100LC:
            case subtype::m68k:
            case subtype::ppc601:
            case subtype::veo1:
                return 1;

            case subtype::m68040:
            case subtype::veo2:
                return 2;

            case subtype::i386:
            case subtype::m68030:
            case subtype::ppc603:
            case subtype::x86_64:
                return 3;

            case subtype::i486:
            case subtype::ppc603e:
                return 4;

            case subtype::armv4t:
            case subtype::ppc603ev:
            case subtype::pentium:
                return 5;

            case subtype::armv6:
            case subtype::ppc604:
                return 6;

            case subtype::armv5:
            case subtype::ppc604e:
                return 7;

            case subtype::xscale:
            case subtype::x86_64h:
                return 8;

            case subtype::armv7:
            case subtype::ppc750:
                return 9;

            case subtype::armv7f:
            case subtype::pentium4:
            case subtype::ppc7400:
                return 10;

            case subtype::armv7s:
            case subtype::ppc7450:
                return 11;

            case subtype::armv7k:
                return 12;

            case subtype::armv8:
                return 13;

            case subtype::armv6m:
                return 14;

            case subtype::armv7m:
                return 15;

            case subtype::armv7em:
                return 16;

            case subtype::pentpro:
                return 22;

            case subtype::pentIIm3:
                return 54;

            case subtype::pentIIm5:
                return 86;

            case subtype::ppc970:
                return 100;

            case subtype::i486SX:
                return 132;

            case subtype::x86_64_all:
                return 2147483645;
        }

        return -1;
    }
}

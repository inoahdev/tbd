//
//  String.cpp
//  String
//
//  Created by Programming on 6/19/16.
//  Copyright © 2016 Programming. All rights reserved.
//

#include "String.hpp"

std::ostream& operator<<(std::ostream& lhs, const String& rhs) {
    lhs.write(rhs.data_, rhs.length_);
    return lhs;
}

std::iostream& operator<<(std::iostream& lhs, const String& rhs) {
    lhs.write(rhs.data_, rhs.length_);
    return lhs;
}

std::ostringstream& operator<<(std::ostringstream& lhs, const String& rhs) {
    lhs.write(rhs.data_, rhs.length_);
    return lhs;
}

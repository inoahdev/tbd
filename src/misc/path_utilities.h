//
//  src/misc/path_utilities.h
//  tbd
//
//  Created by inoahdev on 9/9/17.
//  Copyright © 2017 inoahdev. All rights reserved.
//

#include <utility>

namespace path {
    template <typename T>
    T find_next_slash(const T &begin, const T &end) {
        auto iter = begin;
        while (iter != end && *iter != '/' && *iter != '\\') {
            iter++;
        }

        while (iter != end && (iter[1] == '/' || iter[1] == '\\')) {
            iter++;
        }

        return iter;
    }

    inline char *find_next_slash(char *string) {
        auto iter = string;
        while (*iter != '\0' && *iter != '/' && *iter != '\\') {
            iter++;
        }

        while (*iter != '\0' && (iter[1] == '/' || iter[1] == '\\')) {
            iter++;
        }

        return iter;
    }

    template <typename T>
    T find_last_slash(const T &start, const T &end) {
        auto iter = end - 1;
        while (iter >= start) {
            if (*iter == '/' || *iter == '\\') {
                break;
            }

            --iter;
        }

        return iter;
    }

    inline char *find_last_slash(char *string) {
        auto iter = string;
        while (true) {
            if (*iter == '\0') {
                return nullptr;
            }

            if (*iter == '/' || *iter == '\\') {
                break;
            }

            ++iter;
        }

        return iter;
    }

    template <typename T>
    T find_last_slash_in_front_of_pattern(const T &start, const T &end) {
        auto iter = end - 1;
        while (iter >= start) {
            if (*iter == '/' || *iter == '\\') {
                break;
            }

            --iter;
        }

        while (iter >= start) {
            if (iter[-1] != '/' && iter[-1] != '\\') {
                break;
            }

            --iter;
        }

        return iter;
    }

    inline char *find_last_slash_in_front_of_pattern(char *string) {
        auto end = &string[strlen(string)];
        auto result = find_last_slash_in_front_of_pattern(string, end);

        if (result == end) {
            return nullptr;
        }

        return result;
    }

    template <typename T>
    std::pair<T, T> find_last_component(const T &begin, const T &end) {
        // Return an empty string if begin and
        // end are equal to each other

        if (begin == end) {
            return std::make_pair(begin, end);
        }

        // Start search for terminating slash of
        // requested path component at end - 1, and
        // moving backwards

        T end_slash = end - 1;
        if (*end_slash == '/' || *end_slash == '\\') {
            // Use a `while {}` instead of a `do { } while` here
            // to make sure end != begin + 1, otherwise we go behind
            // begin

            while (end_slash != begin && (end_slash[-1] == '/' || end_slash[-1] == '\\')) {
                --end_slash;
            }
        } else {
            // If end_slash is not a terminating
            // slash for the path-component, set
            // end-slash back to end

            end_slash = end;
        }

        // If end_slash went back to begin, two situations exist, either
        // no slash was found, and we should return the entire range or
        // the only slash found was the one at begin, in which case we
        // should return that one slash

        if (end_slash == begin) {
            // '/' or '\' is the first path component, returned here
            // if the slash found is at the beginning of the
            // string

            if (*end_slash == '/' || *end_slash == '\\') {
                return std::make_pair(begin, end_slash + 1);
            }

            return std::make_pair(begin, end);
        }

        T component_begin = find_last_slash(begin, end_slash);
        if (component_begin == end_slash) {
            // If we don't happen to find a
            // slash, return the whole string

            component_begin = begin;
        } else {
            // component_begin should begin at the first character
            // of the path component, not a slash

            component_begin++;

            // If component_begin is equal to
            // end-slash, then component_begin was
            // likely at the end of the string-range
            // provided, so set end_slash to end

            if (component_begin == end_slash) {
                end_slash = end;
            }
        }

        return std::make_pair(component_begin, end_slash);
    }

    inline std::pair<char *, char *> find_last_component(char *string) {
        return find_last_component(string, &string[strlen(string)]);
    }
}

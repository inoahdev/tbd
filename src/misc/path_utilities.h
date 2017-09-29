//
//  src/misc/path_utilities.h
//  tbd
//
//  Created by inoahdev on 9/9/17.
//  Copyright © 2017 inoahdev. All rights reserved.
//

#include <cstdint>
#include <cstring>

#include <utility>

namespace path {
    template <typename T>
    T find_next_slash(const T &begin, const T &end) {
        auto iter = begin;
        while (iter != end && *iter != '/' && *iter != '\\') {
            iter++;
        }

        return iter;
    }

    char *find_next_slash(char *string);
    const char *find_next_slash(const char *string);

    template <typename T>
    T find_next_unique_slash(const T &begin, const T &end) {
        auto iter = begin;
        while (iter != end && *iter != '/' && *iter != '\\') {
            iter++;
        }

        if (iter != end) {
            auto back = end - 1;
            while (iter != back && (iter[1] == '/' || iter[1] == '\\')) {
                iter++;
            }

            if (iter == back) {
                return end;
            }
        }

        return iter;
    }

    char *find_next_unique_slash(char *string);
    const char *find_next_unique_slash(const char *string);

    template <typename T>
    T find_last_slash(const T &begin, const T &end) {
        auto iter = end - 1;
        while (iter != begin) {
            if (*iter == '/' || *iter == '\\') {
                break;
            }

            --iter;
        }

        if (*iter != '/' && *iter != '\\') {
            return end;
        }

        return iter;
    }

    inline char *find_last_slash(char *string) {
        return find_last_slash(string, &string[strlen(string)]);
    }

    inline const char *find_last_slash(const char *string) {
        return find_last_slash(string, &string[strlen(string)]);
    }

    template <typename T>
    T find_last_slash_in_front_of_pattern(const T &begin, const T &end) {
        auto iter = end - 1;
        while (iter >= begin) {
            if (*iter == '/' || *iter == '\\') {
                break;
            }

            --iter;
        }

        while (iter > begin) {
            if (iter[-1] != '/' && iter[-1] != '\\') {
                break;
            }

            --iter;
        }

        if (*iter != '/' && *iter != '\\') {
            return end;
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
    std::pair<T, T> find_next_component(const T &begin, const T &end) {
        if (begin == end || begin + 1 == end) {
            return std::make_pair(begin, end);
        }

        auto begin_slash = begin;
        auto end_slash = begin_slash + 1;

        if (*begin_slash == '/' || *begin_slash == '\\') {
            return std::make_pair(begin_slash, end_slash);
        }

        while (end_slash != end && *end_slash != '/' && *end_slash != '\\') {
            end_slash++;
        }

        return std::make_pair(begin_slash, end_slash);
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

        auto end_slash = end - 1;
        if (*end_slash == '/' || *end_slash == '\\') {
            // Use a `while {}` instead of a `do { } while` here
            // to make sure end != begin + 1, otherwise we go behind
            // begin

            while (end_slash != begin && (end_slash[-1] == '/' || end_slash[-1] == '\\')) {
                --end_slash;
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
        } else {
            // If end_slash is not a terminating
            // slash for the path-component, set
            // end-slash back to end

            end_slash = end;
        }

        auto component_begin = find_last_slash(begin, end_slash);
        if (component_begin == end_slash) {
            // If we don't happen to find a
            // slash, return from beginning

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

    inline std::pair<const char *, const char *> find_last_component(const char *string) {
        return find_last_component(string, &string[strlen(string)]);
    }

    template <typename T>
    int compare(const T &lhs_begin, const T &lhs_end, const T &rhs_begin, const T &rhs_end) {
        // Skip beginning slashes

        auto lhs_iter = lhs_begin;
        while (lhs_iter != lhs_end && (*lhs_iter == '/' || *lhs_iter == '\\')) {
            ++lhs_iter;
        }

        auto rhs_iter = rhs_begin;
        while (rhs_iter != rhs_end && (*rhs_iter == '/' || *rhs_iter == '\\')) {
            ++rhs_iter;
        }

        // Set end to the front of terminating row of slashes

        auto lhs_reverse_iter = lhs_end;
        if (*(lhs_reverse_iter - 1) == '/' || *(lhs_reverse_iter - 1) == '\\') {
            do {
                --lhs_reverse_iter;
            } while (lhs_reverse_iter != lhs_begin && (*(lhs_reverse_iter - 1) == '/' || *(lhs_reverse_iter - 1) == '\\'));
        }

        auto rhs_reverse_iter = rhs_end;
        if (*(rhs_reverse_iter - 1) == '/' || *(rhs_reverse_iter - 1) == '\\') {
            do {
                --rhs_reverse_iter;
            } while (rhs_reverse_iter != rhs_begin && (*(rhs_reverse_iter - 1) == '/' || *(rhs_reverse_iter - 1) == '\\'));
        }

        // A check for `*hs_reverse_iter` == `*hs_end` isn't needed
        // because the path has already been checked for having atleast
        // 1 non-slash character

        auto lhs_path_component_begin = lhs_iter;
        auto rhs_path_component_begin = rhs_iter;

        while (lhs_path_component_begin != lhs_reverse_iter && rhs_path_component_begin != rhs_reverse_iter) {
            // Skip over the "unique" slash to the front of the
            // path component

            ++lhs_path_component_begin;
            ++rhs_path_component_begin;

            auto lhs_path_component_end = find_next_slash(lhs_path_component_begin, lhs_reverse_iter);
            auto rhs_path_component_end = find_next_slash(rhs_path_component_begin, rhs_reverse_iter);

            const auto lhs_path_component_length = lhs_path_component_end - lhs_path_component_begin;
            const auto rhs_path_component_length = lhs_path_component_end - rhs_path_component_begin;

            if (lhs_path_component_length < rhs_path_component_length) {
                return -*(rhs_path_component_begin + lhs_path_component_length);
            } else if (rhs_path_component_length > lhs_path_component_length) {
                return *(lhs_path_component_begin + rhs_path_component_length);
            }

            // both lhs_path_component and rhs_path_component should have the same length

            auto lhs_path_component_iter = lhs_path_component_begin;
            auto rhs_path_component_iter = rhs_path_component_begin;

            for (; lhs_path_component_iter != lhs_path_component_end; lhs_path_component_iter++, rhs_path_component_end++) {
                if (*lhs_path_component_iter == *rhs_path_component_iter) {
                    continue;
                }

                return *lhs_path_component_iter - *rhs_path_component_iter;
            }

            lhs_path_component_begin = find_next_unique_slash(lhs_path_component_end, lhs_reverse_iter);
            rhs_path_component_begin = find_next_unique_slash(rhs_path_component_end, rhs_reverse_iter);
        }

        if (lhs_path_component_begin == lhs_reverse_iter) {
            if (rhs_path_component_begin == rhs_reverse_iter) {
                return 0;
            }

            return -*rhs_path_component_begin;
        } else if (rhs_path_component_begin == rhs_reverse_iter) {
            return *lhs_path_component_begin;
        }

        return 0;
    }

    template <typename T>
    inline bool ends_with_slash(const T &begin, const T &end) {
        return end[-1] == '/' || end[-1] == '\\';
    }

    bool ends_with_slash(const char *string);
}

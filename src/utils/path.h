//
//  src/utils/path.h
//  tbd
//
//  Created by inoahdev on 9/9/17.
//  Copyright © 2017 inoahdev. All rights reserved.
//

#pragma once

#include <utility>
#include "string.h"

namespace utils::path {
    template <typename T>
    T find_next_slash(const T &begin, const T &end) noexcept {
        auto iter = begin;
        while (iter != end) {
            if (const auto &elmt = *iter; elmt != '/' && elmt != '\\') {
                iter++;
                continue;
            }

            break;
        }

        return iter;
    }

    char *find_next_slash(char *string);
    const char *find_next_slash(const char *string);

    template <typename T>
    T find_next_slash_at_back_of_pattern(const T &begin, const T &end) noexcept {
        auto iter = begin;
        while (iter != end && *iter != '/' && *iter != '\\') {
            iter++;
        }

        if (iter != end) {
            auto back = end - 1;
            while (iter != back) {
                if (const auto &elmt = iter[1]; (elmt != '/' && elmt != '\\')) {
                    break;
                }

                iter++;
            }

            if (iter == back) {
                return end;
            }
        }

        return iter;
    }

    char *find_next_slash_at_back_of_pattern(char *string);
    const char *find_next_slash_at_back_of_pattern(const char *string);

    template <typename T>
    T find_last_slash(const T &begin, const T &end) noexcept {
        auto iter = end - 1;
        while (iter != begin) {
            if (const auto &elmt = *iter; elmt == '/' || elmt == '\\') {
                break;
            }

            --iter;
        }

        if (const auto &elmt = *iter; elmt != '/' && elmt != '\\') {
            return end;
        }

        return iter;
    }

    inline char *find_last_slash(char *string) noexcept {
        return find_last_slash(string, string::find_end_of_null_terminated_string(string));
    }

    inline const char *find_last_slash(const char *string) noexcept {
        return find_last_slash(string, string::find_end_of_null_terminated_string(string));
    }

    template <typename T>
    T find_last_slash_in_front_of_pattern(const T &begin, const T &end) noexcept {
        auto iter = end - 1;
        while (iter > begin) {
            if (const auto &elmt = *iter; elmt == '/' || elmt == '\\') {
                break;
            }

            --iter;
        }

        do {
            if (const auto &elmt = iter[-1]; elmt != '/' && elmt != '\\') {
                break;
            }

            --iter;
        } while (iter > begin);

        if (const auto &elmt = *iter; elmt != '/' && elmt != '\\') {
            return end;
        }

        return iter;
    }

    inline char *find_last_slash_in_front_of_pattern(char *string) noexcept {
        auto end = string::find_end_of_null_terminated_string(string);
        auto result = find_last_slash_in_front_of_pattern(string, end);

        if (result == end) {
            return nullptr;
        }

        return result;
    }

    template <typename T>
    T find_end_of_row_of_slashes(const T &begin, const T &end) noexcept {
        auto iter = begin;
        while (iter != end) {
            if (const auto &elmt = *iter; elmt == '/' || elmt == '\\') {
                iter++;
                continue;
            }

            break;
        }

        return iter;
    }

    inline char *find_end_of_row_of_slashes(char *string) noexcept {
        auto end = string::find_end_of_null_terminated_string(string);
        auto result = find_end_of_row_of_slashes(string, end);

        if (result == end) {
            return nullptr;
        }

        return result;
    }

    inline const char *find_end_of_row_of_slashes(const char *string) noexcept {
        auto end = string::find_end_of_null_terminated_string(string);
        auto result = find_end_of_row_of_slashes(string, end);

        if (result == end) {
            return nullptr;
        }

        return result;
    }

    template <typename T>
    std::pair<T, T> find_next_component(const T &begin, const T &end) noexcept {
        auto begin_slash = begin;
        if (begin_slash == end) {
            return std::make_pair(begin, end);
        }

        auto end_slash = begin_slash + 1;
        if (end_slash == end) {
            return std::make_pair(begin, end);
        }

        if (const auto &begin_slash_elmt = *begin_slash; begin_slash_elmt == '/' || begin_slash_elmt == '\\') {
            return std::make_pair(begin_slash, end_slash);
        }

        while (end_slash != end) {
            if (const auto &end_slash_char = *end_slash; end_slash_char != '/' && end_slash_char != '\\') {
                end_slash++;
                continue;
            }

            break;
        }

        return std::make_pair(begin_slash, end_slash);
    }

    template <typename T>
    std::pair<T, T> find_last_component(const T &begin, const T &end) noexcept {
        // Return an empty string if begin and
        // end are equal to each other

        if (begin == end) {
            return std::make_pair(begin, end);
        }

        // Start search for terminating slash of
        // requested path component at end - 1, and
        // moving backwards

        auto end_slash = end - 1;
        if (const auto &end_slash_elmt = *end_slash; end_slash_elmt == '/' || end_slash_elmt == '\\') {
            // Use a `while {}` instead of a `do { } while` here
            // to make sure end != begin + 1, otherwise we go behind
            // begin

            while (end_slash != begin) {
                if (const auto &end_slash_prev = end_slash[-1]; end_slash_prev == '/' || end_slash_prev == '\\') {
                    --end_slash;
                    continue;
                }

                break;
            }

            // If end_slash went back to begin, two situations exist, either
            // no slash was found, and we should return the entire range or
            // the only slash found was the one at begin, in which case we
            // should return that one slash

            if (end_slash == begin) {
                // '/' or '\' is the first path component, returned here
                // if the slash found is at the beginning of the
                // string

                if (const auto &end_slash_elmt = *end_slash; end_slash_elmt == '/' || end_slash_elmt == '\\') {
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

    inline std::pair<char *, char *> find_last_component(char *string) noexcept {
        return find_last_component(string, string::find_end_of_null_terminated_string(string));
    }

    inline std::pair<const char *, const char *> find_last_component(const char *string) noexcept {
        return find_last_component(string, string::find_end_of_null_terminated_string(string));
    }

    template <typename T>
    T find_extension(const T &begin, const T &end) noexcept {
        auto iter = end - 1;
        auto elmt = *iter;

        while (iter != begin && elmt != '.') {
            iter--;
            elmt = *iter;
        }

        if (elmt != '.') {
            return end;
        }

        return iter;
    }

    inline char *find_extension(char *string) noexcept {
        auto end = string::find_end_of_null_terminated_string(string);
        auto result = find_extension(string, end);

        if (result == end) {
            return nullptr;
        }

        return result;
    }

    inline const char *find_extension(const char *string) noexcept {
        auto end = string::find_end_of_null_terminated_string(string);
        auto result = find_extension(string, end);

        if (result == end) {
            return nullptr;
        }

        return result;
    }

    // component_compare strictly compares path-components

    template <typename T1, typename T2, typename T3, typename T4>
    int component_compare(const T1 &lhs_begin, const T2 &lhs_end, const T3 &rhs_begin, const T4 &rhs_end) noexcept {
        // Set end (for searching) to the front of terminating row of slashes

        auto lhs_reverse_iter = lhs_end;
        if (const auto &lhs_reverse_iter_elmt = lhs_reverse_iter[-1]; lhs_reverse_iter_elmt == '/' || lhs_reverse_iter_elmt == '\\') {
            do {
                --lhs_reverse_iter;
                lhs_reverse_iter_elmt = lhs_reverse_iter[-1];
            } while (lhs_reverse_iter != lhs_begin && (lhs_reverse_iter_elmt == '/' || lhs_reverse_iter_elmt == '\\'));
        }

        auto rhs_reverse_iter = rhs_end;
        if (const auto &rhs_reverse_iter_elmt = rhs_reverse_iter[-1]; rhs_reverse_iter_elmt == '/' || rhs_reverse_iter_elmt == '\\') {
            do {
                --rhs_reverse_iter;
                rhs_reverse_iter_elmt = rhs_reverse_iter[-1];
            } while (rhs_reverse_iter != rhs_begin && (rhs_reverse_iter_elmt == '/' || rhs_reverse_iter_elmt == '\\'));
        }

        // Check to see if both paths have beginning slashes

        auto lhs_path_component_begin = lhs_begin;
        auto rhs_path_component_begin = rhs_begin;

        if (const auto &lhs_path_component_begin_char = *lhs_path_component_begin; lhs_path_component_begin_char == '/' || lhs_path_component_begin_char == '\\') {
            if (const auto &rhs_path_component_begin_char = *rhs_path_component_begin; rhs_path_component_begin_char != '/' && rhs_path_component_begin_char != '\\') {
                return lhs_path_component_begin_char - rhs_path_component_begin_char;
            }

            lhs_path_component_begin = find_end_of_row_of_slashes(lhs_path_component_begin, lhs_reverse_iter);
            rhs_path_component_begin = find_end_of_row_of_slashes(rhs_path_component_begin, rhs_reverse_iter);
        }

        auto lhs_path_component_end = lhs_reverse_iter;
        auto rhs_path_component_end = rhs_reverse_iter;

        while (lhs_path_component_begin != lhs_reverse_iter && rhs_path_component_begin != rhs_reverse_iter) {
            lhs_path_component_end = find_next_slash(lhs_path_component_begin, lhs_reverse_iter);
            rhs_path_component_end = find_next_slash(rhs_path_component_begin, rhs_reverse_iter);

            const auto lhs_path_component_length = lhs_path_component_end - lhs_path_component_begin;
            const auto rhs_path_component_length = rhs_path_component_end - rhs_path_component_begin;

            if (lhs_path_component_length < rhs_path_component_length) {
                return -*(rhs_path_component_begin + lhs_path_component_length);
            } else if (rhs_path_component_length > lhs_path_component_length) {
                return *(lhs_path_component_begin + rhs_path_component_length);
            }

            // both lhs_path_component and rhs_path_component should have the same length

            auto lhs_path_component_iter = lhs_path_component_begin;
            auto rhs_path_component_iter = rhs_path_component_begin;

            for (; lhs_path_component_iter != lhs_path_component_end; lhs_path_component_iter++, rhs_path_component_iter++) {
                const auto &lhs_path_component_elmt = *lhs_path_component_iter;
                const auto &rhs_path_component_elmt = *rhs_path_component_iter;

                if (lhs_path_component_elmt == rhs_path_component_elmt) {
                    continue;
                }

                return lhs_path_component_elmt - rhs_path_component_elmt;
            }

            // Skip over rows of slashes to the beginning of the path-component

            lhs_path_component_begin = find_end_of_row_of_slashes(lhs_path_component_end, lhs_reverse_iter);
            rhs_path_component_begin = find_end_of_row_of_slashes(rhs_path_component_end, rhs_reverse_iter);
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

    template <typename T1, typename T2, typename T3, typename T4>
    int compare(const T1 &lhs_begin, const T2 &lhs_end, const T3 &rhs_begin, const T4 &rhs_end) noexcept {
        // Set end (for searching) to the front of terminating row of slashes

        auto lhs_reverse_iter = lhs_end;
        if (auto lhs_reverse_iter_elmt = lhs_reverse_iter[-1]; lhs_reverse_iter_elmt == '/' || lhs_reverse_iter_elmt == '\\') {
            do {
                --lhs_reverse_iter;
                lhs_reverse_iter_elmt = lhs_reverse_iter[-1];
            } while (lhs_reverse_iter != lhs_begin && (lhs_reverse_iter_elmt == '/' || lhs_reverse_iter_elmt == '\\'));
        }

        auto rhs_reverse_iter = rhs_end;
        if (auto rhs_reverse_iter_elmt = rhs_reverse_iter[-1]; rhs_reverse_iter_elmt == '/' || rhs_reverse_iter_elmt == '\\') {
            do {
                --rhs_reverse_iter;
                rhs_reverse_iter_elmt = rhs_reverse_iter[-1];
            } while (rhs_reverse_iter != rhs_begin && (rhs_reverse_iter_elmt == '/' || rhs_reverse_iter_elmt == '\\'));
        }

        // Check to see if both paths have beginning slashes

        auto lhs_path_component_begin = lhs_begin;
        auto rhs_path_component_begin = rhs_begin;

        if (const auto &lhs_path_component_begin_char = *lhs_path_component_begin; lhs_path_component_begin_char == '/' || lhs_path_component_begin_char == '\\') {
            if (const auto &rhs_path_component_begin_char = *rhs_path_component_begin; rhs_path_component_begin_char != '/' && rhs_path_component_begin_char != '\\') {
                return lhs_path_component_begin_char - rhs_path_component_begin_char;
            }

            lhs_path_component_begin = find_end_of_row_of_slashes(lhs_path_component_begin, lhs_reverse_iter);
            rhs_path_component_begin = find_end_of_row_of_slashes(rhs_path_component_begin, rhs_reverse_iter);
        }

        auto lhs_path_component_end = lhs_reverse_iter;
        auto rhs_path_component_end = rhs_reverse_iter;

        while (lhs_path_component_begin != lhs_reverse_iter && rhs_path_component_begin != rhs_reverse_iter) {
            lhs_path_component_end = find_next_slash(lhs_path_component_begin, lhs_reverse_iter);
            rhs_path_component_end = find_next_slash(rhs_path_component_begin, rhs_reverse_iter);

            const auto lhs_path_component_length = lhs_path_component_end - lhs_path_component_begin;
            const auto rhs_path_component_length = rhs_path_component_end - rhs_path_component_begin;

            // Skip over '.' components

            if (const auto &lhs_path_component_begin_char = *lhs_path_component_begin; lhs_path_component_begin_char == '.' && lhs_path_component_length == 1) {
                if (const auto &rhs_path_component_begin_char = *rhs_path_component_begin; rhs_path_component_begin_char == '.' && rhs_path_component_length == 1) {
                    rhs_path_component_begin = find_end_of_row_of_slashes(rhs_path_component_end, rhs_reverse_iter);
                }
                
                lhs_path_component_begin = find_end_of_row_of_slashes(lhs_path_component_end, lhs_reverse_iter);
                continue;
            } else if (const auto &rhs_path_component_begin_char = *rhs_path_component_begin; rhs_path_component_begin_char == '.' && rhs_path_component_length == 1) {
                rhs_path_component_begin = find_end_of_row_of_slashes(rhs_path_component_end, rhs_reverse_iter);
                continue;
            } else {
                if (lhs_path_component_length < rhs_path_component_length) {
                    return -*(rhs_path_component_begin + lhs_path_component_length);
                } else if (rhs_path_component_length > lhs_path_component_length) {
                    return *(lhs_path_component_begin + rhs_path_component_length);
                }
            }

            // both lhs_path_component and rhs_path_component should have the same length

            auto lhs_path_component_iter = lhs_path_component_begin;
            auto rhs_path_component_iter = rhs_path_component_begin;

            for (; lhs_path_component_iter != lhs_path_component_end; lhs_path_component_iter++, rhs_path_component_iter++) {
                const auto &lhs_path_component_elmt = *lhs_path_component_iter;
                const auto &rhs_path_component_elmt = *rhs_path_component_iter;

                if (lhs_path_component_elmt == rhs_path_component_elmt) {
                    continue;
                }

                return lhs_path_component_elmt - rhs_path_component_elmt;
            }

            // Skip over rows of slashes to the beginning of the path-component

            lhs_path_component_begin = find_end_of_row_of_slashes(lhs_path_component_end, lhs_reverse_iter);
            rhs_path_component_begin = find_end_of_row_of_slashes(rhs_path_component_end, rhs_reverse_iter);
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

    template <typename S, typename T1, typename T2>
    S clean(const T1 &begin, const T2 &end) noexcept {
        auto path = S();
        auto iter = begin;

        if (iter == end) {
            return path;
        }

        if (const auto &elmt = *iter; elmt == '/' || elmt == '\\') {
            path.append(1, '/');
            iter = find_end_of_row_of_slashes(begin, end);
        }

        auto path_component_begin = iter;
        auto path_component_end = end;

        while (path_component_begin != end) {
            path_component_end = find_next_slash(path_component_begin, end);

            // Don't add '.' path-components

            auto next_path_component_begin = find_end_of_row_of_slashes(path_component_end, end);
            auto path_component_length = path_component_end - path_component_begin;

            if (*path_component_begin != '.' || path_component_length != 1) {
                path.append(path_component_begin, path_component_end);

                if (next_path_component_begin != path_component_end) {
                    path.append(1, '/');
                }
            }

            path_component_begin = next_path_component_begin;
        }

        return path;
    }

    template <typename S>
    S &clean(S &path) noexcept {
        auto begin = path.begin();
        auto end = path.end();

        auto slash_row_begin = begin;

        do {
            if (slash_row_begin == end) {
                break;
            }

            auto slash_row_end = find_end_of_row_of_slashes(slash_row_begin, end);
            if (slash_row_begin != slash_row_end) {
                // Change backward-slashes into forward-slashes

                auto &slash_row_begin_elmt = *slash_row_begin;
                if (slash_row_begin_elmt == '\\') {
                    slash_row_begin_elmt = '/';
                }

                slash_row_begin++;
            }

            // Remove './' path-components

            auto path_component_begin = slash_row_begin;
            if (*path_component_begin == '.') {
                auto path_component_end = find_next_slash(path_component_begin, end);
                auto path_component_length = path_component_end - path_component_begin;

                if (path_component_length == 1) {
                    slash_row_begin = path.erase(path_component_begin, path_component_end);
                    end = path.end();
                }
            }

            if (slash_row_begin != end) {
                slash_row_begin = path.erase(slash_row_begin, slash_row_end);

                end = path.end();
                slash_row_begin = find_next_slash(slash_row_begin, end);
            }
        } while (true);

        return path;
    }

    template <typename T1, typename T2, typename T3, typename T4>
    bool is_subdirectory_of(const T1 &lhs_begin, const T2 &lhs_end, const T3 &rhs_begin, const T4 &rhs_end) noexcept {
        auto lhs_reverse_iter = lhs_end;
        if (auto lhs_reverse_iter_elmt = lhs_reverse_iter[-1]; lhs_reverse_iter_elmt == '/' || lhs_reverse_iter_elmt == '\\') {
            do {
                --lhs_reverse_iter;
                lhs_reverse_iter_elmt = lhs_reverse_iter[-1];
            } while (lhs_reverse_iter != lhs_begin && (lhs_reverse_iter_elmt == '/' || lhs_reverse_iter_elmt == '\\'));
        }

        auto rhs_reverse_iter = rhs_end;
        if (auto rhs_reverse_iter_elmt = rhs_reverse_iter[-1]; rhs_reverse_iter_elmt == '/' || rhs_reverse_iter_elmt == '\\') {
            do {
                --rhs_reverse_iter;
                rhs_reverse_iter_elmt = rhs_reverse_iter[-1];
            } while (rhs_reverse_iter != rhs_begin && (rhs_reverse_iter_elmt == '/' || rhs_reverse_iter_elmt == '\\'));
        }

        // Check to see if both paths have beginning slashes

        auto lhs_path_component_begin = lhs_begin;
        auto rhs_path_component_begin = rhs_begin;

        if (const auto &lhs_path_component_begin_char = *lhs_path_component_begin; lhs_path_component_begin_char == '/' || lhs_path_component_begin_char == '\\') {
            if (const auto &rhs_path_component_begin_char = *rhs_path_component_begin; rhs_path_component_begin_char != '/' && rhs_path_component_begin_char != '\\') {
                return false;
            }

            lhs_path_component_begin = find_end_of_row_of_slashes(lhs_path_component_begin, lhs_reverse_iter);
            rhs_path_component_begin = find_end_of_row_of_slashes(rhs_path_component_begin, rhs_reverse_iter);
        }

        auto lhs_path_component_end = lhs_reverse_iter;
        auto rhs_path_component_end = rhs_reverse_iter;

        while (lhs_path_component_begin != lhs_reverse_iter && rhs_path_component_begin != rhs_reverse_iter) {
            // Skip over the "unique" slash to the front of the
            // path component

            lhs_path_component_end = find_next_slash(lhs_path_component_begin, lhs_reverse_iter);
            rhs_path_component_end = find_next_slash(rhs_path_component_begin, rhs_reverse_iter);

            const auto lhs_path_component_length = lhs_path_component_end - lhs_path_component_begin;
            const auto rhs_path_component_length = rhs_path_component_end - rhs_path_component_begin;

            if (lhs_path_component_length < rhs_path_component_length) {
                return -*(rhs_path_component_begin + lhs_path_component_length);
            } else if (rhs_path_component_length > lhs_path_component_length) {
                return *(lhs_path_component_begin + rhs_path_component_length);
            }

            // Skip over '.' components

            if (lhs_path_component_length == 1) {
                if (const auto &lhs_path_component_begin_char = *lhs_path_component_begin; lhs_path_component_begin_char == '.') {
                    if (const auto &rhs_path_component_begin_char = *rhs_path_component_begin; rhs_path_component_begin_char == '.') {
                        rhs_path_component_begin = find_end_of_row_of_slashes(rhs_path_component_end, rhs_reverse_iter);
                    }

                    lhs_path_component_begin = find_end_of_row_of_slashes(lhs_path_component_end, lhs_reverse_iter);
                    continue;
                }
            }

            // both lhs_path_component and rhs_path_component should have the same length

            auto lhs_path_component_iter = lhs_path_component_begin;
            auto rhs_path_component_iter = rhs_path_component_begin;

            for (; lhs_path_component_iter != lhs_path_component_end; lhs_path_component_iter++, rhs_path_component_iter++) {
                const auto &lhs_path_component_elmt = *lhs_path_component_iter;
                const auto &rhs_path_component_elmt = *rhs_path_component_iter;

                if (lhs_path_component_elmt == rhs_path_component_elmt) {
                    continue;
                }

                return false;
            }

            lhs_path_component_begin = find_end_of_row_of_slashes(lhs_path_component_end, lhs_reverse_iter);
            rhs_path_component_begin = find_end_of_row_of_slashes(rhs_path_component_end, rhs_reverse_iter);
        }

        if (lhs_path_component_begin == lhs_reverse_iter) {
            return false;
        }

        return rhs_path_component_begin == rhs_reverse_iter;
    }

    template <typename S, typename T1, typename T2>
    S &append_component(S &path, const T1 &component_begin, const T2 &component_end) {
        const auto &path_back = path.back();
        if (path_back != '/' && path_back != '\\') {
            if (const auto &component_front = *component_begin; component_front != '/' && component_front != '\\') {
                path.append(1, '/');
            }

            path.append(component_begin, component_end);
        } else {
            if (const auto &component_front = *component_begin; component_front == '/' || component_front == '\\') {
                path.append(component_begin + 1, component_end);
            } else {
                path.append(component_begin, component_end);
            }
        }

        return path;
    }
}

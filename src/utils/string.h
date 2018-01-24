//
//  src/utils/string.h
//  tbd
//
//  Created by inoahdev on 10/15/17.
//  Copyright © 2017 - 2018 inoahdev. All rights reserved.
//

#pragma once

namespace utils::string {
    template <typename T, typename C>
    T find_character_in_string(const T &begin, const C &character) noexcept {
        auto iter = begin;
        while (*iter != character) {
            iter++;
        }

        return iter;
    }

    template <typename T, typename C>
    T find_character_in_string(const T &begin, const T &end, const C &character) noexcept {
        auto iter = begin;
        while (iter != end && *iter != character) {
            iter++;
        }

        return iter;
    }

    template <typename T, typename C>
    T find_character_in_null_terminated_string(const T &begin, const C &character) noexcept {
        auto iter = begin;
        auto elmt = *iter;

        while (elmt != '\0' && elmt != character) {
            elmt = *(iter++);
        }

        return iter;
    }

    template <typename T1, typename T2>
    T1 find_string_in_string(const T1 &lhs_begin, const T2 &rhs_begin, const T2 &rhs_end) noexcept {
        auto lhs_iter = lhs_begin;
        while (true) {
            auto lhs_search_iter = lhs_iter;
            auto lhs_search_elmt = *lhs_search_iter;

            auto rhs_iter = rhs_begin;
            auto rhs_elmt = *rhs_iter;

            for (; rhs_iter != rhs_end && lhs_search_elmt == rhs_elmt; ) {
                lhs_search_elmt = *(lhs_search_iter++);
                rhs_elmt = *(rhs_iter++);
            }

            if (rhs_iter == rhs_end) {
                return lhs_iter;
            }

            lhs_iter++;
        }

        return lhs_iter;
    }

    template <typename T1, typename T2>
    T1 find_string_in_null_terminated_string(const T1 &lhs_begin, const T2 &rhs_begin, const T2 &rhs_end) noexcept {
        return find_string_in_string(lhs_begin, find_end_of_null_terminated_string(lhs_begin), rhs_begin, rhs_end);
    }

    template <typename T1, typename T2>
    T1 find_string_in_string(const T1 &lhs_begin, const T1 &lhs_end, const T2 &rhs_begin, const T2 &rhs_end) noexcept {
        const auto lhs_length = lhs_end - lhs_begin;
        const auto rhs_length = rhs_end - rhs_begin;

        if (lhs_length < rhs_length) {
            return lhs_end;
        }

        auto lhs_iter = lhs_begin;
        const auto lhs_search_end = lhs_end - rhs_length;

        for (; lhs_iter != lhs_search_end; lhs_iter++) {
            auto lhs_search_iter = lhs_iter;
            auto lhs_search_elmt = *lhs_search_iter;

            auto rhs_iter = rhs_begin;
            auto rhs_elmt = *rhs_iter;

            for (; rhs_iter != rhs_end && lhs_search_elmt == rhs_elmt; ) {
                lhs_search_elmt = *(lhs_search_iter++);
                rhs_elmt = *(rhs_iter++);
            }

            if (rhs_iter == rhs_end) {
                return lhs_iter;
            }
        }

        return lhs_iter;
    }

    template <typename T, typename C>
    T find_last_of_character_in_string(const T &begin, const T &end, const C &character) noexcept {
        if (begin == end) {
            return end;
        }

        auto iter = end;
        auto elmt = character;

        do {
            iter--;
        } while (iter != begin && (elmt = *iter) != character);

        if (elmt != character) {
            return end;
        }

        return end;
    }

    template <typename T1, typename T2>
    T1 find_last_of_string_in_string(const T1 &lhs_begin, const T1 &lhs_end, const T2 &rhs_begin, const T2 &rhs_end) noexcept {
        const auto lhs_length = lhs_end - lhs_begin;
        const auto rhs_length = rhs_end - rhs_begin;

        if (lhs_length < rhs_length) {
            return lhs_end;
        }

        auto lhs_iter = lhs_end;
        const auto lhs_search_begin = lhs_begin + rhs_length;

        for (; lhs_iter != lhs_search_begin; lhs_iter--) {
            auto lhs_search_iter = lhs_iter - rhs_length;
            auto lhs_search_elmt = *lhs_search_iter;

            auto rhs_iter = rhs_begin;
            auto rhs_elmt = *rhs_iter;

            for (; rhs_iter != rhs_end && lhs_search_elmt == rhs_elmt; ) {
                lhs_search_elmt = *(lhs_search_iter++);
                rhs_elmt = *(rhs_iter++);
            }

            if (rhs_iter == rhs_end) {
                return lhs_iter;
            }
        }

        return lhs_iter;
    }

    template <typename T1, typename T2>
    T1 find_last_of_string_in_null_terminated_string(const T1 &lhs_begin, const T1 &lhs_end, const T2 &rhs_begin, const T2 &rhs_end) noexcept {
        return find_last_of_string_in_string(lhs_begin, find_end_of_null_terminated_string(lhs_begin), rhs_begin, rhs_end);
    }

    template <typename S>
    inline S find_end_of_null_terminated_string(const S &string) {
        return find_character_in_string(string, '\0');
    }
}

//
//  String.hpp
//  String
//
//  Created by Programming on 6/19/16.
//  Copyright © 2016 Programming. All rights reserved.
//

#ifndef String_hpp
#define String_hpp

#include <climits>
#include <cstdlib>

#include <iostream>
#include <memory>

#include <sstream>
#include <stdio.h>
#include <string.h>

#include <string> //for std::string support

template <class _Allocator = std::allocator<char>>
class _String {
public:
    typedef size_t Size;
    
    typedef char *Iterator;
    typedef const char *ConstIterator;
    
    typedef std::reverse_iterator<Iterator> ReverseIterator;
    typedef std::reverse_iterator<ConstIterator> ConstReverseIterator;
    
    _String() {
        data_ = nullptr;
        length_ = 0;
        
        _allocateMemory(0);
    }
    
    _String(char c, Size n) {
        _allocateMemory(n);
        for (Size i = 0; i < n; i++) data_[i] = c;
    }
    
    _String(char *str, ...) noexcept {
        _constructString(str);
        
        va_list list;
        va_start(list, str);
        
        Size sizeCheck = vsnprintf(data_, length_, str, list);
        va_end(list);
        
        strncpy(data_, str, length_); //since we used data_ as a buffer, reapply the old value back
        if (sizeCheck == length_) return;
        
        long size = length_ + std::abs(static_cast<long>(sizeCheck) - static_cast<long>(length_)) + 1;
        vsnprintf(data_, size, str, list);
        
        length_ = strlen(data_);
        this->resize(length_); //trim
    }
    
    _String(const char *str, ...) noexcept {
        _constructString((char *)str);
        
        va_list list;
        va_start(list, str);
        
        Size sizeCheck = vsnprintf(data_, length_, (char *)str, list);
        va_end(list);
        
        strncpy(data_, str, length_); //since we used data_ as a buffer, reapply the old value back
        if (sizeCheck == length_) return;
        
        long size = length_ + (std::abs(static_cast<long>(length_) - static_cast<long>(sizeCheck)) + 1);
        if (size > capacity_) _allocateMemory(size);
            
            va_start(list, str);
            
            vsnprintf(data_, size, (char *)str, list);
            va_end(list);
            
            length_ = strlen(data_);
            this->resize(length_); //trim
            }
    
    _String(std::string str) noexcept {
        _constructString((char *)str.c_str());
    }
    
    _String(Size capacity) noexcept {
        _allocateMemory(capacity);
    }
    
    _String(const _String& str) noexcept {
        this->operator=(str);
    }
    
    static _String ContentsOfFile(const char *path) noexcept {
        FILE *file = fopen(path, "r");
        if (!file) {
            return "";
        }
        
        _String string;
        char *buffer;
        
        while (fread(buffer, 1, 4, file) > 0) {
            string.append(buffer);
        }
        
        return string;
    }
    
    static _String ContentsOfFile(const _String& path) noexcept {
        FILE *file = fopen(path.data_, "r");
        if (!file) return "";
        
        _String string;
        char *buffer;
        
        while (fread(buffer, 1, 4, file) == 4) {
            string.append(buffer);
        }
        
        return string;
    }
    
    static _String ContentsOfFile(FILE *file) noexcept {
        if (!file) return "";
        
        _String string;
        char *buffer;
        
        while (fread(buffer, 1, 4, file) == 4) {
            string.append(buffer);
        }
        
        return string;
    }
    
    ~_String() {
        allocator_.deallocate(data_, capacity_);
    }
    
    const inline char *data() const noexcept {
        return data_;
    }
    
    const inline char *c_str() const noexcept {
        char *data = _Allocator().allocate(length_ + 1);
        bzero(data, length_ + 1);
        
        strncpy(data, data_, length_);
        return (const char *)data;
    }
    
    const inline Size size() const noexcept {
        return length_;
    }
    
    const inline Size length() const noexcept {
        return length_;
    }
    
    const inline Size capacity() const noexcept {
        return capacity_;
    }
    
    const inline Size max_size() const noexcept {
        return UINT_MAX - 20000;
    }
    
    void resize (Size n) noexcept {
        if (n >= length_) _allocateMemory(n);
            }
    
    //TODO: if n < length_, resize string
    void resize (Size n, char c) noexcept {
        if (n <= capacity_) return;
        
        Size diff = n - length_;
        _allocateMemory(n);
        
        for (Size i = diff; i < length_; i++) data_[i] = c;
    }
    
    void reserve(Size n) noexcept {
        if (n > length_) _allocateMemory(n);
            }
    
    const bool empty() const noexcept {
        if (!length_) return true;
        
        for (Size len = 0; len < length_; len++) if (!isspace(data_[len])) return false;
        return true;
    }
    
    const char first() const noexcept {
        return data_[0];
    }
    
    const char last() const noexcept {
        return data_[length_ - 1];
    }
    
    const char at(Size i) const noexcept {
        if (i > length_ - 1) return '\0';
        return data_[i];
    }
    
    _String& append(char *str) noexcept {
        return append(str, strlen(str));
    }
    
    _String& append(const char *str) noexcept {
        return append((char *)str);
    }
    
    _String& append(std::string str) noexcept {
        return append((char *)str.c_str());
    }
    
    _String& append(const _String& str) noexcept {
        return append(str.data_);
    }
    
    void push_back(char c) noexcept {
        if ((capacity_ - length_) < 1) _allocateMemory(length_ + 1);
            
            data_[length_ - 1] = c;
        data_[length_] = '\0';
    }
    
    //append
    _String& append(const _String& str, size_t subpos, size_t sublen) noexcept {
        return *this;
    }
    
    _String& append(const char *str, size_t len) noexcept {
        Size prevLen = length_;
        Size totalLength = prevLen + len;
        
        if (totalLength >= capacity_) _allocateMemory(totalLength);
            for (Size i = prevLen; i < totalLength; i++) data_[i] = str[i - prevLen];
        
        data_[totalLength] = '\0';
        length_ = strlen(data_);
        
        return *this;
    }
    
    _String& append(Size len, char c) noexcept {
        Size prevLen = length_;
        Size totalLength = prevLen + len;
        
        if (totalLength >= capacity_) _allocateMemory(totalLength);
            for (Size i = prevLen; i < totalLength; i++) data_[i] = c;
        
        data_[totalLength] = '\0';
        length_ = strlen(data_);
        
        return *this;
    }
    
    _String& append(std::initializer_list<char> il) noexcept {
        Size prevLen = length_;
        Size totalLength = prevLen + il.size();
        
        if (totalLength >= capacity_) _allocateMemory(totalLength);
            
            Size i = prevLen;
        for (std::initializer_list<char>::const_iterator it = il.begin(); it != il.end(); it++) {
            data_[i] = *it;
            i++;
        }
        
        data_[length_] = '\0';
        return *this;
    }
    
    _String& appendFormat(char *str, ...) noexcept {
        va_list list;
        
        long size = length_ * 2;
        long final_size = size;
        
        char *buffer = new char[size];
        
        while (1) {
            Size len = length_;
            _allocateMemory(size);
            
            va_start(list, str);
            final_size = vsnprintf(&data_[len], size, (char *)str, list);
            
            va_end(list);
            if (final_size < 0 || final_size > size) {
                size += std::abs(final_size - size + 1);
                if (!realloc(buffer, size)) break;
            } else break;
        }
        
        return *this;
    }
    
    _String& appendFormat(const char *str, ...) noexcept {
        va_list list;
        
        long size = length_ * 2;
        long final_size = size;
        
        char *buffer = new char[size];
        
        while (1) {
            Size len = length_;
            _allocateMemory(size);
            
            va_start(list, str);
            final_size = vsnprintf(&data_[len], size, (char *)str, list);
            
            va_end(list);
            if (final_size < 0 || final_size > size) {
                size += std::abs(final_size - size + 1);
                if (!realloc(buffer, size)) break;
            } else break;
        }
        
        return *this;
    }
    
    _String stringByAppendingFormat(char *str, ...) const noexcept {
        va_list list;
        
        long size = length_ * 2;
        long final_size = size;
        
        _String string;
        
        while (1) {
            string.resize(size);
            
            va_start(list, str);
            final_size = vsnprintf(str, size, &string.data_[length_], list);
            
            va_end(list);
            if (final_size < 0 || final_size > size) size += std::abs(final_size - size + 1);
                else break;
        }
        
        return string;
    }
    
    _String stringByAppendingFormat(const char *str, ...) const noexcept {
        va_list list;
        
        long size = length_ * 2;
        long final_size = size;
        
        _String string = *this;
        
        while (1) {
            string.resize(size);
            
            va_start(list, str);
            final_size = vsnprintf(&string.data_[length_], size, str, list);
            
            va_end(list);
            if (final_size < 0 || final_size > size) size += std::abs(final_size - size + 1);
                else break;
        }
        
        return string;
    }
    
    _String& appendPathComponent(char *component) noexcept {
        return this->appendPathComponent((const char *)component);
    }
    
    _String& appendPathComponent(const char *component) noexcept {
        if (component[0] == '/') component = &component[1];
        if (data_[length_ - 1] != '/') this->append(1, '/');
            
            this->append(component);
            return *this;
    }
    
    _String& appendPathExtenstion(char *extension) noexcept {
        return this->appendPathExtenstion((const char *)extension);
    }
    
    _String& appendPathExtenstion(const char *extension) noexcept {
        if (data_[length_ - 1] != '.') this->append(1, '.');
            this->append(extension);
            
            return *this;
    }
    
    _String& deleteLastPathComponent() noexcept {
        Size i = this->find_last_of('/');
        if (i == NoPosition) return *this;
        
        return this->substrInPlace(0, i + 1);
    }
    
    _String stringByDeletingLastPathComponent() const noexcept {
        _String str = *this;
        
        Size i = str.find_last_of('/');
        if (i == NoPosition) return str;
        
        return str.substrInPlace(0, i + 1);
    }
    
    _String& deletePathExtension() noexcept {
        Size i = this->find_last_of('.');
        if (i == NoPosition) return *this;
        
        return this->substrInPlace(0, i);
    }
    
    _String stringByAppendingString(const char *string) const noexcept {
        _String str = *this;
        str.append(string);
        
        return str;
    }
    
    _String stringByAppendingString(const _String& string) const noexcept {
        _String str = *this;
        str.append(string.data_);
        
        return str;
    }
    
    _String stringByAppendingPathComponent(char *component) const noexcept {
        return this->stringByAppendingPathComponent((const char *)component);
    }
    
    _String stringByAppendingPathComponent(const char *component) const noexcept {
        _String str = *this;
        
        if (component[0] == '/') component = &component[1];
        if (str.data_[length_ - 1] != '/') str.append(1, '/');
            
            str.append(component);
            return str;
    }
    
    _String stringByAppendingPathExtension(char *extension) noexcept {
        return this->stringByAppendingPathExtension((const char *)extension);
    }
    
    _String stringByAppendingPathExtension(const char *extension) const noexcept {
        _String str = *this;
        
        if (data_[length_ - 1] != '.') str.append(1, '.');
            str.append(extension);
            
            return str;
    }
    
    _String stringByDeletingPathExtension() const noexcept {
        _String str = *this;
        
        Size i = str.find_last_of('.');
        if (i == NoPosition) return str;
        
        return str.substrInPlace(0, i);
    }
    
    _String lastPathComponent() const noexcept {
        Size i = this->find_last_of("/");
        if (i == NoPosition) return String(*this);
            
            if (i == length_ - 1) {
                for (Size j = i - 1; j < -1; j--) {
                    if (data_[j] != '/') continue;
                    
                    i = j + 1;
                    break;
                }
            }
        
        _String component = this->substr(i);
        component.removeCharacters("/");
        
        return component;
    }
    
    _String pathExtension() const noexcept {
        Size i = this->find_last_of('.');
        if (i == NoPosition) return *this;
        
        return this->substr(i + 1);
    }
    
    _String& remove(char c) noexcept {
        for (Size i = 0; i < length_; i++) {
            if (data_[i] == c) this->erase(i);
                }
        
        return *this;
    }
    
    _String stringByRemoving(char c) const noexcept {
        _String str = *this;
        
        for (Size i = 0; i < str.length_; i++) {
            if (str.data_[i] == c) str.erase(i);
                }
        
        return str;
    }
    
    _String& removeCharacters(char *characters) noexcept {
        return this->removeCharacters((const char *)characters);
    }
    
    _String& removeCharacters(const char *characters) noexcept {
        Size len = strlen(characters);
        for (Size i = 0; i < length_; i++) {
            for (Size j = 0; j < len; j++) if (data_[i] == characters[j]) this->erase(i);
                }
        
        return *this;
    }
    
    _String& removeCharacters(const _String& characters) noexcept {
        for (Size i = 0; i < length_; i++) {
            for (Size j = 0; j < characters.length_; j++) if (data_[i] == characters.data_[j]) this->erase(i);
                }
        
        return *this;
    }
    
    _String stringByRemovingCharacters(char *characters) const noexcept {
        return this->stringByRemovingCharacters((const char *)characters);
    }
    
    _String stringByRemovingCharacters(const char *characters) const noexcept {
        _String str = *this;
        Size len = strlen(characters);
        
        for (Size i = 0; i < str.length_; i++) {
            for (Size j = 0; j < len; j++) if (str.data_[i] == characters[j]) str.erase(i);
                }
        
        return str;
    }
    
    _String stringByRemovingCharacters(const _String& characters) const noexcept {
        _String str = *this;
        
        for (Size i = 0; i < str.length_; i++) {
            for (Size j = 0; j < characters.length_; j++) if (str.data_[i] == characters.data_[j]) str.erase(i);
                }
        
        return str;
    }
    
    //assign
    _String& assign(char *str) noexcept {
        return this->assign((const char *)str);
    }
    
    _String& assign(const _String& str) noexcept {
        _assignToSelf(str.data_, 0, str.length_);
        return *this;
    }
    
    _String& assign(char *str, Size subpos, Size sublen) noexcept {
        return assign((const char *)str, subpos, sublen);
    }
    
    _String& assign(const char *str, Size subpos, Size sublen) noexcept {
        _assignToSelf((char *)str, subpos, sublen);
        return *this;
    }
    
    _String& assign(const _String& str, Size subpos, Size sublen) noexcept {
        _assignToSelf(str.data_, subpos, sublen);
        return *this;
    }
    
    _String& assign(const char* s) noexcept {
        _assignToSelf((char *)s, 0, strlen(s));
        return *this;
    }
    
    _String& assign(const char* s, Size len) noexcept {
        _assignToSelf((char *)s, 0, len);
        return *this;
    }
    
    _String& assign(std::initializer_list<char> il) noexcept {
        if (il.size() > capacity_) _allocateMemory(length_ + il.size());
            
            Size i = 0;
        for (std::initializer_list<char>::const_iterator it = il.begin(); it != il.end(); it++) data_[i] = *it; i++;
        
        return *this;
    }
    
    //insert
    
    _String& insert(Size pos, const _String& str) noexcept {
        _insertIntoSelf(pos, str.data_, 0, str.length_);
        return *this;
    }
    
    _String& insert(Size pos, const _String& str, Size subpos, Size sublen) noexcept {
        _insertIntoSelf(pos, str.data_, subpos, sublen);
        return *this;
    }
    
    _String& insert(Size pos, const char *str) noexcept {
        _insertIntoSelf(pos, (char *)str, 0, strlen(str));
        return *this;
    }
    
    _String& insert(Size pos, const char *str, Size sublen) noexcept {
        _insertIntoSelf(pos, (char *)str, 0, sublen);
        return *this;
    }
    
    _String& insert(Size pos, Size len, char c) noexcept {
        _insertCharacterToSelf(pos, c, len);
        return *this;
    }
    
    _String& insert(Size pos, std::initializer_list<char> str) noexcept {
        if (pos > length_ - 1) return *this;
        
        Size totalLength = length_ + str.size();
        if (totalLength > capacity_) _allocateMemory(totalLength);
            
            Size i = pos;
        for (std::initializer_list<char>::const_iterator it = str.begin(); it != str.end(); it++) data_[i] = *it; i++;
        
        return *this;
    }
    
    //erase
    _String& erase(Size pos = 0, Size len = 0) noexcept {
        if (pos > length_ - 1 || len > (length_ - pos)) return *this;
        
        if (len == 0) data_[pos] = '\0';
        else memmove(&data_[pos], &data_[pos + len], length_ - len);
            
            length_ = strlen(data_);
            return *this;
    }
    
    //replace
    _String& replaceOccurrencesOf(const char *occurrence, const char *replacement) noexcept {
        Size occurrenceLength = strlen(occurrence);
        Size replacementLength = strlen(replacement);
        
        long diff = static_cast<long>(replacementLength) - static_cast<long>(occurrenceLength);
        for (Size i = 0; i < length_ - replacementLength; i++) {
            if (strncmp(&data_[i], occurrence, occurrenceLength) != 0) continue;
            
            if (diff <= 0) strncpy(&data_[i], replacement, replacementLength);
                else if (diff > 0) {
                    _allocateMemory(length_ + diff);
                    
                    insert(i + occurrenceLength, diff, ' ');
                    for (Size j = 0; j < replacementLength; j++) {
                        data_[j + i] = replacement[j];
                    }
                }
            
            if (diff < 0) {
                diff = -diff;
                
                Size index = i + replacementLength;
                memmove(&data_[index], &data_[index + diff], length_ - diff);
                
                length_ = strlen(data_);
            }
        }
        
        return *this;
    }
    
    _String& replaceOccurrencesOf(const _String& occurrence, const _String& replacement) noexcept {
        long diff = static_cast<long>(replacement.length_) - static_cast<long>(occurrence.length_);
        for (Size i = 0; i < length_ - replacement.length_; i++) {
            if (strncmp(&data_[i], occurrence.data_, occurrence.length_) != 0) continue;
            
            if (diff <= 0) strncpy(data_, replacement.data_, replacement.length_);
                else if (diff > 0) {
                    _allocateMemory(length_ + diff);
                    
                    insert(i + occurrence.length_, diff, ' ');
                    for (Size j = i; j < replacement.length_; j++) {
                        data_[j] = replacement.data_[j - i];
                    }
                }
            
            if (diff < 0) {
                diff = -diff;
                
                Size index = i + replacement.length_;
                memmove(&data_[index], &data_[index + diff], length_ - diff);
                
                length_ = strlen(data_);
            }
        }
        
        return *this;
    }
    
    void pop_back() noexcept {
        resize(length_ - 1);
    }
    
    Size copy(char *s, Size len, Size pos = 0) const noexcept {
        if (len > length_) len = length_;
        for (Size i = pos; i < len; i++)  s[i - pos] = data_[i];
        
        s[len] = '\0';
        return len;
    }
    
    //find
    static const Size NoPosition = -1;
    Size find(char c, Size pos = 0) const noexcept {
        for (Size i = pos; i < length_; i++) {
            if (data_[i] == c) return i;
        }
        
        return NoPosition;
    }
    
    Size find(char *str, Size pos = 0) const noexcept {
        return find((const char *)str, pos);
    }
    
    Size find(const char *str, Size pos = 0) const noexcept {
        if (pos > length_ - 1) return NoPosition;
        
        Size len = strlen(str);
        if (len > length_) return NoPosition;
        
        for (Size i = pos; i < length_; i++) {
            if (strncmp(&data_[i], str, len) == 0) return i;
        }
        
        return NoPosition;
    }
    
    Size find(_String& str, Size pos = 0) const noexcept {
        if (str.length_ > length_) return NoPosition;
        if (pos > length_ - 1) return NoPosition;
        
        for (Size i = pos; i < length_; i++) {
            if (strncmp(&data_[i], str.data_, str.length_) == 0) return i;
        }
        
        return NoPosition;
    }
    
    //find_last_of
    Size find_last_of(char c, Size pos = 0) const noexcept {
        for (Size i = length_; i < -1; i--) {
            if (data_[i] == c) return i;
        }
        
        return NoPosition;
    }
    
    Size find_last_of(char *str, Size pos = 0) const noexcept {
        return find_last_of((const char *)str, pos);
    }
    
    Size find_last_of(const char *str, Size pos = 0) const noexcept {
        Size len = strlen(str);
        if (len > length_) return NoPosition;
        
        for (Size i = length_; i - len < -1; i--) {
            if (strncmp(&data_[i - len], str, len) == 0) return i - 1;
        }
        
        return NoPosition;
    }
    
    Size find_last_of(const _String& str, Size pos = 0) const noexcept {
        Size len = str.length_;
        if (len > length_) return NoPosition;
        
        for (Size i = length_; i - len > -1; i--) {
            if (strncmp(&data_[i - len], str.data_, len) == 0) return length_ - i;
        }
        
        return NoPosition;
    }
    
    Size find_first_not_of(char *str, Size pos = 0) const noexcept {
        return this->find_first_not_of((const char *)str);
    }
    
    Size find_first_not_of(const char *str, Size pos = 0) const noexcept {
        Size len = strlen(str);
        for (Size i = pos; i < length_; i++) {
            bool found = false;
            for (Size j = 0; j < len; j++) {
                if (data_[i] != str[j]) continue;
                
                found = true;
                break;
            }
            
            if (!found) return i;
        }
        
        return NoPosition;
    }
    
    Size find_first_not_of(const _String& str, Size pos = 0) const noexcept {
        for (Size i = pos; i < length_; i++) {
            bool found = false;
            for (Size j = 0; j < str.length_; j++) {
                if (data_[i] != str.data_[j]) continue;
                
                found = true;
                break;
            }
            
            if (!found) return i;
        }
        
        return NoPosition;
    }
    
    Size find_last_not_of(const char *str, Size pos = 0) const noexcept {
        Size len = strlen(str);
        for (Size i = length_; i > pos; i--) {
            bool found = false;
            for (Size j = 0; j < len; j++) {
                if (data_[i] != str[j]) continue;
                
                found = true;
                break;
            }
            
            if (!found) return i;
        }
        
        return NoPosition;
    }
    
    Size find_last_not_of(const _String& str, Size pos = 0) const noexcept {
        for (Size i = str.length_; i > pos; i--) {
            bool found = false;
            for (Size j = 0; j < str.length_; j++) {
                if (data_[i] != str.data_[j]) continue;
                
                found = true;
                break;
            }
            
            if (!found) return i;
        }
        
        return NoPosition;
    }
    
    _String substr(Size pos = 0, Size len = NoPosition) const noexcept {
        if (len > length_ || pos > length_ - 1) len = length_;
        
        _String str(len);
        strncpy(str.data_, &data_[pos], len);
        
        if (len != length_) {
            str.data_[len] = '\0';
        }
        
        return str;
    }
    
    _String& substrInPlace(Size pos = 0, Size len = NoPosition) noexcept {
        if (len > length_) len = length_;
        if (pos > length_ - 1) return *this;
        
        if (pos != 0) strncpy(data_, &data_[pos], len);
            if (len != length_) data_[len] = '\0';
        
        return *this;
    }
    
    _String toLower() const noexcept {
        _String str = *this;
        
        for (Size i = 0; i < str.length_; i++) str.data_[i] = tolower(str.data_[i]);
            return str;
    }
    
    _String toUpper() const noexcept {
        _String str = *this;
        
        for (Size i = 0; i < str.length_; i++) str.data_[i] = toupper(str.data_[i]);
            return str;
    }
    
    int compare(char *str) const noexcept {
        return this->compare((const char *)str);
    }
    
    int compare(const char *str) const noexcept {
        return strcmp(data_, str);
    }
    
    int compare(const _String& str) const noexcept {
        return strcmp(data_, str.data_);
    }
    
    //NSString extras
    int caseInsensitiveCompare(char *str) const noexcept {
        return this->caseInsensitiveCompare((const char *)str);
    }
    
    int caseInsensitiveCompare(const char *str, Size len = NoPosition) const noexcept {
        if (len == NoPosition) len = strlen(str);
            if (empty() || !len) return -1;
        
        if (len > length_) len = length_;
        return strncasecmp(data_, str, len);
    }
    
    int caseInsensitiveCompare(const _String& str, Size len = NoPosition) const noexcept {
        if (len == NoPosition) len = str.length_;
        if (empty() || !len) return -1;
        
        if (len > length_) len = length_;
        return strncasecmp(data_, str, len);
    }
    
    bool contains(char *str) const noexcept {
        return this->contains((const char *)str);
    }
    
    bool contains(const char *str) const noexcept {
        return strstr(data_, str);
    }
    
    bool contains(const _String& str) const noexcept {
        return strstr(data_, str.data_);
    }
    
    bool writeToFile(char *path) const noexcept {
        return this->writeToFile((const char *)path);
    }
    
    bool writeToFile(const char *path) const noexcept {
        FILE *file = fopen(path, "w");
        if (!file) return false;
        
        return fwrite(data_, length_, 1, file) == 1;
    }
    
    bool writeToFile(const _String& path) const noexcept {
        FILE *file = fopen(path.data_, "w");
        if (!file) return false;
        
        return fwrite(data_, length_, 1, file) == 1;
    }
    
    bool writeToFile(FILE *file) const noexcept {
        if (!file) return false;
        return fwrite(data_, length_, 1, file) == 1;
    }
    
    bool hasPrefix(const char *prefix) const noexcept {
        Size len = strlen(prefix);
        if (len > length_) return false;
        
        return strncmp(data_, prefix, len) == 0;
    }
    
    bool hasPrefix(const _String& prefix) const noexcept {
        if (prefix.length_ > length_) return false;
        return strncmp(data_, prefix.data_, prefix.length_) == 0;
    }
    
    bool hasSuffix(const char *suffix) const noexcept {
        Size len = strlen(suffix);
        if (len > length_) return false;
        
        return strncmp(&data_[length_ - len], suffix, len) == 0;
    }
    
    char *begin() noexcept {
        return &data_[0];
    }
    
    char *end() noexcept {
        return &data_[length_];
    }
    
    ReverseIterator rbegin() const noexcept {
        return std::reverse_iterator<char *>(&data_[length_]);
    }
    
    ReverseIterator rend() const noexcept {
        return std::reverse_iterator<char *>(&data_[0]);
    }
    
    operator char *() const noexcept {
        return data_;
    }
    
    operator const char *() const noexcept {
        return data_;
    }
    
    operator std::string() const noexcept {
        return std::string(data_);
    }
    
    void operator=(char *str) noexcept {
        this->operator=((const char *)str);
    }
    
    void operator=(const char *str) noexcept {
        Size len = strlen(str);
        if (len >= capacity_) _allocateMemory(len);
            
            strncpy(data_, str, len);
            }
    
    void operator=(const _String& str) noexcept {
        capacity_ = str.capacity_;
        allocator_ = _Allocator();
        data_ = allocator_.allocate(capacity_);
        
        length_ = str.length_;
        if (length_) strncpy(data_, str.data_, length_);
            
            this->resize(length_);
            }
    
    bool operator==(char *str) const noexcept {
        return this->operator==((const char *)str);
    }
    
    bool operator==(const char *str) const noexcept {
        return compare(str) == 0;
    }
    
    bool operator==(const _String& rhs) const noexcept {
        return compare(rhs) == 0;
    }
    
    bool operator!=(char *str) const noexcept {
        return this->operator!=((const char *)str);
    }
    
    bool operator!=(const char *str) const noexcept {
        return !compare(str);
    }
    
    bool operator!=(const _String& rhs) const noexcept {
        return !compare(rhs);
    }
    
    bool operator<(char *str) const noexcept {
        return compare(str) < 0;
    }
    
    bool operator<(const char *str) const noexcept {
        return compare(str) < 0;
    }
    
    bool operator<(const _String& lhs) const noexcept {
        return compare(lhs.data_) < 0;
    }
    
    bool operator>(char *str) const noexcept {
        return compare(str) > 0;
    }
    
    bool operator>(const char *str) const noexcept {
        return compare(str) > 0;
    }
    
    bool operator>(const _String& lhs) const noexcept {
        return compare(lhs.data_) > 0;
    }
    
    char operator[](Size i) const noexcept {
        return this->at(i);
    }
    
    _String operator+(char *str) const noexcept {
        return this->operator+((const char *)str);
    }
    
    _String operator+(const char *str) const noexcept {
        _String string = *this;
        string.append(str);
        
        return string;
    }
    
    _String operator+(const _String& str) const noexcept {
        _String string = *this;
        string.append(str);
        
        return string;
    }
    
    _String& operator+=(char c) noexcept {
        return this->append(1, c);
    }
    
    _String& operator+=(char *str) noexcept {
        return this->append(str);
    }
    
    _String& operator+=(const char *str) noexcept {
        return this->append(str);
    }
    
    _String& operator+=(const _String& str) noexcept {
        return this->append(str);
    }
    
    friend std::ostream& operator<< (std::ostream &lhs, const _String &rhs);
    friend std::iostream& operator<< (std::iostream &lhs, const _String &rhs);
    friend std::ostringstream& operator<< (std::ostringstream &lhs, const _String &rhs);
private:
    char *data_ = nullptr;
    
    Size length_ = 0;
    Size capacity_ = 0;
    
    _Allocator allocator_;
    
    void _constructString(char *str) {
        _allocateMemory(strlen(str));
        strncpy(data_, str, length_);
    }
    
    void _allocateMemory(Size n) {
        if (n < length_) return;
        
        length_ = n;
        capacity_ = length_ + 5;
        
        char *result = nullptr;
        if (capacity_ >= max_size()) {
            capacity_ = 5;
        }
        
        result = allocator_.allocate(capacity_);
        
        if (data_ && n >= length_) strncpy(result, data_, length_);
        else bzero(result, n);
        
        data_ = result;
        data_[n] = '\0';
    }
    
    void _checkCapacity(Size len) {
        if (capacity_ - 5 < len) _allocateMemory(len);
    }
    
    void _insertIntoSelf(Size pos, char *str, Size subpos, Size len) {
        if (pos > length_ - 1) return;
        
        Size totalLength = length_ + len;
        if (totalLength > capacity_) _allocateMemory(totalLength);
        
        Size strLength = strlen(str);
        if (subpos > strLength - 1 || len > (strLength - subpos)) return;
        
        memmove(&data_[pos + len], &data_[pos], length_ - (len - 1));
        
        for (Size i = subpos, j = pos; i < len; i++, j++) data_[j] = str[i];
        
        data_[totalLength] = '\0';
        length_ = strlen(data_);
    }
    
    void _insertCharacterToSelf(Size pos, char c, Size len) {
        if (pos > length_ - 1) return;
        
        Size totalLength = length_ + len;
        if (totalLength > capacity_) _allocateMemory(totalLength);
        
        memmove(&data_[pos + 1], &data_[pos], length_ - (len - 1));
        for (Size i = 0; i < len; i++) data_[i + pos] = c;
        
        length_ = strlen(data_);
    }
    
    void _assignToSelf(char *str, Size start, Size len) {
        Size strLength = strlen(str);
        
        if (start > strLength - 1) return;
        if (len > strLength) return;
        
        if (len > capacity_) _allocateMemory(length_ + len);
        
        if (start != 0) for (Size i = start; i < len; i++) data_[i] = str[i - start];
        
        data_[len] = '\0';
        length_ = strlen(data_);
    }
    
    void _replaceOntoSelf(Size pos, size_t len, char *str, Size subpos, Size sublen) {
        if (pos > length_ - 1 || len > (length_ - pos)) return;
        
        Size strLength = strlen(str);
        if (subpos > strLength - 1 || sublen > (strLength - pos)) return;
        
        Size totalLength = length_ + sublen;
        if (totalLength < capacity_) _allocateMemory(totalLength);
        
        for (Size i = pos, j = subpos; i < len && j < strLength; i++, j++) data_[i] = str[j];
    }
};

typedef _String<std::allocator<char>> String;

#endif /* String_hpp */

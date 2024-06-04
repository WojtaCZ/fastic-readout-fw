/* 
 * This file is part of the stm-cpp distribution (https://github.com/WojtaCZ/stm-cpp).
 * Copyright (c) 2024 Vojtech Vosahlo.
 * 
 * This program is free software: you can redistribute it and/or modify  
 * it under the terms of the GNU General Public License as published by  
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef REGISTER_H
#define REGISTER_H

#include <cstdint>
#include <cstddef>


/*
 * The following file contains the helper function templates
 * used for generic register access throughout the rest of this project.
 */

namespace reg{

    /// @brief Writes the \p value_ into a register specified by the \p address_
    /// @tparam address_ Address of the register
    /// @param value_ Value to be written into the register
    template<uint32_t address_>
    void write(uint32_t value_){
        *reinterpret_cast<volatile uint32_t *>(address_) = value_;
    }

    /// @brief Reads a value from the register specified by \p address_
    /// @tparam address_ Address of the register
    /// @return Value of the register
    template<uint32_t address_>
    uint32_t read(){
        return *reinterpret_cast<volatile uint32_t *>(address_);
    }

    /// @brief Reads a value from the register specified by \p address_ and masks it with the specified \p mask_
    /// @tparam address_ Address of the register
    /// @param mask_ Mask to be applied to the resulting value
    /// @return Masked value of the register
    template<uint32_t address_>
    uint32_t read(uint32_t mask_){
        return (*reinterpret_cast<volatile uint32_t *>(address_)) & mask_;
    }

    /// @brief Sets the bits specified by \p mask_ in the register specified by \p address_
    /// @tparam address_ Address of the register
    /// @param mask_ The bitmask to be ORed with the register value
    template<uint32_t address_>
    void set(uint32_t mask_){
        *reinterpret_cast<volatile uint32_t *>(address_) |= mask_; 
    }

    /// @brief Clears the bits specified by \p mask_ in the register specified by \p address_
    /// @tparam address_ Address of the register
    /// @param mask_ The bitmask to be inverted and ANDed with the register value
    template<uint32_t address_>
    void clear(uint32_t mask_){
        *reinterpret_cast<volatile uint32_t *>(address_) &= ~mask_;
    }

    /// @brief Clears the bits specified by \p mask_ in the register specified by \p address_
    /// @tparam address_ Address of the register
    /// @param mask_ The bitmask to be XORed with the register value
    template<uint32_t address_>
    void toggle(uint32_t mask_){
        *reinterpret_cast<volatile uint32_t *>(address_) ^= mask_;
    }
}

#endif

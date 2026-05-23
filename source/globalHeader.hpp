#pragma once

#include <cstdint>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <utility>
#include <array>
#include <print>
#include <string>
#include <optional>
#include <exception>
#include <nds.h>

#include <NEAMain.h>
#include <filesystem.h>
#include <fat.h>


enum class ClassStates
{
    Init,
    Playing,
    Exit
};

enum class MenusStates
{
    MainMenu,
    PartyGame
};
enum class CardType : int
{
    Negative_2 = -2,
    Negative_1 = -1,
    Neutral_0 = 0,
    Positive_1 = 1,
    Positive_2 = 2,
    Positive_3 = 3,
    Positive_4 = 4,
    Positive_5 = 5,
    Positive_6 = 6,
    Positive_7 = 7,
    Positive_8 = 8,
    Positive_9 = 9,
    Positive_10 = 10,
    Positive_11 = 11,
    Positive_12 = 12
};

// Skyjo game contain 150card in the which repartition
//                       [CardType] [Number in package]
using CardPairs = std::pair<CardType, uint8_t>;

constexpr std::array<CardPairs, 15> cardPackage = {{
                                            {CardType::Negative_2, 5},
                                            {CardType::Negative_1, 10},
                                            {CardType::Neutral_0, 15},
                                            {CardType::Positive_1, 10},
                                            {CardType::Positive_2, 10},
                                            {CardType::Positive_3, 10},
                                            {CardType::Positive_4, 10},
                                            {CardType::Positive_5, 10},
                                            {CardType::Positive_6, 10},
                                            {CardType::Positive_7, 10},
                                            {CardType::Positive_8, 10},
                                            {CardType::Positive_9, 10},
                                            {CardType::Positive_10, 10},
                                            {CardType::Positive_11, 10},
                                            {CardType::Positive_12, 10}
                                        }};


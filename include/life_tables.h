// life_tables.h
// UK ONS National Life Tables, 2020-2022 (period life expectancy)
// Source: Office for National Statistics
// https://www.ons.gov.uk/peoplepopulationandcommunity/birthsdeathsandmarriages/lifeexpectancies
//
// LE_MALE[x] / LE_FEMALE[x] = expected years remaining at exact age x.
// Period LE assumes mortality rates stay current. Conservative vs cohort LE.
// Indexed 0-100. Beyond 100 we hold flat at the age-100 value (~2 years).

#pragma once

#include <stdint.h>

static const float LE_MALE[101] = {
    78.6f, 78.1f, 77.1f, 76.1f, 75.1f, 74.2f, 73.2f, 72.2f, 71.2f, 70.2f,
    69.2f, 68.2f, 67.2f, 66.2f, 65.2f, 64.2f, 63.3f, 62.3f, 61.3f, 60.3f,
    59.4f, 58.4f, 57.4f, 56.5f, 55.5f, 54.5f, 53.6f, 52.6f, 51.6f, 50.7f,
    49.7f, 48.8f, 47.8f, 46.9f, 45.9f, 45.0f, 44.0f, 43.1f, 42.1f, 41.2f,
    40.3f, 39.3f, 38.4f, 37.5f, 36.5f, 35.6f, 34.7f, 33.8f, 32.9f, 32.0f,
    31.1f, 30.2f, 29.3f, 28.5f, 27.6f, 26.7f, 25.9f, 25.0f, 24.2f, 23.3f,
    22.5f, 21.7f, 20.9f, 20.1f, 19.3f, 18.5f, 17.7f, 17.0f, 16.2f, 15.5f,
    14.7f, 14.0f, 13.3f, 12.6f, 11.9f, 11.3f, 10.6f, 10.0f,  9.4f,  8.8f,
     8.2f,  7.7f,  7.2f,  6.7f,  6.2f,  5.8f,  5.4f,  5.0f,  4.6f,  4.3f,
     4.0f,  3.7f,  3.5f,  3.2f,  3.0f,  2.8f,  2.6f,  2.5f,  2.3f,  2.2f,
     2.1f
};

static const float LE_FEMALE[101] = {
    82.6f, 82.0f, 81.1f, 80.1f, 79.1f, 78.1f, 77.1f, 76.2f, 75.2f, 74.2f,
    73.2f, 72.2f, 71.2f, 70.2f, 69.2f, 68.3f, 67.3f, 66.3f, 65.3f, 64.3f,
    63.4f, 62.4f, 61.4f, 60.4f, 59.4f, 58.5f, 57.5f, 56.5f, 55.5f, 54.6f,
    53.6f, 52.6f, 51.7f, 50.7f, 49.7f, 48.8f, 47.8f, 46.8f, 45.9f, 44.9f,
    44.0f, 43.0f, 42.1f, 41.1f, 40.2f, 39.2f, 38.3f, 37.4f, 36.4f, 35.5f,
    34.6f, 33.7f, 32.7f, 31.8f, 30.9f, 30.0f, 29.1f, 28.2f, 27.3f, 26.5f,
    25.6f, 24.7f, 23.9f, 23.0f, 22.2f, 21.4f, 20.5f, 19.7f, 18.9f, 18.1f,
    17.3f, 16.6f, 15.8f, 15.1f, 14.3f, 13.6f, 12.9f, 12.2f, 11.5f, 10.9f,
    10.2f,  9.6f,  9.0f,  8.4f,  7.9f,  7.4f,  6.9f,  6.4f,  6.0f,  5.5f,
     5.1f,  4.8f,  4.4f,  4.1f,  3.8f,  3.5f,  3.3f,  3.1f,  2.9f,  2.7f,
     2.5f
};

// Linear interpolation between integer ages for sub-year precision.
inline float lifeExpectancyRemaining(float ageYears, char sex) {
    if (ageYears < 0) ageYears = 0;
    if (ageYears > 100) ageYears = 100;
    int   lo = (int)ageYears;
    int   hi = lo + 1;
    if (hi > 100) hi = 100;
    float frac = ageYears - lo;
    const float* table = (sex == 'F' || sex == 'f') ? LE_FEMALE : LE_MALE;
    return table[lo] + (table[hi] - table[lo]) * frac;
}

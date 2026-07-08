#include "data/nmea_parser.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <stdexcept>
#include <string>
#include <vector>

namespace eexi_cii {
namespace {

std::string trim_line_end(std::string value) {
    while (!value.empty()) {
        const unsigned char last = static_cast<unsigned char>(value.back());
        if (last != '\r' && last != '\n' && !std::isspace(last)) {
            break;
        }
        value.pop_back();
    }

    return value;
}

std::vector<std::string> split_csv(const std::string& value) {
    std::vector<std::string> fields;
    std::string current;

    for (const char ch : value) {
        if (ch == ',') {
            fields.push_back(current);
            current.clear();
        } else {
            current.push_back(ch);
        }
    }

    fields.push_back(current);
    return fields;
}

bool is_supported_rmc_type(const std::string& type) {
    if (type.size() != 5 || type.substr(2) != "RMC") {
        return false;
    }

    const std::string talker = type.substr(0, 2);
    return talker == "GP" || talker == "GN" || talker == "GL" ||
           talker == "GA" || talker == "GB";
}

double parse_number(const std::string& value) {
    std::size_t consumed = 0;
    const double parsed = std::stod(value, &consumed);
    if (consumed != value.size()) {
        throw std::invalid_argument("Unexpected numeric suffix");
    }

    return parsed;
}

double parse_utc_seconds(const std::string& value) {
    if (value.size() < 6) {
        throw std::invalid_argument("RMC time must include hhmmss");
    }

    const int hours = std::stoi(value.substr(0, 2));
    const int minutes = std::stoi(value.substr(2, 2));
    const double seconds = parse_number(value.substr(4));

    if (hours < 0 || hours > 23 || minutes < 0 || minutes > 59 ||
        seconds < 0.0 || seconds >= 60.0) {
        throw std::invalid_argument("RMC time out of range");
    }

    return hours * 3600.0 + minutes * 60.0 + seconds;
}

double parse_coordinate(const std::string& value, const std::string& direction) {
    if (value.empty() || direction.size() != 1) {
        throw std::invalid_argument("Incomplete coordinate");
    }

    const double raw = parse_number(value);
    const double degrees = std::floor(raw / 100.0);
    const double minutes = raw - degrees * 100.0;
    double decimal = degrees + minutes / 60.0;

    if (minutes < 0.0 || minutes >= 60.0) {
        throw std::invalid_argument("Coordinate minutes out of range");
    }

    if (direction == "S" || direction == "W") {
        decimal = -decimal;
    } else if (direction != "N" && direction != "E") {
        throw std::invalid_argument("Unsupported coordinate direction");
    }

    return decimal;
}

int parse_full_year(const std::string& date) {
    const int yy = std::stoi(date.substr(4, 2));
    return yy >= 80 ? 1900 + yy : 2000 + yy;
}

} // namespace

bool validate_checksum(const std::string& sentence) {
    const std::string value = trim_line_end(sentence);
    if (value.size() < 4 || value.front() != '$') {
        return false;
    }

    const std::size_t star = value.find('*');
    if (star == std::string::npos || star + 2 >= value.size()) {
        return false;
    }

    unsigned char checksum = 0;
    for (std::size_t i = 1; i < star; ++i) {
        checksum ^= static_cast<unsigned char>(value[i]);
    }

    const std::string hex = value.substr(star + 1, 2);
    if (!std::isxdigit(static_cast<unsigned char>(hex[0])) ||
        !std::isxdigit(static_cast<unsigned char>(hex[1]))) {
        return false;
    }

    const auto expected = static_cast<unsigned char>(std::stoul(hex, nullptr, 16));
    return checksum == expected;
}

RMCData parse_rmc(const std::string& sentence) {
    RMCData data;
    const std::string value = trim_line_end(sentence);

    if (!validate_checksum(value)) {
        return data;
    }

    const std::size_t star = value.find('*');
    const std::string body = value.substr(1, star - 1);
    const std::vector<std::string> fields = split_csv(body);

    if (fields.size() < 10 || !is_supported_rmc_type(fields[0])) {
        return data;
    }

    if (fields[2] != "A" || fields[7].empty()) {
        return data;
    }

    try {
        data.utc_seconds = parse_utc_seconds(fields[1]);
        data.latitude = parse_coordinate(fields[3], fields[4]);
        data.longitude = parse_coordinate(fields[5], fields[6]);
        data.sog_knots = parse_number(fields[7]);
        data.cog_degrees = fields[8].empty() ? 0.0 : parse_number(fields[8]);

        if (fields[9].size() != 6) {
            return RMCData{};
        }

        data.day = std::stoi(fields[9].substr(0, 2));
        data.month = std::stoi(fields[9].substr(2, 2));
        data.year = parse_full_year(fields[9]);

        if (data.day < 1 || data.day > 31 || data.month < 1 || data.month > 12 ||
            data.sog_knots < 0.0) {
            return RMCData{};
        }
    } catch (const std::exception&) {
        return RMCData{};
    }

    data.valid = true;
    return data;
}

} // namespace eexi_cii

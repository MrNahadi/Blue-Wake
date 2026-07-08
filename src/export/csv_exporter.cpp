#include "export/csv_exporter.h"

#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>

namespace eexi_cii {
namespace {

std::string csv_escape(const std::string& value) {
    bool needs_quotes = false;
    for (const char ch : value) {
        if (ch == ',' || ch == '"' || ch == '\r' || ch == '\n') {
            needs_quotes = true;
            break;
        }
    }

    if (!needs_quotes) {
        return value;
    }

    std::string escaped = "\"";
    for (const char ch : value) {
        if (ch == '"') {
            escaped += "\"\"";
        } else {
            escaped += ch;
        }
    }
    escaped += '"';
    return escaped;
}

std::string format_number(const double value) {
    std::ostringstream out;
    out << std::fixed << std::setprecision(6) << value;
    std::string formatted = out.str();

    while (!formatted.empty() && formatted.back() == '0') {
        formatted.pop_back();
    }
    if (!formatted.empty() && formatted.back() == '.') {
        formatted.pop_back();
    }
    if (formatted == "-0") {
        return "0";
    }

    return formatted;
}

std::string ship_type_name(const ShipType type) {
    switch (type) {
    case ShipType::BulkCarrier:
        return "Bulk Carrier";
    case ShipType::GasCarrierGte65k:
        return "Gas Carrier >=65k DWT";
    case ShipType::GasCarrierLt65k:
        return "Gas Carrier <65k DWT";
    case ShipType::Tanker:
        return "Tanker";
    case ShipType::ContainerShip:
        return "Container Ship";
    case ShipType::GeneralCargo:
        return "General Cargo";
    case ShipType::RefrigeratedCargo:
        return "Refrigerated Cargo Carrier";
    case ShipType::CombinationCarrier:
        return "Combination Carrier";
    case ShipType::LngCarrierGte100k:
        return "LNG Carrier >=100k DWT";
    case ShipType::LngCarrierLt100k:
        return "LNG Carrier <100k DWT";
    case ShipType::RoroVehicleCarrier:
        return "Ro-ro Cargo Ship (Vehicle Carrier)";
    case ShipType::RoroCargo:
        return "Ro-ro Cargo Ship";
    case ShipType::RoroPassenger:
        return "Ro-ro Passenger Ship";
    case ShipType::CruisePassenger:
        return "Cruise Passenger Ship";
    case ShipType::Count:
        break;
    }

    throw std::invalid_argument("Unknown ship type");
}

std::string capacity_type(const VesselProfile& profile) {
    return uses_cgdist(profile.ship_type) ? "GT" : "DWT";
}

std::int64_t floor_div(const std::int64_t value, const std::int64_t divisor) {
    std::int64_t quotient = value / divisor;
    const std::int64_t remainder = value % divisor;
    if (remainder != 0 && ((remainder < 0) != (divisor < 0))) {
        --quotient;
    }
    return quotient;
}

void civil_from_days(std::int64_t days, int& year, unsigned& month, unsigned& day) {
    days += 719468;
    const std::int64_t era = (days >= 0 ? days : days - 146096) / 146097;
    const unsigned doe = static_cast<unsigned>(days - era * 146097);
    const unsigned yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    year = static_cast<int>(yoe) + static_cast<int>(era) * 400;
    const unsigned doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    const unsigned mp = (5 * doy + 2) / 153;
    day = doy - (153 * mp + 2) / 5 + 1;
    month = mp + (mp < 10 ? 3 : -9);
    year += month <= 2 ? 1 : 0;
}

std::string format_timestamp(const std::int64_t timestamp) {
    if (timestamp <= 0) {
        return "";
    }

    const std::int64_t days = floor_div(timestamp, 86400);
    const std::int64_t seconds_of_day = timestamp - days * 86400;

    int year = 0;
    unsigned month = 0;
    unsigned day = 0;
    civil_from_days(days, year, month, day);

    const int hour = static_cast<int>(seconds_of_day / 3600);
    const int minute = static_cast<int>((seconds_of_day % 3600) / 60);
    const int second = static_cast<int>(seconds_of_day % 60);

    std::ostringstream out;
    out << std::setfill('0') << std::setw(4) << year << '-'
        << std::setw(2) << month << '-' << std::setw(2) << day << 'T'
        << std::setw(2) << hour << ':' << std::setw(2) << minute << ':'
        << std::setw(2) << second << 'Z';
    return out.str();
}

} // namespace

std::string export_annual_csv(
    const YTDState& ytd,
    const VesselProfile& profile,
    const AERResult& aer_result
) {
    std::ostringstream csv;
    csv << "Year,Ship Name,IMO Number,Ship Type,Capacity Type,Capacity,Total Distance (nm),"
           "Total CO2 (tonnes),AER or cgDIST,CII Rating,Required CII\n";
    csv << ytd.year << ','
        << csv_escape(profile.name) << ','
        << csv_escape(profile.imo_number) << ','
        << csv_escape(ship_type_name(profile.ship_type)) << ','
        << capacity_type(profile) << ','
        << format_number(profile.cii_capacity()) << ','
        << format_number(ytd.distance_nm) << ','
        << format_number(ytd.co2_tonnes) << ','
        << format_number(aer_result.aer) << ','
        << aer_result.rating << ','
        << format_number(aer_result.required_cii) << '\n';
    return csv.str();
}

std::string export_voyage_csv(const std::vector<VoyageRecord>& records) {
    std::ostringstream csv;
    csv << "Voyage,Start,End,Displacement,Distance (nm),CO2 (tonnes),AER,Type\n";

    for (const VoyageRecord& record : records) {
        csv << csv_escape(record.is_unassigned ? "Unassigned" : record.name) << ','
            << format_timestamp(record.start_timestamp) << ','
            << format_timestamp(record.end_timestamp) << ','
            << format_number(record.displacement) << ','
            << format_number(record.distance_nm) << ','
            << format_number(record.co2_tonnes) << ','
            << format_number(record.aer) << ','
            << (record.is_unassigned ? "Unassigned" : "Named") << '\n';
    }

    return csv.str();
}

} // namespace eexi_cii

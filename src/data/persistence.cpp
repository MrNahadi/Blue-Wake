#include "data/persistence.h"

#include "nlohmann/json.hpp"

#include <fstream>
#include <stdexcept>

namespace eexi_cii {
namespace {

using nlohmann::json;

json rmc_to_json(const RMCData& rmc) {
    return json{
        {"valid", rmc.valid},
        {"utc_seconds", rmc.utc_seconds},
        {"latitude", rmc.latitude},
        {"longitude", rmc.longitude},
        {"sog_knots", rmc.sog_knots},
        {"cog_degrees", rmc.cog_degrees},
        {"day", rmc.day},
        {"month", rmc.month},
        {"year", rmc.year}
    };
}

RMCData rmc_from_json(const json& value) {
    RMCData rmc;
    rmc.valid = value.at("valid").get<bool>();
    rmc.utc_seconds = value.at("utc_seconds").get<double>();
    rmc.latitude = value.at("latitude").get<double>();
    rmc.longitude = value.at("longitude").get<double>();
    rmc.sog_knots = value.at("sog_knots").get<double>();
    rmc.cog_degrees = value.at("cog_degrees").get<double>();
    rmc.day = value.at("day").get<int>();
    rmc.month = value.at("month").get<int>();
    rmc.year = value.at("year").get<int>();
    return rmc;
}

json voyage_state_to_json(const VoyageState& value) {
    return json{
        {"name", value.name},
        {"displacement", value.displacement},
        {"distance_nm", value.distance_nm},
        {"co2_tonnes", value.co2_tonnes},
        {"elapsed_seconds", value.elapsed_seconds},
        {"start_timestamp", value.start_timestamp},
        {"active", value.active}
    };
}

VoyageState voyage_state_from_json(const json& value) {
    VoyageState state;
    state.name = value.at("name").get<std::string>();
    state.displacement = value.at("displacement").get<double>();
    state.distance_nm = value.at("distance_nm").get<double>();
    state.co2_tonnes = value.at("co2_tonnes").get<double>();
    state.elapsed_seconds = value.at("elapsed_seconds").get<double>();
    state.start_timestamp = value.at("start_timestamp").get<std::int64_t>();
    state.active = value.at("active").get<bool>();
    return state;
}

json voyage_record_to_json(const VoyageRecord& value) {
    return json{
        {"name", value.name},
        {"start_timestamp", value.start_timestamp},
        {"end_timestamp", value.end_timestamp},
        {"displacement", value.displacement},
        {"distance_nm", value.distance_nm},
        {"co2_tonnes", value.co2_tonnes},
        {"aer", value.aer},
        {"is_unassigned", value.is_unassigned}
    };
}

VoyageRecord voyage_record_from_json(const json& value) {
    VoyageRecord record;
    record.name = value.at("name").get<std::string>();
    record.start_timestamp = value.at("start_timestamp").get<std::int64_t>();
    record.end_timestamp = value.at("end_timestamp").get<std::int64_t>();
    record.displacement = value.at("displacement").get<double>();
    record.distance_nm = value.at("distance_nm").get<double>();
    record.co2_tonnes = value.at("co2_tonnes").get<double>();
    record.aer = value.at("aer").get<double>();
    record.is_unassigned = value.at("is_unassigned").get<bool>();
    return record;
}

json ytd_to_json(const YTDState& value) {
    return json{
        {"year", value.year},
        {"distance_nm", value.distance_nm},
        {"co2_tonnes", value.co2_tonnes},
        {"seed_distance_nm", value.seed_distance_nm},
        {"seed_co2_tonnes", value.seed_co2_tonnes}
    };
}

YTDState ytd_from_json(const json& value) {
    YTDState ytd;
    ytd.year = value.at("year").get<int>();
    ytd.distance_nm = value.at("distance_nm").get<double>();
    ytd.co2_tonnes = value.at("co2_tonnes").get<double>();
    ytd.seed_distance_nm = value.at("seed_distance_nm").get<double>();
    ytd.seed_co2_tonnes = value.at("seed_co2_tonnes").get<double>();
    return ytd;
}

} // namespace

std::string serialize_accumulator(const Accumulator& accumulator) {
    const AccumulatorState state = accumulator.state();

    json history = json::array();
    for (const auto& record : state.history) {
        history.push_back(voyage_record_to_json(record));
    }

    json archived_years = json::array();
    for (const auto& year : state.archived_years) {
        archived_years.push_back(ytd_to_json(year));
    }

    const json document{
        {"schema", "eexi_cii_accumulator_v1"},
        {"current_voyage", voyage_state_to_json(state.current_voyage)},
        {"ytd", ytd_to_json(state.ytd)},
        {"last_fix", rmc_to_json(state.last_fix)},
        {"last_timestamp", state.last_timestamp},
        {"has_last_fix", state.has_last_fix},
        {"last_fix_can_accumulate", state.last_fix_can_accumulate},
        {"history", history},
        {"archived_years", archived_years}
    };

    return document.dump(2);
}

Accumulator deserialize_accumulator(const std::string& text) {
    const json document = json::parse(text);
    if (document.at("schema").get<std::string>() != "eexi_cii_accumulator_v1") {
        throw std::invalid_argument("Unsupported accumulator persistence schema");
    }

    AccumulatorState state;
    state.current_voyage = voyage_state_from_json(document.at("current_voyage"));
    state.ytd = ytd_from_json(document.at("ytd"));
    state.last_fix = rmc_from_json(document.at("last_fix"));
    state.last_timestamp = document.at("last_timestamp").get<std::int64_t>();
    state.has_last_fix = document.at("has_last_fix").get<bool>();
    state.last_fix_can_accumulate = document.at("last_fix_can_accumulate").get<bool>();

    for (const auto& item : document.at("history")) {
        state.history.push_back(voyage_record_from_json(item));
    }
    for (const auto& item : document.at("archived_years")) {
        state.archived_years.push_back(ytd_from_json(item));
    }

    Accumulator accumulator;
    accumulator.restore_state(state);
    return accumulator;
}

void save_accumulator(const std::string& filepath, const Accumulator& accumulator) {
    std::ofstream out(filepath);
    if (!out) {
        throw std::runtime_error("Could not open accumulator file for writing");
    }
    out << serialize_accumulator(accumulator);
}

Accumulator load_accumulator(const std::string& filepath) {
    std::ifstream in(filepath);
    if (!in) {
        throw std::runtime_error("Could not open accumulator file for reading");
    }
    return deserialize_accumulator(std::string(
        std::istreambuf_iterator<char>(in),
        std::istreambuf_iterator<char>()
    ));
}

} // namespace eexi_cii

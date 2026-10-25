#include "CalibrationStore.hpp"

#include <algorithm>
#include <array>
#include <cinttypes>
#include <cmath>
#include <memory>

#include "SDK/JSON/JsonStreamReader.hpp"
#include "SDK/JSON/JsonStreamWriter.hpp"

bool CalibrationStore::load()
{
    mEntries.clear();

    auto file = mKernel.fs.file(mPath);
    if (!file || !file->exist() || !file->open(false, false)) {
        return false;
    }

    const std::size_t size = file->size();
    std::unique_ptr<char[]> buffer(size > 0 ? new (std::nothrow) char[size] : nullptr);
    std::size_t read = 0;
    const bool ok = buffer && file->read(buffer.get(), size, read) && read == size;
    file->close();
    if (!ok) {
        return false;
    }

    SDK::JsonStreamReader reader(buffer.get(), size);
    if (!reader.validate()) {
        return false;
    }

    uint32_t count = 0;
    reader.get("count", count);
    for (uint32_t i = 0; i < std::min<uint32_t>(count, kCapacity); ++i) {
        const auto field = [i](const char* name) {
            std::array<char, 40> query{};
            snprintf(query.data(), query.size(), "entries[%" PRIu32 "].%s", i, name);
            return query;
        };
        Entry   entry{};
        uint8_t source = static_cast<uint8_t>(Source::IndoorCadence);
        reader.get(field("lap_m").data(),  entry.key.lapLengthM);
        reader.get(field("lane").data(),   entry.key.lane);
        reader.get(field("source").data(), source);
        reader.get(field("k").data(),      entry.state.k);
        reader.get(field("var").data(),    entry.state.variance);

        if (source <= static_cast<uint8_t>(Source::OutdoorGps)) {
            entry.key.source = static_cast<Source>(source);
            mEntries.push_back(entry);
        }
    }
    return true;
}

bool CalibrationStore::save() const
{
    auto file = mKernel.fs.file(mPath);
    if (!file || !file->open(true, true)) {
        return false;
    }

    SDK::JsonStreamWriter writer(file.get());
    writer.startMap();
    writer.add("count", static_cast<uint32_t>(mEntries.size()));
    writer.startArray("entries");
    for (const Entry& entry : mEntries) {
        writer.startMap();
        writer.add("lap_m",  entry.key.lapLengthM);
        writer.add("lane",   entry.key.lane);
        writer.add("source", static_cast<uint8_t>(entry.key.source));
        writer.add("k",      entry.state.k);
        writer.add("var",    entry.state.variance);
        writer.endMap();
    }
    writer.endArray();
    writer.endMap();

    const bool flushed = file->flush();
    return file->close() && flushed;
}

SegmentCorrector::State CalibrationStore::prior(const Key& key) const
{
    const auto entry = find(key);
    if (entry == mEntries.end()) {
        return {};
    }
    return { entry->state.k, std::max(entry->state.variance, kReturningVariance) };
}

void CalibrationStore::remember(const Key& key, SegmentCorrector::State state)
{
    const auto existing = find(key);
    if (existing != mEntries.end()) {
        mEntries.erase(existing);
    } else if (mEntries.size() == kCapacity) {
        mEntries.erase(mEntries.begin());
    }
    mEntries.push_back({ key, state });
}

std::vector<CalibrationStore::Entry>::const_iterator CalibrationStore::find(const Key& key) const
{
    return std::find_if(mEntries.begin(), mEntries.end(), [&](const Entry& e) {
        return std::fabs(e.key.lapLengthM - key.lapLengthM) < 0.01f && e.key.lane == key.lane &&
               e.key.source == key.source;
    });
}

#include "CalibrationStore.hpp"

#include <algorithm>
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
        char query[32];
        Entry entry{};
        uint8_t lane = 0;

        snprintf(query, sizeof(query), "entries[%" PRIu32 "].lap_m", i);
        reader.get(query, entry.lapLengthM);
        snprintf(query, sizeof(query), "entries[%" PRIu32 "].lane", i);
        reader.get(query, lane);
        snprintf(query, sizeof(query), "entries[%" PRIu32 "].k", i);
        reader.get(query, entry.state.k);
        snprintf(query, sizeof(query), "entries[%" PRIu32 "].var", i);
        reader.get(query, entry.state.variance);

        entry.lane = lane;
        mEntries.push_back(entry);
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
        writer.add("lap_m", entry.lapLengthM);
        writer.add("lane",  entry.lane);
        writer.add("k",     entry.state.k);
        writer.add("var",   entry.state.variance);
        writer.endMap();
    }
    writer.endArray();
    writer.endMap();

    const bool flushed = file->flush();
    return file->close() && flushed;
}

SegmentCorrector::State CalibrationStore::prior(float lapLengthM, uint8_t lane) const
{
    const auto entry = find(lapLengthM, lane);
    if (entry == mEntries.end()) {
        return {};
    }
    return { entry->state.k, std::max(entry->state.variance, kReturningVariance) };
}

void CalibrationStore::remember(float lapLengthM, uint8_t lane, SegmentCorrector::State state)
{
    const auto existing = find(lapLengthM, lane);
    if (existing != mEntries.end()) {
        mEntries.erase(existing);
    } else if (mEntries.size() == kCapacity) {
        mEntries.erase(mEntries.begin());
    }
    mEntries.push_back({ lapLengthM, lane, state });
}

std::vector<CalibrationStore::Entry>::const_iterator CalibrationStore::find(float lapLengthM, uint8_t lane) const
{
    return std::find_if(mEntries.begin(), mEntries.end(), [&](const Entry& e) {
        return std::fabs(e.lapLengthM - lapLengthM) < 0.01f && e.lane == lane;
    });
}

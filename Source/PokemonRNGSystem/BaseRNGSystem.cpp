#include "BaseRNGSystem.h"

#include <algorithm>
#include <cstring>
#include <fstream>
#include <sstream>
#include <map>
#include <set>

std::string BaseRNGSystem::getPrecalcFilenameForSettings(const bool useWii,
                                                         const int rtcErrorMarginSeconds)
{
  std::stringstream ss;
  ss << (useWii ? "Wii" : "GC");
  ss << '-';
  ss << rtcErrorMarginSeconds;
  return ss.str();
}

BaseRNGSystem::seedRange BaseRNGSystem::getRangeForSettings(const bool useWii,
                                                            const int rtcErrorMarginSeconds)
{
  seedRange range;
  int ticksPerSecond = useWii ? Common::ticksPerSecondWii : Common::ticksPerSecondGC;
  if (rtcErrorMarginSeconds == 0)
  {
    range.min = 0;
    range.max = 0x100000000;
  }
  else
  {
    range.min = useWii ? minRTCTicksToBootWii : minRTCTicksToBootGC;
    range.max = range.min + ticksPerSecond * rtcErrorMarginSeconds;
  }
  return range;
}

size_t BaseRNGSystem::getPracalcFileSize(const bool useWii, const int rtcErrorMarginSeconds)
{
  seedRange range = getRangeForSettings(useWii, rtcErrorMarginSeconds);
  return (range.max - range.min) * (sizeof(u16) + sizeof(u8));
}

void BaseRNGSystem::precalculateNbrRollsBeforeTeamGeneration(
    const bool useWii, const int rtcErrorMarginSeconds, const u32 numSeeds,
    std::function<void(long)> progressUpdate, std::function<bool()> shouldCancelNow)
{
  seedRange range = getRangeForSettings(useWii, rtcErrorMarginSeconds);
  long nbrSeedsPrecalculatedTotal = 0;
  int seedsPrecalculatedCurrentBlock = 0;
  bool hasCancelled = false;
  std::map<u8, std::vector<u32>> seedsMap;
  for (u8 i = 0; i < 24; i++) {
    seedsMap[i].reserve(numSeeds/20);
  }
#pragma omp parallel for
  for (s64 i = range.min; i < range.max; i++)
  {
    if (shouldCancelNow())
    {
      hasCancelled = true;
      continue;
    }
    u32 seed = 0;
    seed = rollRNGToBattleMenu(static_cast<u32>(i));
    std::vector<int> criteria = obtainTeamGenerationCritera(seed);
    u8 compactedCriteria = (criteria[0] & 7) | (criteria[1] << 3);
#pragma omp critical (storeSeed)
      seedsMap[compactedCriteria].push_back(seed);
#pragma omp critical (progressPrecalc)
    {
      nbrSeedsPrecalculatedTotal++;
      seedsPrecalculatedCurrentBlock++;
      if (seedsPrecalculatedCurrentBlock >= 10000)
      {
        progressUpdate(nbrSeedsPrecalculatedTotal);
        seedsPrecalculatedCurrentBlock = 0;
      }
    }
  }
  if (!hasCancelled) {
    for (u8 i = 0; i < 24; i++) {
      std::sort(seedsMap[i].begin(), seedsMap[i].end());
      auto last = std::unique(seedsMap[i].begin(), seedsMap[i].end());
      seedsMap[i].erase(last, seedsMap[i].end());
    }
    std::string filename = getPrecalcFilenameForSettings(useWii, rtcErrorMarginSeconds);
    std::ofstream precalcFile(filename, std::ios::binary | std::ios::out);
    for (u8 i = 0; i < 24; i++) {
      u32 size = seedsMap[i].size();
      precalcFile.write(reinterpret_cast<const char*>(&size), sizeof(u32));
    }
    for (u8 i = 0; i < 24; i++) {
      for (auto seed : seedsMap[i]) {
        precalcFile.write(reinterpret_cast<const char *>(&seed), sizeof(u32));
      }
    }
    precalcFile.close();
  }
}

void BaseRNGSystem::seedFinderPass(const std::vector<int> criteria, std::vector<u32>& seeds,
                                   const bool useWii, const int rtcErrorMarginSeconds,
                                   const bool usePrecalc, std::function<void(long)> progressUpdate,
                                   std::function<bool()> shouldCancelNow)
{
  std::vector<u32> newSeeds;
  seedRange range;
  range.max = seeds.size();
  if (seeds.size() == 0) {
    std::ifstream precalcFile(getPrecalcFilenameForSettings(useWii, rtcErrorMarginSeconds),
                              std::ios::binary | std::ios::in);
    bool actuallyUsePrecalc = usePrecalc && precalcFile.good();
    if (actuallyUsePrecalc) {
      u32 offsets[24];
      for (u8 i = 0; i < 24; i++) {
        precalcFile.read(reinterpret_cast<char *>(&(offsets[i])), sizeof(u32));
      }
      u8 chosenCriteria = (criteria[0] & 7) | (criteria[1] << 3);
      u32 offsetSeeds = 0;
      for (u8 i = 0; i < chosenCriteria; i++) {
        offsetSeeds += offsets[i];
      }
      precalcFile.seekg(offsetSeeds * sizeof(u32), std::ios_base::cur);
      for (u32 i = 0; i < offsets[chosenCriteria]; i++) {
        u32 seed;
        precalcFile.read(reinterpret_cast<char *>(&seed), sizeof(u32));
        seeds.push_back(seed);
      }
      return;
    } else {
      range = getRangeForSettings(useWii, rtcErrorMarginSeconds);
    }
  }
  long nbrSeedsSimulatedTotal = 0;
  int seedsSimulatedCurrentBlock = 0;
#pragma omp parallel for
  for (s64 i = range.min; i < range.max; i++)
  {
    // This is probably the most awkward way to do this, but it can't be done properly with OpenMP
    // because it requires to set an environement variable which cannot be set after the program is
    // started
    if (shouldCancelNow()) {
      continue;
    }

    u32 seed = 0;
    bool goodSeed = false;
    if (seeds.size() == 0)
    {
      seed = rollRNGToBattleMenu(static_cast<u32>(i));
      goodSeed = generateBattleTeam(seed, criteria);
    }
    else
    {
      seed = seeds[i];
      goodSeed = generateBattleTeam(seed, criteria);
    }
    if (goodSeed)
#pragma omp critical(addSeed)
      newSeeds.push_back(seed);
#pragma omp critical(progress)
    {
      nbrSeedsSimulatedTotal++;
      seedsSimulatedCurrentBlock++;
      if (seedsSimulatedCurrentBlock >= 10000)
      {
        progressUpdate(nbrSeedsSimulatedTotal);
        seedsSimulatedCurrentBlock = 0;
      }
    }
  }
  std::swap(newSeeds, seeds);
  // As the number of calls differs depending on the starting seed, it may happen that some seeds
  // may end up being the same as another one, this gets rid of the duplicates so the program can
  // only have one unique result at the end
  std::sort(seeds.begin(), seeds.end());
  auto last = std::unique(seeds.begin(), seeds.end());
  seeds.erase(last, seeds.end());
}

std::vector<BaseRNGSystem::StartersPrediction>
BaseRNGSystem::predictStartersForNbrSeconds(u32 seed, const int nbrSeconds)
{
  std::vector<StartersPrediction> predictionsResult;
  seed = rollRNGNamingScreenInit(seed);

  for (int i = getMinFramesAmountNamingScreen();
       i < getMinFramesAmountNamingScreen() + nbrSeconds * pollingRateNamingScreenPerSec; i++)
  {
    seed = rollRNGNamingScreenNext(seed);
    BaseRNGSystem::StartersPrediction prediction = generateStarterPokemons(seed);
    prediction.frameNumber = i;
    predictionsResult.push_back(prediction);
  }
  return predictionsResult;
}

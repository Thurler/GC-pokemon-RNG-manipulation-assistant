// The natures wanted for all Pokemon of all ultimate teams
constant uchar c_natureTeamsData[8][6] = 
      {{0x16, 0x15, 0x0f, 0x13, 0x04, 0x04},
      {0x0b, 0x08, 0x01, 0x10, 0x10, 0x0C},
      {0x02, 0x10, 0x0f, 0x12, 0x0f, 0x03},
      {0x10, 0x13, 0x03, 0x16, 0x03, 0x18},
      {0x11, 0x10, 0x0f, 0x13, 0x05, 0x04},
      {0x0f, 0x11, 0x01, 0x03, 0x13, 0x03},
      {0x01, 0x08, 0x03, 0x01, 0x03, 0x03},
      {0x03, 0x0a, 0x0f, 0x03, 0x0f, 0x03}};

// The genders wanted for all Pokemon of all ultimate teams
constant uchar c_genderTeamsData[8][6] = 
      {{0, 1, 1, 0, 0, 1},
      {2, 1, 0, 0, 1, 0},
      {0, 1, 0, 1, 0, 1},
      {2, 1, 1, 1, 0, 0},
      {0, 0, 0, 0, 0, 1},
      {2, 1, 2, 0, 2, 1},
      {2, 0, 0, 1, 1, 0},
      {1, 0, 1, 0, 1, 0}};

// The genders wanted for dummy Pokemon of dummy teams
constant uchar c_genderDummyTeamsData[4][6] = 
    {{1, 0, 1, 0, 1, 0}, {1, 0, 1, 0, 1, 0}, {1, 0, 1, 0, 1, 0}, {1, 0, 1, 0, 1, 0}};

// The gender ratios of all Pokemon of all ultimate teams
constant uchar c_genderRatioTeamsData[8][6] = 
     {{0x1f, 0x7f, 0x7f, 0x7f, 0xbf, 0x7f},
     {0xff, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f},
     {0x1f, 0x3f, 0x7f, 0x7f, 0x7f, 0x7f},
     {0xff, 0xbf, 0x7f, 0x7f, 0x1f, 0x7f},
     {0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x7f},
     {0xff, 0x7f, 0xff, 0x7f, 0xff, 0x7f},
     {0xff, 0x1f, 0x3f, 0x7f, 0x7f, 0x3f},
     {0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f}};

// The gender ratios of all Pokemon of all dummy teams
constant uchar c_genderRatioDummyTeamsData[4][6] = 
      {{0x3f, 0x3f, 0x1f, 0x1f, 0x1f, 0x1f},
      {0x1f, 0x7f, 0x1f, 0x7f, 0x7f, 0x7f},
      {0xbf, 0x1f, 0x7f, 0x7f, 0x7f, 0x7f},
      {0x7f, 0x7f, 0xff, 0x7f, 0x7f, 0x7f}};

// Alogorithm that returns (base^exp) mod 2^32 in the complexity O(log n)
uint inline modpow32(uint base, uint exp)
{
  uint result = 1;
  while (exp > 0)
  {
    if (exp & 1)
      result = result * base;
    base = base * base;
    exp >>= 1;
  }
  return result;
}

// The LCG used in both Pokemon games
uint inline LCG(uint* seed)
{
  *seed = *seed * 0x343fd + 0x269EC3;
  return *seed;
}

// Apply the LCG n times in O(log n) complexity
uint inline LCGn(uint seed, const uint n)
{
  uint ex = n - 1;
  uint q = 0x343fd;
  uint factor = 1;
  uint sum = 0;
  while (ex > 0)
  {
    if (!(ex & 1))
    {
      sum = sum + (factor * modpow32(q, ex));
      ex--;
    }
    factor *= (1 + q);
    q *= q;
    ex /= 2;
  }
  seed = (seed * modpow32(0x343fd, n)) + (sum + factor) * 0x269EC3;
  return seed;
}

bool inline isPidShiny(const ushort TID, const ushort SID, const uint PID)
{
  return ((TID ^ SID ^ (PID & 0xFFFF) ^ (PID >> 16)) < 8);
}

int inline getPidGender(const uchar genderRatio, const uint pid)
{
  return genderRatio > (pid & 0xff) ? 1 : 0;
}

uint inline generatePokemonPID(uint* seed, const uint hTrainerId,
                               const uint lTrainerId, const uint dummyId,
                               const char wantedGender, const uint genderRatio, 
                               const char wantedNature)
{
  uint id = 0;
  bool goodId = false;
  while (!goodId)
  {
    // A personality ID is generated as candidate, high then low 16 bits
    uint hId = LCG(seed) >> 16;
    uint lId = LCG(seed) >> 16;
    id = (hId << 16) | (lId);

    // If we want a gender AND the gender of the pokemon is uncertain
    if (wantedGender != -1 && !(genderRatio == 0xff || genderRatio == 0xfe || genderRatio == 0x00))
    {
      if (wantedGender == 2)
      {
        uchar pokemonIdGender = getPidGender(genderRatio, dummyId);
        uchar idGender = getPidGender(genderRatio, id);
        if (pokemonIdGender != idGender)
          continue;
      }
      else
      {
        uchar idGender = getPidGender(genderRatio, id);
        if (wantedGender != idGender)
          continue;
      }
    }

    // Reroll until we have the correct nature in the perosnality ID
    if (wantedNature != -1 && id % 25 != wantedNature)
      continue;
    // This is apparently preventing a shiny personality ID...in the most convoluted way ever!
    if (!isPidShiny(lTrainerId, hTrainerId, id))
      goodId = true;
  }
  return id;
}

uint inline rollRNGToPokemonCompanyLogo(uint seed)
{
  // The game generates 500 numbers of 32 bit in a row, this is 2 LCG call per number which makes
  // 1000 calls
  seed = LCGn(seed, 1000);
  // A personality ID is generated as candidate, low then high 16 bits
  uint lTrainerId = LCG(&seed) >> 16;
  uint hTrainerId = LCG(&seed) >> 16;

  for (int i = 0; i < 2; i++)
  {
    // A personality ID is generated as candidate, high then low 16 bits
    uint hDummyId = LCG(&seed) >> 16;
    uint lDummyId = LCG(&seed) >> 16;
    uint dummyId = (hDummyId << 16) | (lDummyId);

    // These calls seems to generate some IV and a 50/50, they don't actually matter for the rest of
    // the calls
    LCG(&seed);
    LCG(&seed);
    LCG(&seed);
    generatePokemonPID(&seed, hTrainerId, lTrainerId, dummyId, 0, 0x1f, -1);
  }

  // These calls don't matter
  LCG(&seed);
  LCG(&seed);

  return seed;
}

uint inline rollRNGEnteringBattleMenu(uint seed)
{
  seed = LCGn(seed, 120);
  for (int i = 0; i < 4; i++)
  {
    // A trainer ID is generated, low then high 16 bits
    uint lTrainerId = LCG(&seed) >> 16;
    uint hTrainerId = LCG(&seed) >> 16;
    for (int j = 0; j < 7; j++)
    {
      // For some reasons, the last call of all the 24 call to the perosnality ID generation is
      // different
      if (j == 6 && i != 3)
        continue;
      // A personality ID is generated as candidate, high then low 16 bits
      uint hDummyId = LCG(&seed) >> 16;
      uint lDummyId = LCG(&seed) >> 16;
      uint dummyId = (hDummyId << 16) | (lDummyId);

      // These calls generate the IV and the ability, they don't actually matter for the rest
      // of the calls
      LCG(&seed);
      LCG(&seed);
      LCG(&seed);
      if (j == 6)
      {
        generatePokemonPID(&seed, hTrainerId, lTrainerId, dummyId, -1, 257, -1);
      }
      else
      {
        generatePokemonPID(&seed, hTrainerId, lTrainerId, dummyId,
                           c_genderDummyTeamsData[i][j], c_genderRatioDummyTeamsData[i][j], -1);
      }
    }
  }
  return seed;
}

bool generateBattleTeam(uint* seed, global const int* criteria)
{
  int enemyTeamIndex = (LCG(seed) >> 16) & 7;
  int playerTeamIndex = -1;
  // The game generates a player team index as long as it is not the same as the enemy one
  do
  {
    playerTeamIndex = (LCG(seed) >> 16) & 7;
  } while (enemyTeamIndex == playerTeamIndex);

  if (playerTeamIndex != criteria[0] && criteria[0] != -1)
    return false;

  // The enemy trainer ID is generated as candidate, low then high 16 bits
  uint lTrainerId = LCG(seed) >> 16;
  uint hTrainerId = LCG(seed) >> 16;
  // For each enemy pokemon
  for (int i = 0; i < 6; i++)
  {
    // A dummy perosnality ID is generated, high then low 16 bits
    uint hDummyId = LCG(seed) >> 16;
    uint lDummyId = LCG(seed) >> 16;
    uint dummyId = (hDummyId << 16) | (lDummyId);

    // These calls generate the IV and the ability, they don't actually matter for the rest
    // of the calls
    LCG(seed);
    LCG(seed);
    LCG(seed);
    generatePokemonPID(
        seed, hTrainerId, lTrainerId, dummyId, c_genderTeamsData[enemyTeamIndex][i],
        c_genderRatioTeamsData[enemyTeamIndex][i], c_natureTeamsData[enemyTeamIndex][i]);
  }

  if ((LCG(seed) >> 16) % 3 != criteria[1] && criteria[1] != -1)
    return false;

  // The player trainer ID is generated, low then high 16 bits
  lTrainerId = LCG(seed) >> 16;
  hTrainerId = LCG(seed) >> 16;
  // For each player pokemon
  for (int i = 0; i < 6; i++)
  {
    // A dummy personality ID is generated, high then low 16 bits
    uint hDummyId = LCG(seed) >> 16;
    uint lDummyId = LCG(seed) >> 16;
    uint dummyId = (hDummyId << 16) | (lDummyId);

    // These calls generate the IV and the ability, they don't actually matter for the rest
    // of the calls
    LCG(seed);
    LCG(seed);
    LCG(seed);
    generatePokemonPID(
        seed, hTrainerId, lTrainerId, dummyId, c_genderTeamsData[playerTeamIndex][i],
        c_genderRatioTeamsData[playerTeamIndex][i], c_natureTeamsData[playerTeamIndex][i]);
  }
  return true;
}

void kernel seedFinder(global const int* criteria, global uint* seeds, global uint* rangeOffset)
{
  uint seed = get_global_id(0) + *rangeOffset;
  seed = rollRNGToPokemonCompanyLogo(seed);
  seed = rollRNGEnteringBattleMenu(seed);
  if (generateBattleTeam(&seed, criteria))
  {
    seeds[get_global_id(0)] = seed;
  }
}

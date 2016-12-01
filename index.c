#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

typedef struct 
{
  uint32_t len; 
  uint32_t mask;
  uint64_t byte_hash[256];
  uint64_t bits[];
} index_t;

typedef struct
{
  uint32_t start;
  uint32_t len;
  uint64_t hash;
} index_result_t;

static inline uint64_t ror(uint64_t n)
{
  return (n >> 1) | (n << 63);
}

static uint64_t random_uint64(uint64_t *s)
{
  // xorshift128plus random number generator.
  uint64_t x = s[0];
  uint64_t const y = s[1];
  s[0] = y;
  x ^= x << 23; // a
  s[1] = x ^ y ^ (x >> 17) ^ (y >> 26); // b, c
  return s[1] + y;
}

static void fill_random_uint64(uint64_t *array, uint32_t len, uint64_t seed) 
{
  uint64_t s[2] = { seed , 0xDEADBEEF };
  for (int i = 0 ; i < 64 ; i++)
  {
    /* ignore few of the first random values */
    random_uint64(s);
  }
  for (int i = 0 ; i < len ; i++)
  { 
    array[i] = random_uint64(s); 
  }
}

static inline void bitset(uint64_t* bits, const uint32_t position)
{
  const uint32_t i = position >> 6;
  const uint32_t j = position & 63;
  bits[i] |= UINT64_C(1) << j;
}

static inline uint32_t bittest(const uint64_t* bits, const uint32_t position)
{
  const uint32_t i = position >> 6;
  const uint32_t j = position & 63;
  return (bits[i] >> j) & 1; 
}

index_t* index_allocate(uint32_t bits_requried)
{
  uint32_t mask = bits_requried - 1;
  uint32_t longs = bits_requried / 64;
  index_t* index = (index_t*) malloc(sizeof(index_t) + longs);
  index->len = longs;
  index->mask = mask;
  return index;
}

void index_init(index_t* index, uint64_t seed)
{ 
  uint64_t* byte_hash = index->byte_hash;
  fill_random_uint64(byte_hash, 256, seed);
}

uint32_t index_add_word(index_t* index, const char *word)
{
  uint32_t mask = index->mask;
  uint64_t* bits = index->bits;
  uint64_t* byte_hash = index->byte_hash;
  uint64_t hash = 0;
  while (*word)
  {
    const uint32_t c = (uint32_t)*word;
    hash = ror(hash) ^ byte_hash[c & 0xff];
    uint32_t pos = hash & mask;
    bitset(bits, pos);
    word++;
  }
  uint32_t final_index = (hash + 1) & mask;
  bitset(bits, final_index & mask);
  return hash;
}

uint32_t index_search(index_t* index,
                      char* input,
                      index_result_t *results,
                      uint32_t max_results)
{
  const uint32_t mask = index->mask;
  const uint64_t* bits = index->bits;
  const uint64_t* byte_hash = index->byte_hash;

  uint32_t input_position = 0;
  uint32_t result_index = 0;
  while (*input)
  {
    char *substring = input;
    uint64_t hash = 0;
    uint32_t len = 0;
    while (*substring)
    {
      uint32_t c = (uint32_t) *substring;
      hash = ror(hash) ^ byte_hash[c & 0xff];
      uint32_t pos = hash & mask;
      substring++;
      len++;

      if (!bittest(bits, pos))
        break;

      if (!bittest(bits, (hash + 1) & mask))
        continue;

      // possible match
      results[result_index].start = input_position;
      results[result_index].len = len;
      results[result_index].hash = hash;
      result_index++;
      if (result_index > max_results)
      {
        return max_results;
      }
    }
    input++;
    input_position++;
  }
  return result_index;
} 

void index_free(index_t* index)
{
  free(index);
}



int main(int argc, char **argv)
{
  index_t* index = index_allocate(1 << 22);
  index_init(index, 1997);
  
  FILE *fp = fopen("/usr/share/dict/words", "r");
  char buf[4096];
  while (fgets(buf, sizeof(buf)-1, fp)) {
    size_t n = strlen(buf);
    if (n >= 4)
    {
      buf[n-1]=0;
      index_add_word(index,buf);
    }
  }
  fclose(fp);
  printf("index build done!\n");

  index_result_t res[10];
  char *word = "worldly";
  uint32_t count = index_search(index, word, res, 10); 
  for (int i = 0 ; i < count ; i++)
  {
      printf("possible match at pos=%d of len=%d with hash=%llu\n", res[i].start, res[i].len, res[i].hash);
  }
  index_free(index);
}


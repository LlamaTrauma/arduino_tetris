uint32_t rand_next = 1;

uint8_t rand8 () // RAND_MAX assumed to be 32767
{
    rand_next = rand_next * 1103515245 + 12345;
    return (rand_next/65536) % 32768;
}

void srand8 (uint32_t seed)
{
    rand_next = seed;
}
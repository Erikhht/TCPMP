#define STEREO_SOUND

#ifdef _M_IX86
#define WORDS_UNALIGNED_OK
#endif

#ifdef CONFIG_BIG_ENDIAN
#define WORDS_BIGENDIAN
#endif

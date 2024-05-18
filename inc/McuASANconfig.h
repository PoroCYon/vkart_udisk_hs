
#ifndef MCUASANCONFIG_H_
#define MCUASANCONFIG_H_

#ifndef McuASAN_CONFIG_IS_ENABLED
  #define McuASAN_CONFIG_IS_ENABLED     (0)
  /*!< 1: ASAN is enabled; 0: ASAN is disabled */
#endif

#ifndef McuASAN_CONFIG_CHECK_MALLOC_FREE
  #define McuASAN_CONFIG_CHECK_MALLOC_FREE  (1)
  /*!< 1: check malloc() and free() */
#endif

#ifndef McuASAN_CONFIG_APP_MEM_START
  #define McuASAN_CONFIG_APP_MEM_START 0x20000000
  /*!< base RAM address */
#endif

#ifndef McuASAN_CONFIG_APP_MEM_SIZE
  #define McuASAN_CONFIG_APP_MEM_SIZE  (32*1024)
  /*!< Memory size in bytes */
#endif

#ifndef McuASAN_CONFIG_PRGM_MEM_START
  #define McuASAN_CONFIG_PRGM_MEM_START 0x00000000
  /*!< base RAM address */
#endif

#ifndef McuASAN_CONFIG_PRGM_MEM_SIZE
  #define McuASAN_CONFIG_PRGM_MEM_SIZE  (0x20000000)
  /*!< Memory size in bytes */
#endif

#ifndef McuASAN_CONFIG_IO_MEM_START
  #define McuASAN_CONFIG_IO_MEM_START 0x40000000
  /*!< base RAM address */
#endif

#ifndef McuASAN_CONFIG_IO_MEM_SIZE
  #define McuASAN_CONFIG_IO_MEM_SIZE  (0xb0000000)
  /*!< Memory size in bytes */
#endif

#if McuASAN_CONFIG_CHECK_MALLOC_FREE
#ifndef McuASAN_CONFIG_MALLOC_RED_ZONE_BORDER
  #define McuASAN_CONFIG_MALLOC_RED_ZONE_BORDER  (8)
  /*!< red zone border in bytes around memory blocks. Must be larger than sizeof(size_t)! */
#endif

#ifndef McuASAN_CONFIG_FREE_QUARANTINE_LIST_SIZE
  #define McuASAN_CONFIG_FREE_QUARANTINE_LIST_SIZE  (8)
  /*!< list of free blocks in quarantine until they are released. Use 0 for no list. */
#endif

#endif /* McuASAN_CONFIG_CHECK_MALLOC_FREE */

#endif /* MCUASANCONFIG_H_ */


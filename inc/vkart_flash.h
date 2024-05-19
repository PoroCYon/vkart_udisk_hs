
#ifndef USER_VKART__FLASH_C_
#define USER_VKART__FLASH_C_

#include <stdint.h>
#include <stdbool.h>

#define VKART_MEMORY_WORDSZ    (128*32*1024 /* 128 pages of 32 KiW each */)
#define VKART_BUFFER_WORDSZ    (4096 /* small page size; also max size we can buffer */)

extern uint16_t vkart_data_buffer[VKART_BUFFER_WORDSZ];


bool vkart_init(void);
void vkart_read_data(uint32_t addr, uint16_t *pbuff, uint32_t len);
void vkart_erase_sector(uint32_t addr, uint8_t block);
void vkart_write_data(const uint16_t* pbuf, uint32_t address, uint32_t len);

bool vkart_wrimage_start(void);
bool vkart_wrimage_next(const uint16_t* pbuf, uint32_t len);
void vkart_wrimage_finish(void);

#endif /* USER_VKART__FLASH_C_ */


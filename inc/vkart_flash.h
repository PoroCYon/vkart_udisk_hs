
#ifndef USER_VKART__FLASH_C_
#define USER_VKART__FLASH_C_

#include <stdint.h>
#include <stdbool.h>

#define VKART_ERASEBLK_WORDSZ  (32*1024)
#define VKART_BUFFER_WORDSZ    (VKART_ERASEBLK_WORDSZ >> 2)
#define VKART_MEMORY_WORDSZ   (128*VKART_ERASEBLK_WORDSZ)


extern uint16_t vkart_data_buffer[VKART_BUFFER_WORDSZ];

bool VKART_Init(void);
void vkart_read_data(uint32_t addr, uint16_t *pbuff, uint32_t len);
void vkart_erase_sector(uint32_t addr, uint8_t block);
void vkart_write_block(uint16_t *pbuf, uint32_t address, uint32_t len);

#endif /* USER_VKART__FLASH_C_ */


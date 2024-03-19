#ifndef USER_VKART__FLASH_C_
#define USER_VKART__FLASH_C_

#ifdef __cplusplus
 extern "C" {
#endif 

#define VKART_FLASH_PAGE_SIZE  512
#define VKART_UDISK_START_ADDR   0x0

#define VKART_UDISK_END_ADDR     0x7FFFFF
//#define VKART_UDISK_END_ADDR     0x512
#define VKART_UDISK_SIZE         (VKART_UDISK_END_ADDR - VKART_UDISK_START_ADDR + 1 )

extern void VKART_Init(void);
extern void vkart_read_data(uint32_t addr, uint16_t *pbuff, uint32_t len);
extern void vkart_erase_sector(uint32_t addr, uint8_t block);
extern void vkart_write_block(uint16_t *pbuf, uint32_t address, uint32_t len);

void set_ce(uint8_t state);
void set_rw(uint8_t state);
void led(uint8_t state);
void set_data_dir(uint8_t state);
void set_address(uint32_t address);
void set_data(uint16_t data);
uint16_t get_data(void);
void write_word(uint32_t address, uint16_t word);
void write_word_mx(uint32_t addr, uint16_t d1);
void write_word_29w(uint32_t addr, uint16_t d1, uint16_t d2);
void erase_block(uint32_t addr);
void erase_chip();
uint16_t read_word(uint32_t address);
uint16_t get_device_id(void);
void do_reset();


#ifdef __cplusplus
}
#endif

#endif /* USER_VKART__FLASH_C_ */

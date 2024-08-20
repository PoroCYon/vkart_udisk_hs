
#include "ch32v30x.h"
#include "vkart_flash.h"
#include "util.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>


static void set_ce(uint8_t state);
static void set_rw(uint8_t state);
static void set_data_dir(uint8_t state);
static void set_address(uint32_t address);
static void set_data(uint16_t data);
static uint16_t get_data(void);
static void write_word(uint32_t address, uint16_t word);
static void write_word_mx(uint32_t addr, uint16_t d1);
static void write_word_29w(uint32_t addr, uint16_t d1, uint16_t d2);
static void erase_block(uint32_t addr);
//static void erase_chip();
static uint16_t read_word(uint32_t address);
static uint16_t get_device_id(void);
static void do_reset(void);

/*
 * MX commands
 * 
 * 000,F0			Reset
 * 555,AA 2AA,55 555,90		Manufacture ID, Device ID, Sect Fact, Sect Prot
 * 555,AA 2AA,55 555,88		Enter Security region enable
 * 555,AA 2AA,55 555,90 xxx,00  Exit Security sector
 * 555,AA 2AA,55 555,A0		Program
 * 555,AA 2AA,55 555,80 555,AA 2AA,55 555,10	Chip Erase
 * 555,AA 2AA,55 555,80 555,AA 2AA,55 sct,30	Sector sct erase
 *  55,98			CFI read
 * xxx,B0 			Erase suspend
 * xxx,30			Erase resume
 *
 *
 *
 * ST commands
 * 22ED (top),22FD (bottom)
 *
 * 000,F0					Reset
 * 555,AA 2AA,55 000,F0		Reset
 * 555,AA 2AA,55 555,90		Auto Select
 * 555,98					CFI read
 * 55,AA 2AA,55 555,A0 PA,PD	Program
 * 555,50 PA0,PD0 PA1,PD1		Double Program
 * 555,56 PA0,PD0 PA1,PD1, PA2,PD2 PA3 PD3	Quadruple Program
 * 555,AA 2AA,55 555,20		Unlock ByPass
 * 000,A0 PA,PD				Unlock ByPass Program
 * 000,90 000,00			Unlock ByPass Reset
 * 555,AA 2AA,55 55,555 555,80 555,AA 2AA,55 555,10	Chip Erase
 * 555,AA 2AA,55 555,80 555,AA 2AA,55 BlkA,30	Block Erase
 * 000,B0					Program Erase Suspend
 * 000,30					Program Erase Resume
 * 555,AA 2AA,55 555,88		Enter Extended Block
 * 555,AA 2AA,55 555,90 000,00	Exit Extended Block
 */

#define DATA_READ 	0
#define DATA_WRITE  	1

#define SET_MASK(A, B, mask) ( (A & ~mask) | (B & mask) )
#define WAIT_SOME() asm volatile("nop;nop;nop;nop;nop;nop;nop;nop;nop":::"memory")

enum layout_type_t {
	REGULAR = 0,
	BOTTOM,
	TOP
};

static struct {
	uint16_t device_id;
	uint8_t flash_layout; // layout_type_t
	int8_t top_bottom;
	bool support_double;
} meta = {
	.flash_layout = REGULAR,
	.top_bottom = -1,
	.support_double = false,
};

enum sector_action_type {
	ERASE_REWRITE_FULL = 1, // do a full erase & write
	WAS_ERASED = 2,         // was already erased, only write
	SAME_CHECK_BUSY = 3,    // check if the data we're writing is already the same
};

uint16_t vkart_data_buffer[VKART_BUFFER_WORDSZ];
#define wrimage_buf vkart_data_buffer
static struct {
	uint32_t blockaddr;
	uint32_t off_in_block;
	uint16_t blocklen;
	uint8_t block;
	uint8_t act_typ; // sector_action_type
	bool new_sector;
} wrimage = {
	.block = 0xff,
	.act_typ = 0,
	.new_sector = false
};


struct len_and_block {
	uint16_t len;
	uint16_t block;
};

static struct len_and_block info_of_address(uint32_t addr) {
	uint16_t len, block;

	len = 0x8000;
	block = addr >> 15;
	if (meta.flash_layout == BOTTOM) {
		if (block == meta.top_bottom) {
			block = addr >> 12;
			len = 0x1000;
		} else {
			block += 7;
		}
	} else if (meta.flash_layout == TOP) {
		if (block == meta.top_bottom) {
			block += (addr >> 12) & 7;
			len = 0x1000;
		}
	}

	return (struct len_and_block){.len=len, .block=block};
}

bool vkart_init(void) {
	//GPIO_InitTypeDef  GPIO_InitStructure = {0};
	RCC_APB2PeriphClockCmd(
		RCC_APB2Periph_USART1 |
		RCC_APB2Periph_GPIOA |
		RCC_APB2Periph_GPIOD |
		RCC_APB2Periph_GPIOC |
		RCC_APB2Periph_GPIOB |
		RCC_APB2Periph_GPIOE, ENABLE);
	GPIOD->CFGLR = 0x44444444;
	GPIOD->CFGHR = 0x44444444;
	// Address Low
	GPIOC->CFGLR = 0x44444444;
	GPIOC->CFGHR = 0x44444444;

	// Adress Hi
	GPIOB->CFGHR = SET_MASK(GPIOB->CFGHR, 0x44444400, 0xffffff00);

	// Control
	GPIOE->CFGLR |= 0x00000333;
	GPIOE->CFGLR = 0x44444333;

	do_reset();
	Delay_Ms(10);
	uint16_t devid = get_device_id();
	iprintf("[vkart] device id %04x\r\n", devid);
	if (devid == 0x0000) return false;

	meta.device_id = devid;
	if (devid == 0x22cb || devid == 0x22e9) { // Macronix
		if (devid == 0x22cb) {
			meta.flash_layout = BOTTOM;
			meta.top_bottom = 0;
		}
		if (devid == 0x22e9) {
			meta.flash_layout = TOP;
			meta.top_bottom = 127;
		}
	}
	if (devid == 0x22ed || devid == 0x22fd) { // ST-Numonix
		if (devid == 0x22fd) {
			meta.flash_layout = BOTTOM;
			meta.top_bottom = 0;
		}
		if (devid == 0x22ed) {
			meta.flash_layout = TOP;
			meta.top_bottom = 127;
		}
		meta.support_double = 1;
	}

	iprintf("[vkart] flash layout : %d.%d\r\n", meta.flash_layout, meta.top_bottom);

	do_reset();
	Delay_Ms(10);

	return true;
}

inline static void set_ce(uint8_t state) {
	if (state) {
		GPIOE->BSHR = 0x00000002; // 00000000 00000000 00000000 00000010
	} else {
		GPIOE->BSHR = 0x00020000; // 00000000 00000010 00000000 00000000
	}
}
inline static void set_rw(uint8_t state) {
	if (state) {
		GPIOE->BSHR = 0x00000001;
	} else {
		GPIOE->BSHR = 0x00010000;
	}
}
inline static void set_data_dir(uint8_t state) {
	if (state) {
		GPIOD->CFGLR = 0x33333333; // output
		GPIOD->CFGHR = 0x33333333;
	} else {
		GPIOD->CFGLR = 0x44444444; // input
		GPIOD->CFGHR = 0x44444444;
	}
}
static void set_address(uint32_t addr) {
	GPIOC->CFGLR = 0x33333333;
	GPIOC->CFGHR = 0x33333333;
	//GPIOB->CFGLR = 0x33333333;
	//GPIOB->CFGHR ^= 0x33333300 ^ 0xffffff00;
	GPIOB->CFGHR = SET_MASK(GPIOB->CFGHR, 0x33333300, 0xffffff00);
	GPIO_Write(GPIOC, addr);
	// 1111110000000000
	int mask = 0xfc00;
	//GPIOB->OUTDR ^= ((addr >> 16) << 10) ^ 0xfc00;
	//GPIOB->OUTDR = (GPIOB->OUTDR & ~mask) | (((addr>>16)<<10) & mask);
	GPIOB->OUTDR = SET_MASK(GPIOB->OUTDR, (addr >> 16) << 10, mask);
	//out = (in & ~mask) | (val & mask);
}
inline static void set_data(uint16_t data) {
	GPIO_Write(GPIOD, data);
}
inline static uint16_t get_data(void) {
	return GPIO_ReadInputData(GPIOD);
}
static uint16_t read_word(uint32_t addr) {
	uint16_t ret;
	CRITICAL_SECTION({
		set_ce(1);
		set_rw(1);
		set_address(addr);
		WAIT_SOME();
		set_ce(0);
		WAIT_SOME();
		ret = get_data();
	});
	//iprintf("[vkart] read %04x\r\n", ret);
	return ret;
}
static void write_word(uint32_t addr, uint16_t word) {
	CRITICAL_SECTION({
		set_ce(1);
		set_rw(0);
		WAIT_SOME();
		set_data_dir(DATA_WRITE);
		set_address(addr);
		WAIT_SOME();
		set_ce(0);
		WAIT_SOME();
		set_data(word);
		WAIT_SOME();
		set_ce(1);
	});
}
/*static void write_word_mx2(uint32_t addr, uint16_t d1) {
	do_reset();
}*/

/*static ret_msg data_toggle(void) {
	uint8_t tog1, tog2;

	tog1 = read_word(0);
	tog2 = read_word(0);

	if ((tog1 & 0x40) == (tog2 & 0x40)) {
		return Success;
	}
	if ((tog2 & 0x20) != 0x20) {
		return InProgress;
	}
	tog1 = read_word(0);
	tog2 = read_word(0);
	if ((tog1 & 0x40) == (tog2 & 0x40)) {
		return Success;
	} else {
		return ToggleFailed;
	}
}*/


static void write_word_mx(uint32_t addr, uint16_t d1) {
	uint8_t t1,t2;
	//if (d1) iprintf("[vkart] writing %04x at %08x\r\n", d1, addr);
//	if (d1 != 0xffff) {
		write_word(0x0555,0xAA);
		write_word(0x02AA,0x55);
		write_word(0x0555,0xA0);
		write_word(addr, d1);
		//Delay_Us(13); // typical 11us
		while (1) {
			t1 = read_word(0);
			t2 = read_word(0);
			//iprintf("%d %d\r\n", t1, t2);
			if ((t1 & 0x40) == (t2 & 0x40)) break;
		}
//	}
	//check_status();
}
static void write_word_29w(uint32_t addr, uint16_t d1, uint16_t d2) {
	write_word(0x0555,0x0050);
	write_word(addr, d1);
	write_word(addr+1, d2);
	//while ((read_word(0)&0x40) != (read_word(0)&0x40));
	Delay_Us(20); // typical 10us
}
static void erase_block(uint32_t addr) {
	do_reset();
	write_word(0x0555, 0xaa);
	write_word(0x02aa, 0x55);
	write_word(0x0555, 0x80);
	write_word(0x0555, 0xaa);
	write_word(0x02aa, 0x55);
	write_word(addr, 0x30);

	uint8_t q = 0;
	/*
	set_data_dir(DATA_READ);
	while (1) {
		q = read_word(addr);
		if ((q & 0x08) != 0) break;
	}
	while (1) {
		q = read_word(addr);

		if ((q & 0x80) != 0) {
			// Finished
			break;
		}
	} */
	Delay_Ms(900);
	//iprintf("[vkart] End erase block %08lx st=%02x\r\n", addr, q);
}

static uint16_t get_device_id(void) {
	//iprintf("[vkart] -- get_device_id --\r\n");
	write_word(0x555, 0xAA);
	write_word(0x2AA, 0x55);
	write_word(0x555, 0x90);
	set_data_dir(DATA_READ);
	return read_word(0x1);
}
static void do_reset(void) {
	write_word(0x0, 0x00f0);
}

void vkart_read_data(uint32_t addr, uint16_t* buff, uint32_t len) {
	set_data_dir(DATA_READ);
	for (uint32_t i = 0; i < len; ++i) {
		buff[i] = read_word(addr + i);
	}
}
void vkart_erase_sector(uint32_t addr, uint8_t block) {
	iprintf("[vkart] erase sector addr %08lx for %d\r\n", addr, block);
	erase_block(addr);
	do_reset();
}
void vkart_write_data(const uint16_t *pbuf, uint32_t addr, uint32_t len) {
	if (len < 2) return;

	if (meta.support_double) {
		//iprintf("[vkart] double write at %08lx for len %08lx\r\n", addr, len);
		for (uint32_t i = 0; i < len; i += 2) {
			//if ((pbuf[i] & pbuf[i+1]) == 0xffff) continue;
			write_word_29w(addr + i, pbuf[i], pbuf[i+1]);
			WAIT_SOME();
			WAIT_SOME();
			WAIT_SOME();
		}
	} else {
		//iprintf("[vkart] single write at %08lx for len %08lx\r\n", addr, len);
		for (uint32_t i = 0; i < len; ++i) {
			//if (pbuf[i] == 0xffff) continue;
			write_word_mx(addr + i, pbuf[i]);
			WAIT_SOME();
			WAIT_SOME();
			WAIT_SOME();
		}
	}
	//iprintf("[vkart] prog %ld words done at %08lx\r\n", len, addr);
}

static void check_new_sector(void) {
	if (!wrimage.new_sector) return;
	wrimage.new_sector = false;

	bool blank = true;
	set_data_dir(DATA_READ);
	wrimage.act_typ = WAS_ERASED;
	for (uint32_t i = 0; i < wrimage.blocklen; ++i) {
		if (read_word(wrimage.blockaddr + i) != 0xffff) {
			blank = false;
			break;
		}
	}

	if (blank) {
		wrimage.act_typ = WAS_ERASED;
		iprintf("[vkart] wrimage: clean, only write\r\n");
	} else if (wrimage.blocklen > VKART_BUFFER_WORDSZ) {
		// can't buffer, so can't do a wear-levelling check -> no other choice
		// but to erase the entire sector.
		wrimage.act_typ = ERASE_REWRITE_FULL;
		iprintf("[vkart] wrimage: full erase & rewrite\r\n");
	} else {
		wrimage.act_typ = SAME_CHECK_BUSY;
		iprintf("[vkart] wrimage: same-data-check\r\n");
	}

	if (wrimage.act_typ == ERASE_REWRITE_FULL) {
		vkart_erase_sector(wrimage.blockaddr, wrimage.block);
	}
}
static void start_new_sector(void) {
	wrimage.blockaddr += wrimage.blocklen;
	struct len_and_block lab = info_of_address(wrimage.blockaddr);
	wrimage.block = lab.block;
	wrimage.blocklen = lab.len;
	wrimage.off_in_block = 0;

	//iprintf("[vkart] wrimage: new sector %08lx %d len %06x\r\n", wrimage.blockaddr, wrimage.block, wrimage.blocklen);

	wrimage.new_sector = true;
}

bool vkart_wrimage_start(void) {
	if (wrimage.block != 0xff) return false;

	iprintf("[vkart] wrimage: start\r\n");

	wrimage.new_sector = false;
	wrimage.blockaddr = 0;
	wrimage.blocklen = 0;
	start_new_sector();

	return true;
}
bool vkart_wrimage_next(const uint16_t* pbuf, uint32_t len) {
	uint32_t todo = len;
	bool end = false;
	if (wrimage.off_in_block + todo > wrimage.blocklen) {
		todo = wrimage.blocklen - wrimage.off_in_block;
	}

	if (wrimage.blockaddr + wrimage.off_in_block + todo >= VKART_MEMORY_WORDSZ) {
		todo = VKART_MEMORY_WORDSZ - (wrimage.blockaddr + wrimage.off_in_block);
		end = true; // don't tailcall!
	}

	//iprintf("[vkart] wrimage: next %06lx, will do %06lx\r\n", len, todo);
	if (end) {
		iprintf("[vkart] wrimage: REACHES END\r\n");
	}

	check_new_sector();

	if (wrimage.act_typ == ERASE_REWRITE_FULL || wrimage.act_typ == WAS_ERASED) {
		vkart_write_data(pbuf, wrimage.blockaddr + wrimage.off_in_block, todo);
	} else if (wrimage.act_typ == SAME_CHECK_BUSY) {
		bool failed = false;

		set_data_dir(DATA_READ);

		memcpy(&wrimage_buf[wrimage.off_in_block], pbuf, todo * sizeof(uint16_t));

		iprintf("[vkart] wrimage: same check from %08lx len %06lx...\r\n",
				wrimage.blockaddr + wrimage.off_in_block, todo);
		for (uint32_t i = 0; i < todo; ++i) {
			uint16_t memv = read_word(wrimage.blockaddr + wrimage.off_in_block + i);
			if (memv != pbuf[i]) {
				failed = true; // oh no!
				iprintf("[vkart] wrimage: same check failed! at %08lx: have %04x, write %04x\r\n",
						wrimage.blockaddr + wrimage.off_in_block + i, memv, pbuf[i]);
				break;
			}
		}

		if (failed) {
			iprintf("[vkart] wrimage: selfcheck recover: erasing & writing buffer, len %08lx\r\n",
					wrimage.off_in_block + todo);
			wrimage.act_typ = ERASE_REWRITE_FULL;
			vkart_erase_sector(wrimage.blockaddr, wrimage.block);

			vkart_write_data(wrimage_buf, wrimage.blockaddr, wrimage.off_in_block + todo);
		} else {
			iprintf("[vkart] wrimage: same check passed, continuing...\r\n");
		}
	}

	wrimage.off_in_block += todo;
	if (wrimage.off_in_block == wrimage.blocklen) {
		if (end) { // we've reached the end of our flash memory, need to stop
			return end;
		}

		start_new_sector();
	}

	if (!end && len > todo) {
		// there's more left -> do that as well
		iprintf("[vkart] wrimage: tailcall!\r\n");

		return vkart_wrimage_next(pbuf + todo, len - todo); // tailcall
	} else return end;
}
void vkart_wrimage_finish(void) {
	if (wrimage.block == 0xff) return;

	wrimage.block = 0xff;
	wrimage.new_sector = false;

	iprintf("[vkart] wrimage: done\r\n");
}


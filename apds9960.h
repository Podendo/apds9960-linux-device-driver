#ifndef APDS9960_H_
#define APDS9960_H_

			/* APDS9960 Register Set */
#define APDS9960_RAM_START	(0x00)
#define APDS9960_RAM_END	(0x7F)
#define APDS9960_REG_ENABLE	(0x80) /* enable states and interrupts */
#define APDS9960_REG_ATIME	(0x81) /* adc integration time */
#define APDS9960_REG_WTIME	(0x83) /* wait time (non-gesture) */

#define APDS9960_REG_AILTL	(0x84) /* ALS int low threshold low byte*/
#define APDS9960_REG_AILTH	(0x85) /* ALS int low threshold high byte*/
#define APDS9960_REG_AIHTL	(0x86) /* ALS int hight threshold low byte */
#define APDS9960_REG_AIHTH	(0x87) /* ALS int hight threshold hight byte*/

#define APDS9960_REG_PILT	(0x89) /* proximity int low threshold */
#define APDS9960_REG_PIHT	(0x8B) /* proximity int hight threshold */
#define APDS9960_REG_PERS	(0x8C) /* int persistence filters non-gesture*/

#define APDS9960_REG_CONFIG1	(0x8D) /* configuration register one */
#define APDS9960_REG_CONFIG2	(0x90) /* configuration register two */
#define APDS9960_REG_CONFIG3	(0x9F) /* configuration register three */

#define APDS9960_REG_PPULSE	(0x8E) /* proximity pulse count and lenght */
#define APDS9960_REG_CONTROL	(0x8F) /* gain control */

#define APDS9960_REG_ID		(0x92) /* Device ID */
#define APDS9960_REG_STATUS	(0x93) /* Device status */
#define APDS9960_REG_CDATAL	(0x94) /* lowbyte of clear channel data */
#define APDS9960_REG_CDATAH	(0x95) /* highbyte of clear channel data */
#define APDS9960_REG_RDATAL	(0x96) /* lowbyte of red channel data */
#define APDS9960_REG_RDATAH	(0x97) /* highbyte of red channel data */
#define APDS9960_REG_GDATAL	(0x98) /* lowbyte of green channel data */
#define APDS9960_REG_GDATAH	(0x99) /* highbyte of green channel data */
#define APDS9960_REG_BDATAL	(0x9A) /* lowbyte of blue channel data */
#define APDS9960_REG_BDATAH	(0x9B) /* highbyte of blue channel data */
#define APDS9960_REG_PDATA	(0x9C) /* proximity data */

#define APDS9960_REG_POFFSET_UR	(0x9D) /* prox offset for UP and RIGHT VD */
#define APDS9960_REG_POFFSET_DL (0x9E) /* prox offset for DOWN and LEFT VD */
#define APDS9960_REG_GPENTH	(0xA0) /* gesture proximity enter threshold */
#define APDS9960_REG_GEXTH	(0xA1) /* gesture exit threshold */

#define APDS9960_REG_GCONF1	(0xA2) /* gesture configuration one */
#define APDS9960_REG_GCONF2	(0xA3) /* gesture configuration two */
#define APDS9960_REG_GCONF3	(0xAA) /* gesture configuration three */
#define APDS9960_REG_GCONF4	(0xAB) /* gestre configuration four */

#define APDS9960_REG_GOFFSET_U	(0xA4) /* gesture UP offset register */
#define APDS9960_REG_GOFFSET_D	(0xA5) /* gesture DOWN offset register */
#define APDS9960_REG_GOFFSET_L	(0xA7) /* gesture LEFT offset register */
#define APDS9960_REG_GOFFSET_R	(0xA9) /* gesture RIGHT offset register */
#define APDS9960_REG_GPULSE	(0xA6) /* gesture pulse count and lenght */
#define APDS9960_REG_GFLVL	(0xAE) /* gesture FIFO level */
#define APDS9960_REG_GSTATUS	(0xAF) /* gesture status */

		/* only write registers, special i2c address accessing */
#define APDS9960_REG_IFORCE	(0xE4) /* force interrupt */
#define APDS9960_REG_PICLEAR	(0xE5) /* proximity interrupt clear */
#define APDS9960_REG_CICLEAR	(0xE6) /* ALS clear channel interrupts clear */
#define APDS9960_REG_AICLEAR	(0xE7) /* all non-gesture interrupts clear */

#define APDS9960_REG_GFIFO_U	(0xFC)	/* gesture FIFO UP value */
#define APDS9960_REG_GFIFO_D	(0xFD)	/* gesture FIFO DOWN value */
#define APDS9960_REG_GFIFO_L	(0xFE)	/* gesture FIFO LEFT value */
#define APDS9960_REG_GFIFO_R	(0xFF)	/* gesture FIFO RIGHT value */

#endif /* APDS9960_H_ */

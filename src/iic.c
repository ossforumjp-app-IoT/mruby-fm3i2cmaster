/*------------------------------------------------------------------------/
/  MB9BF616/617/618 I2C master control module
/-------------------------------------------------------------------------/
/
/  Copyright (C) 2012, ChaN, all right reserved.
/
/ * This software is a free software and there is NO WARRANTY.
/ * No restriction on use. You can use, modify and redistribute it for
/   personal, non-profit or commercial products UNDER YOUR RESPONSIBILITY.
/ * Redistributions of source code must retain the above copyright notice.
/
/-------------------------------------------------------------------------*/

#include "iic.h"

#define F_PCLK		72000000	/* Bus clock for the MFS module */

#define NUM_MFS 8
#define T_SMR 0
#define T_IBCR 1
#define T_IBSR 2
#define T_SSR 3
#define T_TDR 4
#define T_RDR 5
#define T_BGR 6
#define T_ISBA 7
#define T_ISMK 8

#define ISER ((volatile uint32_t*)0xE000E100)
#define ICER ((volatile uint32_t*)0xE000E180)
#define __enable_irqn(n) ISER[(n) / 32] = 1 << ((n) % 32)
#define __disable_irqn(n) ICER[(n) / 32] = 1 << ((n) % 32)

/* Register address table */
static const uint32_t registerTable_mfs_set[NUM_MFS][9] = {
	/*	MFS_SMR		MFS_IBCR	MFS_IBSR	MFS_SSR		MFS_TDR		MFS_RDR		MFS_BGR		MFS_ISBA	MFS_ISMK	*/
	{	0x40038000,	0x40038001, 0x40038004,	0x40038005,	0x40038008, 0x40038008,	0x4003800C,	0x40038010,	0x40038011	},	/* MFS0 */
	{	0x40038100,	0x40038101,	0x40038104,	0x40038105,	0x40038108,	0x40038108,	0x4003810C,	0x40038110,	0x40038111	},	/* MFS1 */
	{	0x40038200,	0x40038201,	0x40038204,	0x40038205,	0x40038208,	0x40038208,	0x4003820C,	0x40038210,	0x40038211	},	/* MFS2 */
	{	0x40038300,	0x40038301,	0x40038304,	0x40038305,	0x40038308,	0x40038308,	0x4003830C,	0x40038310,	0x40038311	},	/* MFS3 */
	{	0x40038400,	0x40038401,	0x40038404,	0x40038405,	0x40038408,	0x40038408,	0x4003840C,	0x40038410,	0x40038411	},	/* MFS4 */
	{	0x40038500,	0x40038501,	0x40038504,	0x40038505,	0x40038508,	0x40038508,	0x4003850C,	0x40038510,	0x40038511	},	/* MFS5 */
	{	0x40038600,	0x40038601,	0x40038604,	0x40038605,	0x40038608,	0x40038608,	0x4003860C,	0x40038610,	0x40038611	},	/* MFS6 */
	{	0x40038700,	0x40038701,	0x40038704,	0x40038705,	0x40038708,	0x40038708,	0x4003870C,	0x40038710,	0x40038711	}	/* MFS7 */
};

static const uint32_t registerTable_epfr[NUM_MFS] = {
	0x4003361C,		/* MFS0 */
	0x4003361C,		/* MFS1 */
	0x4003361C,		/* MFS2 */
	0x4003361C,		/* MFS3 */
	0x40033620,		/* MFS4 */
	0x40033620,		/* MFS5 */
	0x40033620,		/* MFS6 */
	0x40033620		/* MFS7 */
};

static const uint32_t registerTable_pfr[NUM_MFS][3] = {
	/*	PIN_LOC0		PIN_LOC1		PIN_LOC2	*/
	{	0x40033008,		0x40033004,		0x4003302C	},	/* MFS0(PFR2, PFR1, PFRB) */
	{	0x40033014,		0x40033004,		0x4003303C	},	/* MFS1(PFR5, PFR1, PFRF) */
	{	0x4003301C,		0x40033008,		0x40033004	},	/* MFS2(PFR7, PFR2, PFR1) */
	{	0x4003301C,		0x40033014,		0x40033010	},	/* MFS3(PFR7, PFR5, PFR4) */
	{	0x40033034,		0x40033004,		0x40033000	},	/* MFS4(PFRD, PFR1, PFR0) */
	{	0x40033018,		0x40033024,		0x4003300C	},	/* MFS5(PFR6, PFR9, PFR3) */
	{	0x40033014,		0x4003300C,		0x4003303C	},	/* MFS6(PFR5, PFR3, PFRF) */
	{	0x40033014,		0x40033010,		0x4003302C	},	/* MFS7(PFR5, PFR4, PFRB) */
};

/* EPFR setting table */
static const uint8_t settingTable_epfr[] = {
	6,		/* MFS0, MFS4 */
	12,		/* MFS1, MFS5 */
	18,		/* MFS2, MFS6 */
	24		/* MFS3, MFS7 */
};

/* Pin relocation table */
static const uint8_t settingTable_pinloc[NUM_MFS][3] = {
	/*	PIN_LOC0	PIN_LOC1	PIN_LOC2	*/
	{	2,			5,			5			},	/* MFS0 */
	{	7,			21,			1			},	/* MFS1 */
	{	3,			5,			8			},	/* MFS2 */
	{	6,			1,			9			},	/* MFS3 */
	{	0,			11,			6			},	/* MFS4 */
	{	1,			3,			7			},	/* MFS5 */
	{	4,			1,			4			},	/* MFS6 */
	{	10,			12,			1			}	/* MFS7 */
};

/* MFS_IRQn table (MFS_TX_IRQn) */
static const uint8_t settingTable_irqn[NUM_MFS] = {
	8,		/* MFS0 */
	10,		/* MFS1 */
	12,		/* MFS2 */
	14,		/* MFS3 */
	16,		/* MFS4 */
	18,		/* MFS5 */
	20,		/* MFS6 */
	22		/* MFS7 */
};

enum { S_SLA_W, S_CMD_W, S_DAT_W, S_SLA_R, S_DAT_R };	/* State of transaction Ctrl0->phase */

static volatile I2CCTRL		*Ctrl0_0, *Ctrl0_1, *Ctrl0_2, *Ctrl0_3,
							*Ctrl0_4, *Ctrl0_5, *Ctrl0_6, *Ctrl0_7;	/* Current I2C control structure */

/*------------------------*/
/* Initialize I2C Module  */
/*------------------------*/

void i2c_attach_mfs(uint8_t mfs, uint8_t loc)
{
	volatile uint32_t *add_epfr, *add_pfr;
	uint8_t epfr_no, pinloc_no;
	
	add_epfr = (uint32_t *)(registerTable_epfr[mfs]);
	add_pfr = (uint32_t *)(registerTable_pfr[mfs][loc]);
	epfr_no = settingTable_epfr[mfs%4];
	pinloc_no = settingTable_pinloc[mfs][loc];
	
	switch(loc)
	{
		case 0:		/* PIN_LOC0 */
			*add_epfr = (*add_epfr & ~(15 << epfr_no)) | (5 << epfr_no);
			break;
		case 1:		/* PIN_LOC1 */
			*add_epfr = (*add_epfr & ~(15 << epfr_no)) | (10 << epfr_no);
			break;
		case 2: 	/* PIN_LOC2 */
			*add_epfr = (*add_epfr & ~(15 << epfr_no)) | (15 << epfr_no);
			break;
		default:
			break;
	}
	
	*add_pfr |= 3 << pinloc_no;
}

void i2c_init (volatile I2CCTRL *ctrl, uint32_t bps)
{
	volatile uint8_t *add_smr, *add_ssr, *add_isba, *add_ismk;
	volatile uint16_t *add_bgr; 
	uint8_t mfs, loc;
	uint8_t tx_irqn;

	mfs = ctrl->mfsch;
	loc = ctrl->locno;
	
	add_smr = (uint8_t *)(registerTable_mfs_set[mfs][T_SMR]);
	add_ssr = (uint8_t *)(registerTable_mfs_set[mfs][T_SSR]);
	add_bgr = (uint16_t *)(registerTable_mfs_set[mfs][T_BGR]);
	add_isba = (uint8_t *)(registerTable_mfs_set[mfs][T_ISBA]);
	add_ismk = (uint8_t *)(registerTable_mfs_set[mfs][T_ISMK]);
	tx_irqn = settingTable_irqn[mfs];
	
	__disable_irqn(tx_irqn);

	*add_ismk = 0;		/* Disable I2C function */

	/* Bus reset sequence may be required here to make all slaves inactive */

	/* Initialize MFS in I2C mode */
	*add_smr = 0x80;		/* Set I2C mode */
	*add_ssr = 0;
	*add_bgr = F_PCLK / bps - 1;
	*add_isba = 0;		/* No address detection */
	*add_ismk = 0xFF;	/* Enable I2C function */
	
	/* Attach MFS module to I/O pads */
	i2c_attach_mfs(mfs, loc);
	
	/* Enable MFS status interrupt */
	__enable_irqn(tx_irqn);
}

/*--------------------------*/
/* Start an I2C Transaction */
/*--------------------------*/

int i2c_start (
	volatile I2CCTRL *ctrl	/* Pointer to the initialized I2C control structure */
)
{
	volatile uint8_t *add_ibcr;
	volatile uint16_t *add_tdr;
	uint8_t mfs;
	
	mfs = ctrl->mfsch;
	add_ibcr = (uint8_t *)(registerTable_mfs_set[mfs][T_IBCR]);
	add_tdr = (uint16_t *)(registerTable_mfs_set[mfs][T_TDR]);

	switch(mfs)
	{
		case 0:		/* MFS0 */
			if(Ctrl0_0)		return 0;	/* Reject if an I2C transaction is in progress */
			Ctrl0_0 = ctrl;		break;	/* Register the I2C control strucrure as current transaction */
		case 1:		/* MFS1 */
			if(Ctrl0_1)		return 0;
			Ctrl0_1 = ctrl;		break;
		case 2:		/* MFS2 */
			if(Ctrl0_2)		return 0;
			Ctrl0_2 = ctrl;		break;
		case 3:		/* MFS3 */
			if(Ctrl0_3)		return 0;
			Ctrl0_3 = ctrl;		break;
		case 4:		/* MFS4 */
			if(Ctrl0_4)		return 0;
			Ctrl0_4 = ctrl;		break;
		case 5:		/* MFS5 */
			if(Ctrl0_5)		return 0;
			Ctrl0_5 = ctrl;		break;
		case 6:		/* MFS6 */
			if(Ctrl0_6)		return 0;
			Ctrl0_6 = ctrl;		break;
		case 7:		/* MFS7 */
			if(Ctrl0_7)		return 0;
			Ctrl0_7 = ctrl;		break;
		default:
			break;
	}
	
	ctrl->status = I2C_BUSY;	/* An I2C transaction is in progress */
	*add_tdr = ctrl->sla << 1;	/* Data to be sent as 1st byte = SLA+W */
	ctrl->phase = S_SLA_W;
	*add_ibcr = 0x85;			/* Wait for bus-free and then generate start condition */

	return 1;
}

/*--------------------------*/
/* Abort I2C Transaction    */
/*--------------------------*/

void i2c_abort (volatile I2CCTRL *ctrl)
{
	volatile uint8_t *add_ibcr, *add_ismk;
	uint8_t mfs, tx_irqn;
	
	mfs = ctrl->mfsch;
	add_ibcr = (uint8_t *)(registerTable_mfs_set[mfs][T_IBCR]);
	add_ismk = (uint8_t *)(registerTable_mfs_set[mfs][T_ISMK]);
	tx_irqn = settingTable_irqn[mfs];

	*add_ibcr = 0;	/* Disable I2C function */
	*add_ismk = 0;
	
	switch(mfs)		/* Discard I2C control structure */
	{
		case 0:
			Ctrl0_0 = 0;		break;
		case 1:
			Ctrl0_1 = 0;		break;
		case 2:
			Ctrl0_2 = 0;		break;
		case 3:
			Ctrl0_3 = 0;		break;
		case 4:
			Ctrl0_4 = 0;		break;
		case 5:
			Ctrl0_5 = 0;		break;
		case 6:
			Ctrl0_6 = 0;		break;
		case 7:
			Ctrl0_7 = 0;		break;
		default:
			break;
	}
	
	__disable_irqn(tx_irqn);
}

/*-------------------------*/
/* I2C Background Process  */
/*-------------------------*/

void MFS_TX_IRQHandler_i2cmaster (uint8_t mfs)
{
	volatile I2CCTRL *ctrl;
	uint8_t ibcr, ibsr, eot = 0;
	uint16_t n;
	uint16_t ismk;
	volatile uint8_t *add_ibcr, *add_ibsr, *add_ismk;
	volatile uint16_t *add_tdr, *add_rdr;
	uint8_t tx_irqn;
	
	switch(mfs)
	{
		case 0:
			ctrl = Ctrl0_0;		break;
		case 1:
			ctrl = Ctrl0_1;		break;
		case 2:
			ctrl = Ctrl0_2;		break;
		case 3:
			ctrl = Ctrl0_3;		break;
		case 4:
			ctrl = Ctrl0_4;		break;
		case 5:
			ctrl = Ctrl0_5;		break;
		case 6:
			ctrl = Ctrl0_6;		break;
		case 7:
			ctrl = Ctrl0_7;		break;
		default:
			return;
	}
	
	add_ibcr = (uint8_t *)(registerTable_mfs_set[mfs][T_IBCR]);
	add_ibsr = (uint8_t *)(registerTable_mfs_set[mfs][T_IBSR]);
	add_tdr = (uint16_t *)(registerTable_mfs_set[mfs][T_TDR]);
	add_rdr = (uint16_t *)(registerTable_mfs_set[mfs][T_RDR]);
	add_ismk = (uint8_t *)(registerTable_mfs_set[mfs][T_ISMK]);
	tx_irqn = settingTable_irqn[mfs];

	if (!ctrl) {		/* Spurious Interrupt */
		*add_ibcr = 0;	/* Disable I2C function */
		*add_ismk = 0;
		__disable_irqn(tx_irqn);
		return;
	}
	
	ibcr = *add_ibcr;
	ibsr = *add_ibsr;
	ismk = *add_ismk;
	if (!(ibcr & 0x80) || (ibsr & 0x20)) {	/* Bus error, Arbitration lost or Reserved address */
		eot = I2C_ERROR;

	} else {
		switch (ctrl->phase) {
		case S_SLA_W:	/* S+SLA+W sent */
			if (ibsr & 0x40) {	/* ACK not received (slave not responded) */
				n = ctrl->retry;
				if (n) {
					ctrl->retry = n - 1;
					*add_tdr = ctrl->sla << 1;	/* Send Sr+SLA+W */
					*add_ibcr = 0xC5;
				} else {
					eot = I2C_TIMEOUT;
				}
			} else {			/* ACK received (slave responded) */
				ctrl->ncmd--;
				*add_tdr = ctrl->cmd[0];	/* Sent command byte */
				ctrl->icmd = 1;
				*add_ibcr = 0x84;
				ctrl->phase = S_CMD_W;
			}
			break;

		case S_CMD_W:	/* Command byte sent */
			if (ibsr & 0x40) {	/* ACK not received */
				eot = I2C_ABORTED;
			} else {			/* ACK received */
				n = ctrl->ncmd;
				if (n) {	/* There is command byte to be sent */
					ctrl->ncmd = n - 1;
					*add_tdr = ctrl->cmd[ctrl->icmd++];	/* Send next command byte */
					*add_ibcr = 0x84;
				} else {	/* All command byte has been sent */
					n = ctrl->ndata;
					if (n) {
						if (ctrl->dir == I2C_WRITE) {	/* There is data to be written */
							ctrl->ndata = n - 1;
							ctrl->phase = S_DAT_W;
							*add_tdr = *ctrl->data++;	/* Send data */
							*add_ibcr = 0x84;
							ctrl->phase = S_DAT_W;
						} else {						/* There is data to be read */
							*add_tdr = (ctrl->sla << 1) + 1;	/* Send Sr+SLA+R */
							*add_ibcr = 0xC5;
							ctrl->phase = S_SLA_R;
						}
					} else {	/* No data to be written/read */
						eot = I2C_SUCCEEDED;
					}
				}
			}
			break;

		case S_DAT_W:	/* Data sent */
			if (ibsr & 0x40) {	/* ACK not received */
				eot = I2C_ABORTED;
			} else {			/* ACK received */
				n = ctrl->ndata;
				if (n) {		/* There is any data to be written */
					ctrl->ndata = n - 1;
					*add_tdr = *ctrl->data++;	/* Send data */
					*add_ibcr = 0x84;
				} else {		/* All data has been sent */
					eot = I2C_SUCCEEDED;
				}
			}
			break;

		case S_SLA_R:	/* Sr+SLA+R sent */
			if (ibsr & 0x40) {	/* ACK not received (slave not responded) */
				eot = I2C_ABORTED;
			} else {			/* ACK received (slave responded) */
				*add_ibcr = (ctrl->ndata > 1) ? 0xA4 : 0x84;
				ctrl->phase = S_DAT_R;
			}
			break;

		case S_DAT_R:	/* Data received */
			*ctrl->data++ = *add_rdr;	/* Store read data */
			n = ctrl->ndata - 1;
			if (n) {	/* There is any data to be read */
				ctrl->ndata = n;
				*add_ibcr = (n > 1) ? 0xA4 : 0x84;
			} else {	/* All data has been read */
				eot = I2C_SUCCEEDED;
			}
			break;

		default:
			eot = I2C_ERROR;
		}
	}

	if (eot) {	/* End of I2C transaction? */
		*add_ibcr = 0x00;
		switch(mfs)		/* Release I2C control structure */
		{
			case 0:
				Ctrl0_0 = 0;		break;
			case 1:
				Ctrl0_1 = 0;		break;
			case 2:
				Ctrl0_2 = 0;		break;
			case 3:
				Ctrl0_3 = 0;		break;
			case 4:
				Ctrl0_4 = 0;		break;
			case 5:
				Ctrl0_5 = 0;		break;
			case 6:
				Ctrl0_6 = 0;		break;
			case 7:
				Ctrl0_7 = 0;		break;
			default:
				break;
	}
		ctrl->status = eot;	/* Set result code */
		if (ctrl->eotfunc) ctrl->eotfunc(eot);	/* Notify EOT if call-back function is specified */
	}
	
}

/*
** mrb_fm3i2cmaster.c - FM3 I2C Master
**
**
*/

#include "iic.h"

#include "mruby.h"
#include "mruby/data.h"
#include "mruby/class.h"

#define MFS0 0
#define MFS1 1
#define MFS2 2
#define MFS3 3
#define MFS4 4
#define MFS5 5
#define MFS6 6
#define MFS7 7
#define PIN_LOC0 0
#define PIN_LOC1 1
#define PIN_LOC2 2

void mrb_fm3i2cmaster_free(mrb_state *mrb, void *ptr);

mrb_value mrb_FM3_i2cmasterInitialize(mrb_state *mrb, mrb_value self);
mrb_value mrb_FM3_i2cmasterWrite(mrb_state *mrb, mrb_value self);
mrb_value mrb_FM3_i2cmasterRead(mrb_state *mrb, mrb_value self);
void mrb_mruby_fm3i2cmaster_gem_init(mrb_state *mrb);
void mrb_mruby_fm3i2cmaster_gem_final(mrb_state *mrb);

static const struct mrb_data_type mrb_fm3i2cmaster_type = {"I2CMaster", mrb_fm3i2cmaster_free};

/* instance free */
void mrb_fm3i2cmaster_free(mrb_state *mrb, void *ptr)
{
	volatile I2CCTRL *i2cm = ptr;
	
	if(i2cm->data != NULL)
		i2cm->data = NULL;
	
	if(i2cm->eotfunc != 0)
		i2cm->eotfunc = 0;
	
	mrb_free(mrb, i2cm);
}

/* I2CHost.initialize(mfs, loc, bps) */
mrb_value mrb_FM3_i2cmasterInitialize(mrb_state *mrb, mrb_value self)
{
	volatile I2CCTRL *i2cm;
	mrb_int mfs;
	mrb_int loc;
	mrb_int bps;
	
	i2cm = (I2CCTRL *)mrb_malloc(mrb, sizeof(I2CCTRL));
	i2cm->data = NULL;
	i2cm->eotfunc = 0;
	DATA_TYPE(self) = &mrb_fm3i2cmaster_type;
	DATA_PTR(self) = i2cm;
	
	mrb_get_args(mrb, "iii", &mfs, &loc, &bps);
	i2cm->mfsch = (uint8_t)mfs;
	i2cm->locno = (uint8_t)loc;
	
	i2c_init(i2cm, (uint32_t)bps);
	
	return self;
}

/* I2CHost.write(addr, command, data) */
mrb_value mrb_FM3_i2cmasterWrite(mrb_state *mrb, mrb_value self)
{
	volatile I2CCTRL *i2cm;
	mrb_int addr, command, data;
	uint8_t buff[1];
	
	mrb_get_args(mrb, "iii", &addr, &command, &data);
	i2cm = DATA_PTR(self);
	
	i2cm->sla = (uint8_t)addr;				/* 7-bit slave address */
	i2cm->cmd[0] = (uint8_t)command;		/* Command bytes follows SLA+W */
	i2cm->ncmd = 1;
	buff[0] = (uint8_t)data;
	i2cm->data = buff;	i2cm->ndata = 1;	/* Data to write and count */
	i2cm->dir = I2C_WRITE;					/* Data direction */
	i2cm->eotfunc = 0;						/* Call-back function */
	
	i2c_start(i2cm);						/* Start the I2C transaction */
	while(i2cm->status == I2C_BUSY);		/* Wait for end of I2C transaction */
	
	return mrb_nil_value();
}

/* I2CHost.read(addr, comand) */
mrb_value mrb_FM3_i2cmasterRead(mrb_state *mrb, mrb_value self)
{
	volatile I2CCTRL *i2cm;
	mrb_int addr, command;
	uint8_t buff[1];
	
	mrb_get_args(mrb, "ii", &addr, &command);
	i2cm = DATA_PTR(self);
	
	i2cm->sla = (uint8_t)addr;				/* 7-bit slave address */
	i2cm->cmd[0] = (uint8_t)command;		/* Command bytes following SLA+W */
	i2cm->ncmd = 1; 
	buff[0] = (uint8_t)command;
	i2cm->data = buff;	i2cm->ndata = 1;	/* Data to write and count */
	i2cm->dir = I2C_READ;					/* Data direction */
	i2cm->eotfunc = 0;						/* Call-back function */
	
	i2c_start(i2cm);						/* Start the I2C transaction */
	while (i2cm->status == I2C_BUSY) ;		/* Wait for end of I2C transaction */
	
	if(i2cm->status != I2C_SUCCEEDED)
		return mrb_fixnum_value(-1);
	else
		return mrb_fixnum_value(buff[0]);
}

void mrb_mruby_fm3i2cmaster_gem_init(mrb_state *mrb)
{
	struct RClass *i2cmaster;
	
	/* define class */
	i2cmaster = mrb_define_class(mrb, "I2CMaster", mrb->object_class);
	MRB_SET_INSTANCE_TT(i2cmaster, MRB_TT_DATA);
	
	/* define method */
	mrb_define_method(mrb, i2cmaster, "initialize", mrb_FM3_i2cmasterInitialize, MRB_ARGS_REQ(3));
	mrb_define_method(mrb, i2cmaster, "write", mrb_FM3_i2cmasterWrite, MRB_ARGS_REQ(3));
	mrb_define_method(mrb, i2cmaster, "read", mrb_FM3_i2cmasterRead, MRB_ARGS_REQ(2));
	
	/* define constracts */
	mrb_define_const(mrb, i2cmaster, "MFS0", mrb_fixnum_value(MFS0));
	mrb_define_const(mrb, i2cmaster, "MFS1", mrb_fixnum_value(MFS1));
	mrb_define_const(mrb, i2cmaster, "MFS2", mrb_fixnum_value(MFS2));
	mrb_define_const(mrb, i2cmaster, "MFS3", mrb_fixnum_value(MFS3));
	mrb_define_const(mrb, i2cmaster, "MFS4", mrb_fixnum_value(MFS4));
	mrb_define_const(mrb, i2cmaster, "MFS5", mrb_fixnum_value(MFS5));
	mrb_define_const(mrb, i2cmaster, "MFS6", mrb_fixnum_value(MFS6));
	mrb_define_const(mrb, i2cmaster, "MFS7", mrb_fixnum_value(MFS7));
	mrb_define_const(mrb, i2cmaster, "PIN_LOC0", mrb_fixnum_value(PIN_LOC0));
	mrb_define_const(mrb, i2cmaster, "PIN_LOC1", mrb_fixnum_value(PIN_LOC1));
	mrb_define_const(mrb, i2cmaster, "PIN_LOC2", mrb_fixnum_value(PIN_LOC2));
}
 
void mrb_mruby_fm3i2cmaster_gem_final(mrb_state *mrb)
{
	
}

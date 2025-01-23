#include "bflb_i2c.h"
#include "es9038q2m.h"

#define es9038q2m_I2C_SLAVE_ADDR (0x90 >> 1)

static struct bflb_device_s *i2c0 = NULL;
static struct bflb_i2c_msg_s msgs[2];
static union All_Registers_bits Regp;
/****************************************************************************/ /**
 * @brief  es9038q2m_I2C_Init
 *
 * @param  None
 *
 * @return None
 *
*******************************************************************************/
void es9038q2m_I2C_Init(void)
{
  i2c0 = bflb_device_get_by_name("i2c0");
  if (i2c0 == NULL) {
    printf("I2C_Init_fail\r\n");
    return -1;
  }
  bflb_i2c_init(i2c0, 400000);
  return 0;
}
void print_regsiter(uint8_t data)
{
  printf("%d%d%d%d%d%d%d%d",
         (data >> 7) & 1,
         (data >> 6) & 1,
         (data >> 5) & 1,
         (data >> 4) & 1,
         (data >> 3) & 1,
         (data >> 2) & 1,
         (data >> 1) & 1,
         data & 1);
}
/****************************************************************************/ /**
 * @brief  es9038q2m write register
 *
 * @param  addr: Register address
 * @param  data: data
 *
 * @return None
 *
*******************************************************************************/
int es9038q2m_Write_Reg(uint8_t Reg, uint8_t data)
{
  msgs[0].addr = es9038q2m_I2C_SLAVE_ADDR;
  msgs[0].flags = I2C_M_NOSTOP;
  msgs[0].buffer = &Reg;
  msgs[0].length = 1;
  msgs[1].addr = es9038q2m_I2C_SLAVE_ADDR;
  msgs[1].flags = 0;
  msgs[1].buffer = &data;
  msgs[1].length = 1;
  if (!i2c0) {
    return 0;
  }
  return bflb_i2c_transfer(i2c0, msgs, 2);
}

/****************************************************************************/ /**
 * @brief  es9038q2m_Read_Reg
 *
 * @param  addr: Register address
 * @param  rdata: data
 *
 * @return None
 *
*******************************************************************************/
int es9038q2m_Read_Reg(uint8_t Reg, uint8_t *rdata)
{
  msgs[0].addr = es9038q2m_I2C_SLAVE_ADDR;
  msgs[0].flags = I2C_M_NOSTOP;
  msgs[0].buffer = &Reg;
  msgs[0].length = 1;

  msgs[1].flags = I2C_M_READ;
  msgs[1].buffer = rdata;
  msgs[1].length = 1;
  if (!i2c0) {
    return 0;
  }
  return bflb_i2c_transfer(i2c0, msgs, 2);
}
union All_Registers_bits *get_reg(void)
{
  return &Regp;
}
void es9038q2m_Reg_Dump(void)
{
  int i;
  uint8_t tmp;
  for (i = 0; i <= 102; i++) {
    if (i > 54 && i < 64) {
      continue;
    }
    int i2state = es9038q2m_Read_Reg(i, &tmp);
    if (i2state == -ETIMEDOUT) {
      printf("iic read err,reg:%d\r\n", i);
    } else {
      Regp.datas[i] = tmp;
    }
    /*
    printf("Register %03d state:", i);
    print_regsiter(tmp);
    printf("\r\n");
    */
  }
  printf("ES9038Q2M Reg Dump complete\r\n");
  //Regp.canwrite.Input_selection;
  //print_regsiter(Regp.canwrite.Input_selection);
  //printf("\r\n");
  //printf("%d\r\n",sizeof(Regp));
}

void es9038q2m_Reg_over_write(void)
{
  int i;
  for (i = 0; i <= 102; i++) {
    if (i > 54 && i < 64) {
      continue;
    }
    int i2state = es9038q2m_Write_Reg(i, Regp.datas[i]);
    if ((i2state == -ETIMEDOUT) && (i != 0)) {
      printf("iic over write err,reg:%d\r\n", i);
    }
  }
}
#define Volume_limit 255

static union All_Registers_bits *cfg;
void es9038q2m_init(void)
{
  es9038q2m_I2C_Init();
  es9038q2m_Write_Reg(0, 0b00000001); //reset
  es9038q2m_Reg_Dump();
  cfg = get_reg();
  //cfg->canwrite.System_Registers |= 0b00000001;
  cfg->canwrite.Input_selection = 0b00010000;
  cfg->canwrite.Soft_Start_Configuration = 0b10000010;
  // 踩过的坑,es9038q2m要用这个寄存器来启动模拟输出
  // i2c_cmd write 001 01010000
  // i2c_cmd dump
  cfg->canwrite.Filter_Bandwidth &= 0b00011111;
  cfg->canwrite.Filter_Bandwidth |= 0b01100000;
  //cfg->canwrite.General_Configuration = 0b11010111;//18db gain
  //cfg->canwrite.Volume_Control[0] = 80;
  //cfg->canwrite.Volume_Control[1] = 80;
  cfg->canwrite.Volume_Control[0] = 255 - Volume_limit;
  cfg->canwrite.Volume_Control[1] = 255 - Volume_limit;
  es9038q2m_Reg_over_write();
}
//16 24 32
void es9038q2m_set_data_width(uint8_t bit_width)
{
  Regp.datas[1] &= 0x3f;
  switch (bit_width) {
    case 16:
      Regp.datas[1] |= (0 << 6);
      break;
    case 24:
      Regp.datas[1] |= (1 << 6);
      break;
    case 32:
      Regp.datas[1] |= (2 << 6);
      break;
    default:
      Regp.datas[1] |= (3 << 6);
      printf("Unsupport bit width\r\n");
      break;
  }
  es9038q2m_Write_Reg(1, Regp.datas[1]);
}
uint8_t volume_table[] = {
0 ,35 ,56 ,72 ,83 ,93 ,101 ,108 ,114 ,120 ,
125 ,130 ,134 ,138 ,141 ,145 ,148 ,151 ,154 ,157 ,
159 ,162 ,164 ,166 ,168 ,170 ,172 ,174 ,176 ,178 ,
180 ,181 ,183 ,185 ,186 ,188 ,189 ,190 ,192 ,193 ,
194 ,196 ,197 ,198 ,199 ,201 ,202 ,203 ,204 ,205 ,
206 ,207 ,208 ,209 ,210 ,211 ,212 ,213 ,214 ,215 ,
215 ,216 ,217 ,218 ,219 ,220 ,220 ,221 ,222 ,223 ,
223 ,224 ,225 ,226 ,226 ,227 ,228 ,228 ,229 ,230 ,
230 ,231 ,232 ,232 ,233 ,234 ,234 ,235 ,235 ,236 ,
236 ,237 ,238 ,238 ,239 ,239 ,240 ,240 ,241 ,241 ,
242 ,243 ,243 ,244 ,244 ,245 ,245 ,246 ,246 ,247 ,
247 ,247 ,248 ,248 ,249 ,249 ,250 ,250 ,251 ,251 ,
252 ,252 ,252 ,253 ,253 ,254 ,254 ,255 };
void es9038q2m_set_volume(int volume)
{  
  if (volume < 0) {
    volume = 0;
  }
  volume = volume_table[volume];
  if (volume > Volume_limit) {
    volume = Volume_limit;
  }

  Regp.datas[15] = Volume_limit - volume;
  Regp.datas[16] = Volume_limit - volume;
  es9038q2m_Write_Reg(15, Regp.datas[15]);
  es9038q2m_Write_Reg(16, Regp.datas[16]);
}
void set18dbgain(uint8_t status){
  Regp.datas[27] = (Regp.datas[27]&0b11111100)|status;
  es9038q2m_Write_Reg(27, Regp.datas[27]);

}
int get_volume(void)
{
  uint8_t volume;
  es9038q2m_Read_Reg(15, &volume);
  return volume;
}
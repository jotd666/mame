// license:BSD-3-Clause
// copyright-holders:Angelo Salese
/***************************************************************************

    Seibu Cup Soccer bootlegs

    Splitted since it definitely doesn't use neither real COP nor CRTC


    TODO:
    - tilemap chip drawings might be merged between this and other
      Seibu implementations.

OSC: 40MHz, 15MHz & 8MHz

Measured clocks:
 MC68000P12F - 14.99MHz
 Z0840006PSC -  3.996MHz
   OKI M6295 -  0.999MHz

***************************************************************************/


#include "emu.h"
#include "cpu/m68000/m68000.h"
#include "cpu/z80/z80.h"
#include "machine/gen_latch.h"
#include "machine/seicopbl.h"
#include "sound/okim6295.h"
#include "emupal.h"
#include "screen.h"
#include "speaker.h"
#include "tilemap.h"


class seicupbl_state : public driver_device
{
public:
	seicupbl_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag)
		, m_maincpu(*this, "maincpu")
		, m_audiocpu(*this, "audiocpu")
		, m_back_data(*this, "back_data")
		, m_fore_data(*this, "fore_data")
		, m_mid_data(*this, "mid_data")
		, m_textram(*this, "textram")
		, m_spriteram(*this, "spriteram")
		, m_vregs(*this, "vregs")
		, m_oki(*this, "oki")
		, m_soundlatch(*this, "soundlatch")
		, m_gfxdecode(*this, "gfxdecode")
		, m_palette(*this, "palette")
	{ }

	void cupsocbl(machine_config &config);

private:
	// devices
	required_device<cpu_device> m_maincpu;
	required_device<cpu_device> m_audiocpu;
	required_shared_ptr<uint16_t> m_back_data;
	required_shared_ptr<uint16_t> m_fore_data;
	required_shared_ptr<uint16_t> m_mid_data;
	required_shared_ptr<uint16_t> m_textram;
	required_shared_ptr<uint16_t> m_spriteram;
	required_shared_ptr<uint16_t> m_vregs;
	required_device<okim6295_device> m_oki;
	required_device<generic_latch_8_device> m_soundlatch;
	required_device<gfxdecode_device> m_gfxdecode;
	required_device<palette_device> m_palette;

	tilemap_t *m_sc_layer[4]{};

	// screen updates
	uint32_t screen_update(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect);
	void okim_rombank_w(uint8_t data);
	void vram_sc0_w(offs_t offset, uint16_t data, uint16_t mem_mask = ~0);
	void vram_sc1_w(offs_t offset, uint16_t data, uint16_t mem_mask = ~0);
	void vram_sc2_w(offs_t offset, uint16_t data, uint16_t mem_mask = ~0);
	void vram_sc3_w(offs_t offset, uint16_t data, uint16_t mem_mask = ~0);
	void layer_disable_w(offs_t offset, uint16_t data, uint16_t mem_mask = ~0);
	TILE_GET_INFO_MEMBER(get_sc0_tileinfo);
	TILE_GET_INFO_MEMBER(get_sc1_tileinfo);
	TILE_GET_INFO_MEMBER(get_sc2_tileinfo);
	TILE_GET_INFO_MEMBER(get_sc3_tileinfo);

	void cupsocbl_mem(address_map &map);
	void cupsocbl_sound_mem(address_map &map);

	// driver_device overrides
	virtual void machine_start() override;
	virtual void machine_reset() override;

	virtual void video_start() override;

	void draw_sprites(screen_device &screen, bitmap_ind16 &bitmap,const rectangle &cliprect);
	uint16_t m_layer_disable = 0;
};

TILE_GET_INFO_MEMBER(seicupbl_state::get_sc0_tileinfo)
{
	int tile=m_back_data[tile_index];
	int color=(tile>>12)&0xf;

	tile &= 0xfff;
	//tile |= m_back_gfx_bank;        /* Heatbrl uses banking */

	tileinfo.set(1,tile,color,0);
}

TILE_GET_INFO_MEMBER(seicupbl_state::get_sc1_tileinfo)
{
	int tile=m_mid_data[tile_index];
	int color=(tile>>12)&0xf;

	tile &= 0xfff;

	tile |= 0x1000;
	color += 0x10;

	tileinfo.set(1,tile,color,0);
}

TILE_GET_INFO_MEMBER(seicupbl_state::get_sc2_tileinfo)
{
	int tile=m_fore_data[tile_index];
	int color=(tile>>12)&0xf;

	tile &= 0xfff;

	tileinfo.set(4,tile,color,0);
}

TILE_GET_INFO_MEMBER(seicupbl_state::get_sc3_tileinfo)
{
	int tile = m_textram[tile_index];
	int color=(tile>>12)&0xf;

	tile &= 0xfff;

	tileinfo.set(0,tile,color,0);
}

void seicupbl_state::draw_sprites(screen_device &screen, bitmap_ind16 &bitmap,const rectangle &cliprect)
{
	uint16_t *spriteram16 = m_spriteram;
	int offs,fx,fy,x,y,color,sprite,cur_pri;
	int dx,dy,ax,ay;
	int pri_mask;

	for (offs = 0;offs < 0x400;offs += 4)
	{
		uint16_t data = spriteram16[offs];
		if (!(data &0x8000)) continue;

		pri_mask = 0;

		cur_pri = (spriteram16[offs+1] & 0xc000) >> 14;

		if(data & 0x0040)
			cur_pri |= 0x4; // definitely seems to be needed by grainbow

		//
		// -4 behind bg? (mask sprites)
		// -32 behind mid
		// -256 behind tx
		// 0    above all

		// is the low bit REALLY priority?

		switch (cur_pri)
		{
			// gumdam swamp monster l2
			case 0: pri_mask = -256; break;
			// (players and football goal, should go above sidelines but behind portraits when there's a goal)
			case 1: pri_mask = 0xfff0; break;
			// masking effect for gundam l2 monster
			case 2: pri_mask = -4; break;
			// cupsoc (radar dots)
			case 3: pri_mask = 0x0000; break;
			// gundam level 2/3 player
			case 4: pri_mask = -32; break;
			//case 5: pri_mask = 0; break;
			// insert coin in gundam
			case 6: pri_mask = 0; break;
			//case 7: pri_mask = 0; break;

			default: printf("unhandled pri %d\n",cur_pri); pri_mask=0;
		}

		sprite = spriteram16[offs+1];

		sprite &= 0x3fff;

		y = spriteram16[offs+3];
		x = spriteram16[offs+2];

		/* heated barrel hardware seems to need 0x1ff with 0x100 sign bit for sprite warp,
		   this doesn't work on denjin makai as the visible area is larger */
		if (cliprect.max_x<(320-1))
		{
			x&=0x1ff;
			y&=0x1ff;

			if (x&0x100) x-=0x200;
			if (y&0x100) y-=0x200;
		}
		else
		{
			x&=0xfff;
			y&=0xfff;

			if (x&0x800) x-=0x1000;
			if (y&0x800) y-=0x1000;

		}


		color = (data &0x3f) + 0x40;
		fx =  (data &0x4000) >> 14;
		fy =  (data &0x2000) >> 13;
		dy = ((data &0x0380) >> 7)  + 1;
		dx = ((data &0x1c00) >> 10) + 1;

		if (!fx)
		{
			if(!fy)
			{
				for (ax=0; ax<dx; ax++)
					for (ay=0; ay<dy; ay++)
					{
						m_gfxdecode->gfx(3)->prio_transpen(bitmap,cliprect,
						sprite++,
						color,fx,fy,(x+ax*16),y+ay*16,
						screen.priority(),pri_mask, 15);
					}
			}
			else
			{
				for (ax=0; ax<dx; ax++)
					for (ay=0; ay<dy; ay++)
					{
						m_gfxdecode->gfx(3)->prio_transpen(bitmap,cliprect,
						sprite++,
						color,fx,fy,(x+ax*16),y+(dy-ay-1)*16,
						screen.priority(),pri_mask,15);
					}
			}
		}
		else
		{
			if(!fy)
			{
				for (ax=0; ax<dx; ax++)
					for (ay=0; ay<dy; ay++)
					{
						m_gfxdecode->gfx(3)->prio_transpen(bitmap,cliprect,
						sprite++,
						color,fx,fy,(x+(dx-ax-1)*16)+0,y+ay*16,
						screen.priority(),pri_mask,15);
					}
			}
			else
			{
				for (ax=0; ax<dx; ax++)
					for (ay=0; ay<dy; ay++)
					{
						m_gfxdecode->gfx(3)->prio_transpen(bitmap,cliprect,
						sprite++,
						color,fx,fy,(x+(dx-ax-1)*16)+0,y+(dy-ay-1)*16,
						screen.priority(),pri_mask, 15);
					}
			}
		}
	}
}


void seicupbl_state::video_start()
{
	m_sc_layer[0] = &machine().tilemap().create(*m_gfxdecode, tilemap_get_info_delegate(*this, FUNC(seicupbl_state::get_sc0_tileinfo)), TILEMAP_SCAN_ROWS, 16,16, 32,32);
	m_sc_layer[1] = &machine().tilemap().create(*m_gfxdecode, tilemap_get_info_delegate(*this, FUNC(seicupbl_state::get_sc1_tileinfo)), TILEMAP_SCAN_ROWS, 16,16, 32,32);
	m_sc_layer[2] = &machine().tilemap().create(*m_gfxdecode, tilemap_get_info_delegate(*this, FUNC(seicupbl_state::get_sc2_tileinfo)), TILEMAP_SCAN_ROWS, 16,16, 32,32);
	m_sc_layer[3] = &machine().tilemap().create(*m_gfxdecode, tilemap_get_info_delegate(*this, FUNC(seicupbl_state::get_sc3_tileinfo)), TILEMAP_SCAN_ROWS,   8,8, 64,32);

	for(int i=0;i<4;i++)
		m_sc_layer[i]->set_transparent_pen(15);
}

uint32_t seicupbl_state::screen_update( screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect )
{
	/* Setup the tilemaps */
	screen.priority().fill(0, cliprect);
	bitmap.fill(m_palette->black_pen(), cliprect);    /* wrong color? */

	// compared to regular CRTC, it looks like it mixes the two values instead (hence the -0x1f0)
	for(int i=0;i<4;i++)
	{
		m_sc_layer[i]->set_scrollx(0, m_vregs[i*2+0] - 0x1f0);
		m_sc_layer[i]->set_scrolly(0, m_vregs[i*2+1]);
	}

	if (!(m_layer_disable&0x0001))
		m_sc_layer[0]->draw(screen, bitmap, cliprect, 0, 1);

	if (!(m_layer_disable&0x0002))
		m_sc_layer[1]->draw(screen, bitmap, cliprect, 0, 2);

	if (!(m_layer_disable&0x0004))
		m_sc_layer[2]->draw(screen, bitmap, cliprect, 0, 4);

	if (!(m_layer_disable&0x0008))
		m_sc_layer[3]->draw(screen, bitmap, cliprect, 0, 8);

	if (!(m_layer_disable&0x0010))
		draw_sprites(screen,bitmap,cliprect);
	return 0;
}

void seicupbl_state::vram_sc0_w(offs_t offset, uint16_t data, uint16_t mem_mask)
{
	COMBINE_DATA(&m_back_data[offset]);
	m_sc_layer[0]->mark_tile_dirty(offset);
}

void seicupbl_state::vram_sc1_w(offs_t offset, uint16_t data, uint16_t mem_mask)
{
	COMBINE_DATA(&m_mid_data[offset]);
	m_sc_layer[1]->mark_tile_dirty(offset);
}

void seicupbl_state::vram_sc2_w(offs_t offset, uint16_t data, uint16_t mem_mask)
{
	COMBINE_DATA(&m_fore_data[offset]);
	m_sc_layer[2]->mark_tile_dirty(offset);
}

void seicupbl_state::vram_sc3_w(offs_t offset, uint16_t data, uint16_t mem_mask)
{
	COMBINE_DATA(&m_textram[offset]);
	m_sc_layer[3]->mark_tile_dirty(offset);
}

void seicupbl_state::layer_disable_w(offs_t offset, uint16_t data, uint16_t mem_mask)
{
	COMBINE_DATA(&m_layer_disable);
}

void seicupbl_state::cupsocbl_mem(address_map &map)
{
//  legionna_cop_mem(map);
	map(0x000000, 0x0fffff).rom();
	map(0x100400, 0x1005ff).rw("seibucop_boot", FUNC(seibu_cop_bootleg_device::read), FUNC(seibu_cop_bootleg_device::write));
	map(0x10065c, 0x10065d).w(FUNC(seicupbl_state::layer_disable_w));
	map(0x100660, 0x10066f).ram().share("vregs");
	map(0x100700, 0x100701).portr("DSW1");
	map(0x100704, 0x100705).portr("PLAYERS12");
	map(0x100708, 0x100709).portr("PLAYERS34");
	map(0x10070c, 0x10070d).portr("SYSTEM");
	map(0x10071c, 0x10071d).portr("DSW2");
	map(0x100741, 0x100741).w(m_soundlatch, FUNC(generic_latch_8_device::write));
	map(0x100800, 0x100fff).ram().w(FUNC(seicupbl_state::vram_sc0_w)).share("back_data");
	map(0x101000, 0x1017ff).ram().w(FUNC(seicupbl_state::vram_sc2_w)).share("fore_data");
	map(0x101800, 0x101fff).ram().w(FUNC(seicupbl_state::vram_sc1_w)).share("mid_data");
	map(0x102000, 0x102fff).ram().w(FUNC(seicupbl_state::vram_sc3_w)).share("textram");
	map(0x103000, 0x103fff).ram().w(m_palette, FUNC(palette_device::write16)).share("palette");
	map(0x104000, 0x106fff).ram();
	map(0x107000, 0x1077ff).ram().share("spriteram");
	map(0x107800, 0x107fff).ram();
	map(0x108000, 0x11ffff).ram();
}

void seicupbl_state::okim_rombank_w(uint8_t data)
{
//  popmessage("%08x",0x40000 * (data & 0x07));
	m_oki->set_rom_bank(data & 0x7);
}

void seicupbl_state::cupsocbl_sound_mem(address_map &map)
{
	map(0x0000, 0x7fff).rom();
	map(0x8000, 0x87ff).ram();
	map(0x9000, 0x9000).w(FUNC(seicupbl_state::okim_rombank_w));
	map(0x9800, 0x9800).rw(m_oki, FUNC(okim6295_device::read), FUNC(okim6295_device::write));
	map(0xa000, 0xa000).r(m_soundlatch, FUNC(generic_latch_8_device::read));
}


static INPUT_PORTS_START( cupsoc )
	//SEIBU_COIN_INPUTS   /* coin inputs read through sound cpu */

	PORT_START("SYSTEM")
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_UNKNOWN ) //TEST
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_SERVICE2 )
	PORT_BIT( 0x00f0, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_START3 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_START4 )
	PORT_BIT( 0xfc00, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START("PLAYERS12")
	PORT_BIT( 0x0001, IP_ACTIVE_LOW,  IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW,  IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW,  IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW,  IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW,  IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW,  IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW,  IPT_BUTTON3 ) PORT_PLAYER(1)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW,  IPT_COIN1 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW,  IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW,  IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW,  IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW,  IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW,  IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW,  IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW,  IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT( 0x8000, IP_ACTIVE_LOW,  IPT_COIN2 )

	PORT_START("PLAYERS34")
	PORT_BIT( 0x0001, IP_ACTIVE_LOW,  IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(3)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW,  IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(3)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW,  IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(3)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW,  IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(3)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW,  IPT_BUTTON1  ) PORT_PLAYER(3)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW,  IPT_BUTTON2  ) PORT_PLAYER(3)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW,  IPT_BUTTON3 ) PORT_PLAYER(3)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW,  IPT_UNKNOWN ) //TEST
	PORT_BIT( 0x0100, IP_ACTIVE_LOW,  IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(4)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW,  IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(4)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW,  IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(4)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW,  IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(4)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW,  IPT_BUTTON1 ) PORT_PLAYER(4)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW,  IPT_BUTTON2 ) PORT_PLAYER(4)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW,  IPT_BUTTON3 ) PORT_PLAYER(4)
	PORT_BIT( 0x8000, IP_ACTIVE_LOW,  IPT_SERVICE1 )

	PORT_START("DSW1")
	PORT_DIPNAME( 0x0007, 0x0007, "Coin 1 (3)" )
	PORT_DIPSETTING(      0x0000, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0007, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0006, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0005, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0003, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x0038, 0x0038, "Coin 2 (4)" )
	PORT_DIPSETTING(      0x0000, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0038, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0030, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0028, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0018, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x0040, 0x0040, "Staring Coin" )
	PORT_DIPSETTING(      0x0040, DEF_STR( Normal ) )
	PORT_DIPSETTING(      0x0000, "x2" )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0300, 0x0300, "Time vs Computer" )
	PORT_DIPSETTING(      0x0200, "1:30" )
	PORT_DIPSETTING(      0x0300, "2:00" )
	PORT_DIPSETTING(      0x0100, "2:30" )
	PORT_DIPSETTING(      0x0000, "3:00" )
	PORT_DIPNAME( 0x0c00, 0x0c00, "Time vs Player, 2 Players" )
	PORT_DIPSETTING(      0x0800, "2:00" )
	PORT_DIPSETTING(      0x0c00, "2:30" )
	PORT_DIPSETTING(      0x0400, "3:00" )
	PORT_DIPSETTING(      0x0000, "x:xx" )
	PORT_DIPNAME( 0x3000, 0x3000, "Time vs Player, 3 Players" )
	PORT_DIPSETTING(      0x2000, "2:30" )
	PORT_DIPSETTING(      0x3000, "3:00" )
	PORT_DIPSETTING(      0x1000, "3:30" )
	PORT_DIPSETTING(      0x0000, "x:xx" )
	PORT_DIPNAME( 0xc000, 0xc000, "Time vs Player, 4 Players" )
	PORT_DIPSETTING(      0x8000, "3:00" )
	PORT_DIPSETTING(      0xc000, "3:30" )
	PORT_DIPSETTING(      0x4000, "4:00" )
	PORT_DIPSETTING(      0x0000, "x:xx" )

	PORT_START("DSW2")
	PORT_DIPNAME( 0x0003, 0x0003, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0003, DEF_STR( Normal ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( Easy ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( Hard ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Hardest ) )
	PORT_DIPNAME( 0x000c, 0x0000, "Players / Coin Mode" )
	PORT_DIPSETTING(      0x0000, "4 Players / 1 Coin Slot" )
	PORT_DIPSETTING(      0x0004, "4 Players / 4 Coin Slots" )
	PORT_DIPSETTING(      0x0008, "4 Players / 2 Coin Slots" )
	PORT_DIPSETTING(      0x000c, "2 Players" )
	PORT_DIPNAME( 0x0010, 0x0010, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Service_Mode ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	// probably not mapped, service lists 3*8 dips
	PORT_DIPNAME( 0xff00, 0xff00, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0xff00, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
INPUT_PORTS_END

void seicupbl_state::machine_start()
{
}

void seicupbl_state::machine_reset()
{
}

static const gfx_layout cupsocsb_spritelayout =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{ 0,1,2,3 },
	{ 4,0,12,8,20,16,28,24, 512+4, 512+0, 512+12, 512+8, 512+20, 512+16, 512+28, 512+24 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32, 8*32, 9*32, 10*32, 11*32, 12*32, 13*32, 14*32, 15*32 },
	32*32
};

static const gfx_layout cupsocsb_8x8_tilelayout =
{
	8,8,
	RGN_FRAC(1,1),
	4,
	{ 8,12,0,4 },
	{ 0,3,2,1,16,19,18,17 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	8*32
};


static const gfx_layout cupsocsb_tilelayout =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{ 8,12,0,4 },
	{ 0,3,2,1,16,19,18,17,  512+0,512+3,512+2,512+1,512+16,512+19,512+18,512+17 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32, 8*32, 9*32, 10*32, 11*32, 12*32, 13*32, 14*32, 15*32 },
	32*32
};


static GFXDECODE_START( gfx_seicupbl_csb )
	GFXDECODE_ENTRY( "char", 0, cupsocsb_8x8_tilelayout,    48*16, 16 )
	GFXDECODE_ENTRY( "gfx3", 0, cupsocsb_tilelayout,        0*16, 32 )
	GFXDECODE_ENTRY( "gfx4", 0, cupsocsb_tilelayout,        32*16, 16 ) /* unused */
	GFXDECODE_ENTRY( "sprite", 0, cupsocsb_spritelayout,    0*16, 8*16 )
	GFXDECODE_ENTRY( "gfx5", 0, cupsocsb_tilelayout,        32*16, 16 )
	GFXDECODE_ENTRY( "gfx6", 0, cupsocsb_tilelayout,        16*16, 16 )
GFXDECODE_END


void seicupbl_state::cupsocbl(machine_config &config)
{
	/* basic machine hardware */
	M68000(config, m_maincpu, 15_MHz_XTAL); // 15MHz verified
	m_maincpu->set_addrmap(AS_PROGRAM, &seicupbl_state::cupsocbl_mem);
	m_maincpu->set_vblank_int("screen", FUNC(seicupbl_state::irq4_line_hold)); /* VBL */

	SEIBU_COP_BOOTLEG(config, "seibucop_boot", m_maincpu);

	/*Different Sound hardware*/
	Z80(config, m_audiocpu, 8_MHz_XTAL/2); // 4MHz verified
	m_audiocpu->set_addrmap(AS_PROGRAM, &seicupbl_state::cupsocbl_sound_mem);
	//m_audiocpu->set_periodic_int("screen", FUNC(seicupbl_state::nmi_line_pulse));

	/* video hardware */
	screen_device &screen(SCREEN(config, "screen", SCREEN_TYPE_RASTER));
	screen.set_refresh_hz(60);
	screen.set_vblank_time(ATTOSECONDS_IN_USEC(0));
	screen.set_size(42*8, 36*8);
	screen.set_visarea(0*8, 40*8-1, 0*8, 30*8-1);
	screen.set_screen_update(FUNC(seicupbl_state::screen_update));
	screen.set_palette(m_palette);

	//seibu_crtc_device &crtc(SEIBU_CRTC(config, "crtc", 0));
	//crtc.layer_en_callback().set(FUNC(seicupbl_state::tilemap_enable_w));
	//crtc.layer_scroll_callback().set(FUNC(seicupbl_state::tile_scroll_w));
	//crtc.reg_1a_callback().set(FUNC(seicupbl_state::tile_vreg_1a_w));

	GFXDECODE(config, m_gfxdecode, m_palette, gfx_seicupbl_csb);

	PALETTE(config, m_palette, palette_device::BLACK).set_format(palette_device::xBGR_555, 128*16);

//  MCFG_VIDEO_START_OVERRIDE(seicupbl_state,cupsoc)

	/* sound hardware */
	SPEAKER(config, "mono").front_center();

	GENERIC_LATCH_8(config, m_soundlatch);
	m_soundlatch->data_pending_callback().set_inputline(m_audiocpu, INPUT_LINE_NMI);

	OKIM6295(config, m_oki, 8_MHz_XTAL/8, okim6295_device::PIN7_HIGH).add_route(ALL_OUTPUTS, "mono", 1.0); // 1MHz verified
}



/***************************************************************************

  Machine driver(s)

***************************************************************************/

/*

Seibu Cup Soccer - Seibu - Bootleg

2 boards

1st board

(snd)
1 x z80
1 x oki 6295
sc_01 (prg)
sc_02 and sc_03 (data)
8MHz & 40MHz OSC

(prg)
1 x 68000P12
sc_04 and sc_05
15MHz OSC

(gfx)
2 x ti tpc1020
from sc_06 to sc_11

2nd board

(gfx)
1 x actel pl84c
from sc_12 to sc_15

*/

ROM_START( cupsocsb )
	ROM_REGION( 0x100000, "maincpu", 0 ) /* 68000 Code */
	ROM_LOAD16_BYTE( "sc_04.bin", 0x00001, 0x80000, CRC(22566087) SHA1(4392f46ca50cc9947823a5190aa25f5e9654aa0d) )
	ROM_LOAD16_BYTE( "sc_05.bin", 0x00000, 0x80000, CRC(2f977dff) SHA1(4d8d6e7d06ce17bb7292072965911f8b1f1067e2) )

	ROM_REGION( 0x10000, "audiocpu", 0 )    /* Z80 code */
	ROM_LOAD( "sc_01.bin",    0x000000, 0x08000, CRC(cea39d6d) SHA1(f0b79c03ffafdd1e57673d6d4836becbe415110b) )
	ROM_CONTINUE(             0x000000, 0x08000 )

	ROM_REGION( 0x200000, "sprite", ROMREGION_INVERT ) /* bootleg sprite gfx */
	ROM_LOAD( "sc_07.bin", 0x000000, 0x080000, CRC(dcb29d01) SHA1(72b4234622605f0ab03f21fdb6a61c6dac36000d) )
	ROM_LOAD( "sc_06.bin", 0x080000, 0x080000, CRC(2dc70e05) SHA1(f1d0beb8428a7e1d7c7818e6719abdc543b2fa80) )
	ROM_COPY( "sprite", 0x000000, 0x100000, 0x100000 )

	ROM_REGION( 0x200000, "test1", 0 ) /* bootleg tile gfx */
	ROM_LOAD16_BYTE( "sc_09.bin", 0x000000, 0x080000, CRC(695b6342) SHA1(dfccb43789021ba2568b9284ae61e64f7f89b152) )
	ROM_LOAD16_BYTE( "sc_10.bin", 0x000001, 0x080000, CRC(27e172b8) SHA1(ed86db2f42c8061607d46f2407b0130aaf692a02) )
	ROM_LOAD16_BYTE( "sc_08.bin", 0x100000, 0x080000, CRC(637120f3) SHA1(b4b2ad192e46ff80d4cb440d7fb6dac215a353ed) )
	ROM_LOAD16_BYTE( "sc_11.bin", 0x100001, 0x080000, CRC(0cd5ca5e) SHA1(a59665e543e9383355de2576e6693348ec356591) )

	ROM_REGION( 0x020000, "char", ROMREGION_INVERT )
	ROM_COPY( "test1", 0x080000, 0x00000, 0x020000 )

	ROM_REGION( 0x100000, "gfx3", ROMREGION_INVERT )    /* MBK tiles */
	ROM_COPY( "test1", 0x000000, 0x00000, 0x080000 )
	ROM_COPY( "test1", 0x100000, 0x80000, 0x080000 )

	ROM_REGION( 0x100000, "gfx4", ROMREGION_INVERT )    /* not used */
	ROM_COPY("gfx3",0x000000,0x00000,0x100000)

	ROM_REGION( 0x080000, "gfx5", ROMREGION_INVERT )    /* BK3 tiles */
	ROM_COPY( "test1", 0x180000, 0x00000, 0x080000 )

	ROM_REGION( 0x080000, "gfx6", ROMREGION_INVERT )    /* LBK tiles */
	ROM_COPY( "gfx5", 0x000000, 0x00000, 0x080000 )

	ROM_REGION( 0x100000, "adpcm", ROMREGION_ERASEFF )  /* ADPCM samples */
	ROM_LOAD( "sc_02.bin",    0x000000, 0x020000, CRC(a70d4f03) SHA1(c2482e624c8a828a94206a36d10c1021ad8ca1d0) )
	ROM_LOAD( "sc_03.bin",    0x080000, 0x080000, CRC(6e254d12) SHA1(857779dbd276b688201a8ea3afd5817e38acad2e) )

	ROM_REGION( 0x200000, "oki", ROMREGION_ERASEFF )
	ROM_COPY( "adpcm", 0x00000, 0x000000, 0x20000 ) //bank 0
	ROM_COPY( "adpcm", 0x00000, 0x020000, 0x20000 )
	ROM_COPY( "adpcm", 0x00000, 0x100000, 0x20000 ) //bank 4
	ROM_COPY( "adpcm", 0x80000, 0x120000, 0x20000 )
	ROM_COPY( "adpcm", 0x00000, 0x140000, 0x20000 ) //bank 5
	ROM_COPY( "adpcm", 0xa0000, 0x160000, 0x20000 )
	ROM_COPY( "adpcm", 0x00000, 0x180000, 0x20000 ) //bank 6
	ROM_COPY( "adpcm", 0xc0000, 0x1a0000, 0x20000 )
	ROM_COPY( "adpcm", 0x00000, 0x1c0000, 0x20000 ) //bank 7
	ROM_COPY( "adpcm", 0xe0000, 0x1e0000, 0x20000 )


	/* these are maths tables, for whatever COP replacement the bootlegs use */
	ROM_REGION( 0x500000, "unknown0", 0 )
	ROM_LOAD16_BYTE( "sc_13.bin", 0x00000, 0x010000, CRC(229bddd8) SHA1(0924bf29db9c5a970546f154e7752697fdce6a58) )
	ROM_LOAD16_BYTE( "sc_12.bin", 0x00001, 0x010000, CRC(dabfa826) SHA1(0db587c846755491b169ef7751ba8e7cdc2607e6) )
	ROM_REGION( 0x500000, "unknown1", 0 )
	ROM_LOAD16_BYTE( "sc_15.bin", 0x00000, 0x080000, CRC(8fd87e65) SHA1(acc9fd0289fa9ab60bec16d3e642039380e5180a) )
	ROM_LOAD16_BYTE( "sc_14.bin", 0x00001, 0x080000, CRC(566086c2) SHA1(b7d09ce978f99ecc0d1975b31330ed49317701d5) )
ROM_END

/* slight changes in the program roms compared to above set, all remaining roms were the same
   main PCB is marked: "SOCCER 1", "rev.5" and "lc" on component side ("lc" is the Italian for "Lato Componenti" which translates to "Components Side")
   main PCB is marked: "SOCCER 1", "rev.5" and "ls" on component side ("ls" is the Italian for "Lato Saldature" which translates to "Solders Side")
   ROMs PCB is marked: "CALCIO 2", "rev.3" and "lc" on component side ("CALCIO is the Italian for "SOCCER"; "lc" is the Italian for "Lato Componenti" which translates to "Components Side")
   ROMs PCB is marked: "CALCIO 2", "rev.3" and "ls" on component side ("CALCIO is the Italian for "SOCCER"; "ls" is the Italian for "Lato Saldature" which translates to "Solders Side")
   main PCB is labelled: "SOCCER" on solder side */
ROM_START( cupsocsb2 )
	ROM_REGION( 0x100000, "maincpu", 0 ) /* 68000 Code */
	ROM_LOAD16_BYTE( "4.27040.u11", 0x00001, 0x80000, CRC(83db76f8) SHA1(ffcd0a728de58871b945c15cc27da374b587e170) ) // actually was a 27c4001
	ROM_LOAD16_BYTE( "5.27040.u17", 0x00000, 0x80000, CRC(c01e88c6) SHA1(8f90261792343c92ddd877ab8a2480b5aac82961) ) // actually was a 27c4001

	ROM_REGION( 0x10000, "audiocpu", 0 )    /* Z80 code */
	ROM_LOAD( "1.27512.u20",    0x000000, 0x08000, CRC(cea39d6d) SHA1(f0b79c03ffafdd1e57673d6d4836becbe415110b) ) // 27c512
	ROM_CONTINUE(             0x000000, 0x08000 )

	ROM_REGION( 0x200000, "sprite", ROMREGION_INVERT ) /* bootleg sprite gfx */
	ROM_LOAD( "7.27040.u86", 0x000000, 0x080000, CRC(dcb29d01) SHA1(72b4234622605f0ab03f21fdb6a61c6dac36000d) ) // actually was a 27c4001
	ROM_LOAD( "6.27040.u87", 0x080000, 0x080000, CRC(2dc70e05) SHA1(f1d0beb8428a7e1d7c7818e6719abdc543b2fa80) ) // actually was a 27c4001
	ROM_COPY( "sprite", 0x00000, 0x100000, 0x100000 )

	ROM_REGION( 0x200000, "test1", 0 ) /* bootleg tile gfx */
	ROM_LOAD16_BYTE( "9.27040.u36", 0x000000, 0x080000, CRC(695b6342) SHA1(dfccb43789021ba2568b9284ae61e64f7f89b152) ) // actually was a 27c4001
	ROM_LOAD16_BYTE( "10.27040.u37", 0x000001, 0x080000, CRC(27e172b8) SHA1(ed86db2f42c8061607d46f2407b0130aaf692a02) ) // actually was a 27c4001
	ROM_LOAD16_BYTE( "8.27040.u38", 0x100000, 0x080000, CRC(637120f3) SHA1(b4b2ad192e46ff80d4cb440d7fb6dac215a353ed) ) // actually was a 27c4001
	ROM_LOAD16_BYTE( "11.27040.u39", 0x100001, 0x080000, CRC(0cd5ca5e) SHA1(a59665e543e9383355de2576e6693348ec356591) ) // actually was a 27c4001

	ROM_REGION( 0x020000, "char", ROMREGION_INVERT )
	ROM_COPY( "test1", 0x080000, 0x00000, 0x020000 )

	ROM_REGION( 0x100000, "gfx3", ROMREGION_INVERT )    /* MBK tiles */
	ROM_COPY( "test1", 0x00000, 0x00000, 0x080000 )
	ROM_COPY( "test1", 0x100000, 0x80000, 0x080000 )

	ROM_REGION( 0x100000, "gfx4", ROMREGION_INVERT )    /* not used */
	ROM_COPY("gfx3",0x00000,0x00000,0x100000)

	ROM_REGION( 0x080000, "gfx5", ROMREGION_INVERT )    /* BK3 tiles */
	ROM_COPY( "test1", 0x180000, 0x00000, 0x080000 )

	ROM_REGION( 0x080000, "gfx6", ROMREGION_INVERT )    /* LBK tiles */
	ROM_COPY( "gfx5", 0x00000, 0x00000, 0x080000 )

	ROM_REGION( 0x100000, "adpcm", ROMREGION_ERASEFF )  /* ADPCM samples */
	ROM_LOAD( "2.27010.u26",    0x000000, 0x020000, CRC(a70d4f03) SHA1(c2482e624c8a828a94206a36d10c1021ad8ca1d0) ) // 27c010a
	ROM_LOAD( "3.27040.u25",    0x080000, 0x080000, CRC(6e254d12) SHA1(857779dbd276b688201a8ea3afd5817e38acad2e) ) // actually was a 27c4001

	ROM_REGION( 0x200000, "oki", ROMREGION_ERASEFF )
	ROM_COPY( "adpcm", 0x00000, 0x000000, 0x20000 ) //bank 0
	ROM_COPY( "adpcm", 0x00000, 0x020000, 0x20000 )
	ROM_COPY( "adpcm", 0x00000, 0x100000, 0x20000 ) //bank 4
	ROM_COPY( "adpcm", 0x80000, 0x120000, 0x20000 )
	ROM_COPY( "adpcm", 0x00000, 0x140000, 0x20000 ) //bank 5
	ROM_COPY( "adpcm", 0xa0000, 0x160000, 0x20000 )
	ROM_COPY( "adpcm", 0x00000, 0x180000, 0x20000 ) //bank 6
	ROM_COPY( "adpcm", 0xc0000, 0x1a0000, 0x20000 )
	ROM_COPY( "adpcm", 0x00000, 0x1c0000, 0x20000 ) //bank 7
	ROM_COPY( "adpcm", 0xe0000, 0x1e0000, 0x20000 )


	/* these are maths tables, for whatever COP replacement the bootlegs use */
	ROM_REGION( 0x500000, "unknown0", 0 )
	ROM_LOAD16_BYTE( "13.bin", 0x00000, 0x010000, CRC(229bddd8) SHA1(0924bf29db9c5a970546f154e7752697fdce6a58) ) // 27c512
	ROM_LOAD16_BYTE( "12.bin", 0x00001, 0x010000, CRC(dabfa826) SHA1(0db587c846755491b169ef7751ba8e7cdc2607e6) ) // 27c512
	ROM_REGION( 0x500000, "unknown1", 0 )
	ROM_LOAD16_BYTE( "15.bin", 0x00000, 0x080000, CRC(8fd87e65) SHA1(acc9fd0289fa9ab60bec16d3e642039380e5180a) ) // 27c4001
	ROM_LOAD16_BYTE( "14.bin", 0x00001, 0x080000, CRC(566086c2) SHA1(b7d09ce978f99ecc0d1975b31330ed49317701d5) ) // 27c4001
ROM_END

ROM_START( cupsocsb3 )
	ROM_REGION( 0x100000, "maincpu", 0 ) /* 68000 Code */
	ROM_LOAD16_BYTE( "4.bin", 0x00001, 0x80000, CRC(f615058f) SHA1(f7c0eb6b9f8dcdc8b13f8e5b03f46252a87a6c0f) ) // sldh
	ROM_LOAD16_BYTE( "5.bin", 0x00000, 0x80000, CRC(6500edf2) SHA1(1a617b18b4997c24af53601c98e9a0efbe637a4b) ) // sldh

	ROM_REGION( 0x10000, "audiocpu", 0 )    /* Z80 code */
	ROM_LOAD( "sc_01.bin",    0x000000, 0x08000, CRC(cea39d6d) SHA1(f0b79c03ffafdd1e57673d6d4836becbe415110b) )
	ROM_CONTINUE(             0x000000, 0x08000 )

	ROM_REGION( 0x200000, "sprite", ROMREGION_INVERT ) /* bootleg sprite gfx */
	ROM_LOAD( "sc_07.bin", 0x000000, 0x080000, CRC(dcb29d01) SHA1(72b4234622605f0ab03f21fdb6a61c6dac36000d) )
	ROM_LOAD( "sc_06.bin", 0x080000, 0x080000, CRC(2dc70e05) SHA1(f1d0beb8428a7e1d7c7818e6719abdc543b2fa80) )
	ROM_COPY( "sprite", 0x00000, 0x100000, 0x100000 )

	ROM_REGION( 0x200000, "test1", 0 ) /* bootleg tile gfx */
	ROM_LOAD16_BYTE( "sc_09.bin", 0x000000, 0x080000, CRC(695b6342) SHA1(dfccb43789021ba2568b9284ae61e64f7f89b152) )
	ROM_LOAD16_BYTE( "sc_10.bin", 0x000001, 0x080000, CRC(27e172b8) SHA1(ed86db2f42c8061607d46f2407b0130aaf692a02) )
	ROM_LOAD16_BYTE( "sc_08.bin", 0x100000, 0x080000, CRC(637120f3) SHA1(b4b2ad192e46ff80d4cb440d7fb6dac215a353ed) )
	ROM_LOAD16_BYTE( "sc_11.bin", 0x100001, 0x080000, CRC(0cd5ca5e) SHA1(a59665e543e9383355de2576e6693348ec356591) )

	ROM_REGION( 0x020000, "char", ROMREGION_INVERT )
	ROM_COPY( "test1", 0x080000, 0x00000, 0x020000 )

	ROM_REGION( 0x100000, "gfx3", ROMREGION_INVERT )    /* MBK tiles */
	ROM_COPY( "test1", 0x00000, 0x00000, 0x080000 )
	ROM_COPY( "test1", 0x100000, 0x80000, 0x080000 )

	ROM_REGION( 0x100000, "gfx4", ROMREGION_INVERT )    /* not used */
	ROM_COPY("gfx3",0x00000,0x00000,0x100000)

	ROM_REGION( 0x080000, "gfx5", ROMREGION_INVERT )    /* BK3 tiles */
	ROM_COPY( "test1", 0x180000, 0x00000, 0x080000 )

	ROM_REGION( 0x080000, "gfx6", ROMREGION_INVERT )    /* LBK tiles */
	ROM_COPY( "gfx5", 0x00000, 0x00000, 0x080000 )

	ROM_REGION( 0x100000, "adpcm", ROMREGION_ERASEFF )  /* ADPCM samples */
	ROM_LOAD( "sc_02.bin",    0x000000, 0x020000, CRC(a70d4f03) SHA1(c2482e624c8a828a94206a36d10c1021ad8ca1d0) )
	ROM_LOAD( "sc_03.bin",    0x080000, 0x080000, CRC(6e254d12) SHA1(857779dbd276b688201a8ea3afd5817e38acad2e) )

	ROM_REGION( 0x200000, "oki", ROMREGION_ERASEFF )
	ROM_COPY( "adpcm", 0x00000, 0x000000, 0x20000 ) //bank 0
	ROM_COPY( "adpcm", 0x00000, 0x020000, 0x20000 )
	ROM_COPY( "adpcm", 0x00000, 0x100000, 0x20000 ) //bank 4
	ROM_COPY( "adpcm", 0x80000, 0x120000, 0x20000 )
	ROM_COPY( "adpcm", 0x00000, 0x140000, 0x20000 ) //bank 5
	ROM_COPY( "adpcm", 0xa0000, 0x160000, 0x20000 )
	ROM_COPY( "adpcm", 0x00000, 0x180000, 0x20000 ) //bank 6
	ROM_COPY( "adpcm", 0xc0000, 0x1a0000, 0x20000 )
	ROM_COPY( "adpcm", 0x00000, 0x1c0000, 0x20000 ) //bank 7
	ROM_COPY( "adpcm", 0xe0000, 0x1e0000, 0x20000 )

	/* these are maths tables, for whatever COP replacement the bootlegs use */
	ROM_REGION( 0x020000, "unknown0", 0 )
	ROM_LOAD16_BYTE( "sc_13.bin", 0x00000, 0x010000, CRC(229bddd8) SHA1(0924bf29db9c5a970546f154e7752697fdce6a58) )
	ROM_LOAD16_BYTE( "sc_12.bin", 0x00001, 0x010000, CRC(dabfa826) SHA1(0db587c846755491b169ef7751ba8e7cdc2607e6) )
	ROM_REGION( 0x100000, "unknown1", 0 )
	ROM_LOAD16_BYTE( "sc_15.bin", 0x00000, 0x080000, CRC(8fd87e65) SHA1(acc9fd0289fa9ab60bec16d3e642039380e5180a) )
	ROM_LOAD16_BYTE( "sc_14.bin", 0x00001, 0x080000, CRC(566086c2) SHA1(b7d09ce978f99ecc0d1975b31330ed49317701d5) )
ROM_END

// almost identical to cupsocsb2, but for 02.u26 (OKI ROM) which has 3 different bytes: 0x8000 from 0x80 to 0xff; 0x10000 from 0xa4 to 0x4a; 0x18000 from 0x4a to 0xa4.
// this has been found on 2 different PCBs, so it seems intentional
ROM_START( cupsocsb4 )
	ROM_REGION( 0x100000, "maincpu", 0 ) /* 68000 Code */
	ROM_LOAD16_BYTE( "4.u11", 0x00001, 0x80000, CRC(83db76f8) SHA1(ffcd0a728de58871b945c15cc27da374b587e170) ) // 27c4001
	ROM_LOAD16_BYTE( "5.u17", 0x00000, 0x80000, CRC(c01e88c6) SHA1(8f90261792343c92ddd877ab8a2480b5aac82961) ) // 27c4001

	ROM_REGION( 0x10000, "audiocpu", 0 )    /* Z80 code */
	ROM_LOAD( "1.u20",    0x000000, 0x08000, CRC(cea39d6d) SHA1(f0b79c03ffafdd1e57673d6d4836becbe415110b) ) // 27c512
	ROM_CONTINUE(             0x000000, 0x08000 )

	ROM_REGION( 0x200000, "sprite", ROMREGION_INVERT ) /* bootleg sprite gfx */
	ROM_LOAD( "7.u86", 0x000000, 0x080000, CRC(dcb29d01) SHA1(72b4234622605f0ab03f21fdb6a61c6dac36000d) ) // 27c4001
	ROM_LOAD( "6.u87", 0x080000, 0x080000, CRC(2dc70e05) SHA1(f1d0beb8428a7e1d7c7818e6719abdc543b2fa80) ) // 27c4001
	ROM_COPY( "sprite", 0x00000, 0x100000, 0x100000 )

	ROM_REGION( 0x200000, "test1", 0 ) /* bootleg tile gfx */
	ROM_LOAD16_BYTE( "9.u36",  0x000000, 0x080000, CRC(695b6342) SHA1(dfccb43789021ba2568b9284ae61e64f7f89b152) ) // 27c4001
	ROM_LOAD16_BYTE( "10.u37", 0x000001, 0x080000, CRC(27e172b8) SHA1(ed86db2f42c8061607d46f2407b0130aaf692a02) ) // 27c4001
	ROM_LOAD16_BYTE( "8.u38",  0x100000, 0x080000, CRC(637120f3) SHA1(b4b2ad192e46ff80d4cb440d7fb6dac215a353ed) ) // 27c4001
	ROM_LOAD16_BYTE( "11.u39", 0x100001, 0x080000, CRC(0cd5ca5e) SHA1(a59665e543e9383355de2576e6693348ec356591) ) // 27c4001

	ROM_REGION( 0x020000, "char", ROMREGION_INVERT )
	ROM_COPY( "test1", 0x080000, 0x00000, 0x020000 )

	ROM_REGION( 0x100000, "gfx3", ROMREGION_INVERT )    /* MBK tiles */
	ROM_COPY( "test1", 0x000000, 0x00000, 0x080000 )
	ROM_COPY( "test1", 0x100000, 0x80000, 0x080000 )

	ROM_REGION( 0x100000, "gfx4", ROMREGION_INVERT )    /* not used */
	ROM_COPY("gfx3",0x00000,0x00000,0x100000)

	ROM_REGION( 0x080000, "gfx5", ROMREGION_INVERT )    /* BK3 tiles */
	ROM_COPY( "test1", 0x180000, 0x00000, 0x080000 )

	ROM_REGION( 0x080000, "gfx6", ROMREGION_INVERT )    /* LBK tiles */
	ROM_COPY( "gfx5", 0x00000, 0x00000, 0x080000 )

	ROM_REGION( 0x100000, "adpcm", ROMREGION_ERASEFF )  /* ADPCM samples */
	ROM_LOAD( "2.u26",    0x000000, 0x020000, CRC(4d8e554a) SHA1(9f8adaed10222216c9db8869fe2c9d02f8b1ef1f) ) // 27c010a
	ROM_LOAD( "3.u25",    0x080000, 0x080000, CRC(6e254d12) SHA1(857779dbd276b688201a8ea3afd5817e38acad2e) ) // 27c4001

	ROM_REGION( 0x200000, "oki", ROMREGION_ERASEFF )
	ROM_COPY( "adpcm", 0x00000, 0x000000, 0x20000 ) //bank 0
	ROM_COPY( "adpcm", 0x00000, 0x020000, 0x20000 )
	ROM_COPY( "adpcm", 0x00000, 0x100000, 0x20000 ) //bank 4
	ROM_COPY( "adpcm", 0x80000, 0x120000, 0x20000 )
	ROM_COPY( "adpcm", 0x00000, 0x140000, 0x20000 ) //bank 5
	ROM_COPY( "adpcm", 0xa0000, 0x160000, 0x20000 )
	ROM_COPY( "adpcm", 0x00000, 0x180000, 0x20000 ) //bank 6
	ROM_COPY( "adpcm", 0xc0000, 0x1a0000, 0x20000 )
	ROM_COPY( "adpcm", 0x00000, 0x1c0000, 0x20000 ) //bank 7
	ROM_COPY( "adpcm", 0xe0000, 0x1e0000, 0x20000 )


	/* these are maths tables, for whatever COP replacement the bootlegs use */
	ROM_REGION( 0x500000, "unknown0", 0 )
	ROM_LOAD16_BYTE( "13.bin", 0x00000, 0x010000, CRC(229bddd8) SHA1(0924bf29db9c5a970546f154e7752697fdce6a58) ) // 27c512
	ROM_LOAD16_BYTE( "12.bin", 0x00001, 0x010000, CRC(dabfa826) SHA1(0db587c846755491b169ef7751ba8e7cdc2607e6) ) // 27c512
	ROM_REGION( 0x500000, "unknown1", 0 )
	ROM_LOAD16_BYTE( "15.bin", 0x00000, 0x080000, CRC(8fd87e65) SHA1(acc9fd0289fa9ab60bec16d3e642039380e5180a) ) // 27c4001
	ROM_LOAD16_BYTE( "14.bin", 0x00001, 0x080000, CRC(566086c2) SHA1(b7d09ce978f99ecc0d1975b31330ed49317701d5) ) // 27c4001
ROM_END


GAME( 1992, cupsocsb,  cupsoc,   cupsocbl, cupsoc, seicupbl_state, empty_init, ROT0, "bootleg", "Seibu Cup Soccer :Selection: (bootleg, set 1)", MACHINE_UNEMULATED_PROTECTION | MACHINE_NOT_WORKING )
GAME( 1992, cupsocsb2, cupsoc,   cupsocbl, cupsoc, seicupbl_state, empty_init, ROT0, "bootleg", "Seibu Cup Soccer :Selection: (bootleg, set 2)", MACHINE_UNEMULATED_PROTECTION | MACHINE_NOT_WORKING )
GAME( 1992, cupsocsb3, cupsoc,   cupsocbl, cupsoc, seicupbl_state, empty_init, ROT0, "bootleg", "Seibu Cup Soccer :Selection: (bootleg, set 3)", MACHINE_UNEMULATED_PROTECTION | MACHINE_NOT_WORKING )
GAME( 1992, cupsocsb4, cupsoc,   cupsocbl, cupsoc, seicupbl_state, empty_init, ROT0, "bootleg", "Seibu Cup Soccer :Selection: (bootleg, set 4)", MACHINE_UNEMULATED_PROTECTION | MACHINE_NOT_WORKING )

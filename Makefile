PRJNAME := binary_gunner
OBJS := data.rel actor.rel map.rel shot.rel score.rel binary_gunner.rel

all: $(PRJNAME).sms

data.c: data/* data/sprites_tiles.psgcompr data/tileset_tiles.psgcompr data/title_tiles.psgcompr data/level1.bin \
	data/path1.path data/path2.path data/path3.path data/path4.path \
	data/player_shot.psg data/enemy_death.psg data/player_death.psg
	folder2c data data
	
data/sprites_tiles.psgcompr: data/img/sprites.png
	BMP2Tile.exe data/img/sprites.png -noremovedupes -8x16 -palsms -fullpalette -savetiles data/sprites_tiles.psgcompr -savepalette data/sprites_palette.bin

data/tileset_tiles.psgcompr: data/img/tileset.png
	BMP2Tile.exe data/img/tileset.png -noremovedupes -8x16 -palsms -fullpalette -savetiles data/tileset_tiles.psgcompr -savepalette data/tileset_palette.bin

data/title_tiles.psgcompr: data/img/title.png
	BMP2Tile.exe data/img/title.png -palsms -fullpalette -savetiles data/title_tiles.psgcompr -savetilemap data/title_tilemap.bin -savepalette data/title_palette.bin

data/path1.path: data/path/path1.spline.json
	node tool/convert_splines.js data/path/path1.spline.json data/path1.path

data/path2.path: data/path/path2.spline.json
	node tool/convert_splines.js data/path/path2.spline.json data/path2.path

data/path3.path: data/path/path3.spline.json
	node tool/convert_splines.js data/path/path3.spline.json data/path3.path

data/path4.path: data/path/path4.spline.json
	node tool/convert_splines.js data/path/path4.spline.json data/path4.path

data/level1.bin: data/map/level1.tmx
	node tool/convert_map.js data/map/level1.tmx data/level1.bin
	
data/enemy_death.psg: data/deflemask/enemy_death.vgm
	vgm2psg data/deflemask/enemy_death.vgm data/enemy_death.psg 23

data/player_death.psg: data/deflemask/player_death.vgm
	vgm2psg data/deflemask/player_death.vgm data/player_death.psg

%.vgm: %.wav
	psgtalk -r 512 -u 1 -m vgm $<

%.rel : %.c
	sdcc -c -mz80 --peep-file lib/peep-rules.txt $<

$(PRJNAME).sms: $(OBJS)
	sdcc -o $(PRJNAME).ihx -mz80 --no-std-crt0 --data-loc 0xC000 lib/crt0_sms.rel $(OBJS) SMSlib.lib lib/PSGlib.rel
	ihx2sms $(PRJNAME).ihx $(PRJNAME).sms	

clean:
	rm *.sms *.sav *.asm *.sym *.rel *.noi *.map *.lst *.lk *.ihx data.*

LIBDIR = lib8001
DEPFILES = main.c sp.c name.c game.c bg.c emitter.c chr.c title.c
DEPEND = Depend

all: $(DEPEND) xevious_r.ihx 

xevious_r.ihx: main.lk $(LIBDIR)/lib8001.a $(LIBDIR)/crt0.rel \
	main.rel game.rel sp.rel bg.rel bgdraw.rel emitter.rel chr.rel \
	bgdata.rel es.rel title.rel name.rel music.rel pattern.rel
	sdldz80 -nf main.lk
	$(LIBDIR)/mkadr.pl *.sym $(LIBDIR)/*.sym > xevious_r.adr
	$(LIBDIR)/total.pl xevious_r.map

bgconf.h bgdata.s: mkbg.pl map.bmp
	./mkbg.pl

pattern.h pattern.s: sprite.tim $(LIBDIR)/mkpat.pl cut
	$(LIBDIR)/mkpat.pl -t $< 

es.c: mkes.pl essrc
	./mkes.pl essrc > $@

music.h music.s: mkmusic
	./mkmusic

mkmusic: msrc.cpp $(LIBDIR)/mkmusic.h $(LIBDIR)/mml.c $(LIBDIR)/tone_dcsg.c
	c++ -std=c++11 -Wno-deprecated -I$(LIBDIR) -o $@ $< $(LIBDIR)/mml.c $(LIBDIR)/tone_dcsg.c

$(LIBDIR)/lib8001.a:
	make -C $(LIBDIR)

$(DEPEND):
	touch music.h pattern.h bgconf.h
	rm -f $(DEPEND)
	for file in $(DEPFILES); do sdcc -I$(LIBDIR) -MM $$file >> $(DEPEND); done
	rm -f music.h pattern.h bgconf.h

clean:
	rm -f $(DEPEND) mkmusic es.c music.* pattern.* bgconf.h bgdata.s
	rm -f *.{adr,asm,ihx,lst,map,noi,rel,sym}
	make -C $(LIBDIR) clean

%.rel: %.c
	sdcc -c -mz80 -I$(LIBDIR) $<
%.rel: %.s
	sdasz80 -lsw -o $@ $<

-include $(DEPEND)

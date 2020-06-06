#! /usr/bin/perl

$pre_blank1 = 160;
$pre_blank2 = 64;

open(S, "map.bmp") || die;
binmode S;
read(S, $_, -s S);
close S;
@img = unpack("C*", $_);
$imgofs = $img[10] | $img[11] << 8;
$width = ($img[18] | $img[19] << 8) + 3 >> 2 << 2;
$height = $img[22] | $img[23] << 8;

for ($y = 0; $y < $height >> 2; $y++) {
	push(@ofs, $#d + 1);
	push(@d, $pre_blank1 >> 1, 0, $pre_blank2 >> 1, 0);
	$l = -1;
	$c = 0;
	for ($x = 0; $x < $width >> 1; $x++) {
		$t = 0;
		for ($i = 0; $i < 8; $i++) {
			$t = $t >> 1 | ($img[$imgofs + $width * ($height - 1 - (($y << 2) + ($i & 3))) + ($x << 1) + ($i >> 2)] < 0x80) << 7;
		}
		if ($l != $t || $c >= 255) {
			push(@d, $c, $l) if $l >= 0;
			$l = $t;
			$c = 0;
		}
		$c++;
	}
	push(@d, $c, $l);
}

open(O, "> bgdata.s") || die;
print O "\t.module\tbgdata\n\t.globl\t_bgdata\n\t.area\t_CODE\n_bgdata:\n";
for ($i = 0; $i < @ofs; $i++) {
	printf O "\t.dw\t0x%04x\n", $ofs[$i] + ($height >> 2 << 1);
}
for ($i = 0; $i < @d; $i++) {
	printf O "\t.db\t0x%02x\n", $d[$i] & 0xff;
}
close O;
open(O, "> bgconf.h") || die;
printf O "#define BG_XN\t%d\n#define BG_YN\t%d\n", $width, $height;
printf O "#define PRE_BLANK1\t%d\n", $pre_blank1;
printf O "#define PRE_BLANK2\t%d\n", $pre_blank2;
close O;
exit 0;

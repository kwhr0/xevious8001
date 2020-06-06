#! /usr/bin/perl

@yofs = (
	58, 90, 13, 77, 32, 66, 0, 
	87, 52, 7, 77, 87, 40, 13, 67, 90
);
$bg_xn = 884;
$preblank = 64;

$ground{$_} = 1 foreach ("barra", "garubarra", "zolbak", "logram", "bozalogram", "domogram", "grobda", "derota", "garuderota", "sol");

print  "#include \"emitter.h\"\n\nconst EmitterSch es[] = {\n";
while (<>) {
	chomp;
	s/#.*$//;
	@_ = split(/\s/, $_);
	next unless @_;
	$s = shift(@_);
	$x = shift(@_);
	$y = shift(@_);
	$t = shift(@_);
	$fix = $t =~ s/\+//;
	$t = "random" unless $fix || $ground{$t};
	$f = $t eq "domogram" || $t eq "grobda";
	$y = $y - 3 - 4 * $yofs[$s - 1] if $ground{$t};
	unshift(@_, $y);
	push(@move, join(' ', @_)) if $f;
	$_ = ($s - 1) * ($bg_xn + $preblank) + $x - $preblank - 9;
	$t =~ tr/a-z/A-Z/;
	printf "\t{ %s0x%04x, 0x%02x, %s },\n", 
		$_ < 0 ? "-" : "", $_ < 0 ? -$_ : $_, $f ? $cnt++ : $y, $t;
}
print "\t{ 0x7fff }\n};\n";
print "const u8 extradata[] = {\n";
foreach (@move) {
	@_ = split(/\s/, $_);
	$_ = shift(@_);
	printf "\t0x%02x, ", $_;
	push(@ofs, $n);
	$n += 2 * @_ + 2;
	foreach (@_) {
		if (/,/) {
			($t, $x, $y) = split(',', $_);
			printf "0x%02x, ", $t;
			printf "0x%02x, ", 4 * $x << 4 & 0xf0 | 4 * $y & 0xf;
		}
		else {
			printf "0x%02x, 0x%02x, ", $_ & 0xff, $_ >> 8;
		}
	}
	print "0,\n";
}
print "};\n";
print "const u16 extraofs[] = {\n";
for ($i = 0; $i < @ofs; $i++) {
	print "\t" unless $i & 7;
	printf "0x%03x, ", $ofs[$i];
	print "\n" if ($i & 7) == 7;
}
print "\n};\n";
exit 0;
